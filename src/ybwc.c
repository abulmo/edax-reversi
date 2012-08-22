/**
 * @file ybwc.c
 *
 * @brief Parallel search.
 *
 * The Young Brother Wait Concept [1, 2] is an efficient technique to search a
 * position with several cpu/core working in parallel. At an inner nodes, this
 * technique always evaluates the first move using a sequential approach, but
 * try to evaluate the siblings in parallel, once the first move has been
 * computed. The YBWC has some nice properties: low search overhead, good scalability,
 * easy implementation, etc.
 *
 * This file holds the function definition to manipulate structures used in our
 * implementation of parallelism:
 *  - Node describes a position shared between different threads.
 *  - Task describes a search running in parallel within a thread.
 *  - TaskStack is a FIFO providing task available for a new search.
 *
 * References:
 *
 * -# Feldmann R., Monien B., Mysliwietz P. Vornberger O. (1989) Distributed Game-Tree Search.
 * ICCA Journal, Vol. 12, No. 2, pp. 65-73.
 * -# Feldmann R. (1993) Game-Tree Search on Massively Parallel System - PhD Thesis, Paderborn (English version).
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#include "ybwc.h"

#include "move.h"
#include "options.h"
#include "util.h"
#include "search.h"
#include "stats.h"
#include "settings.h"

#include <assert.h>
#include <stdlib.h>

extern Log search_log[1];

/**
 * @brief Initialize a node
 *
 * Initialize the various members of the node structure. The important part of
 * the initialization establishes a master-slave relashionship between this
 * node (the slave) and its parent (the master).
 *
 * @param node A potentially shared position.
 * @param search The position searched
 * @param alpha lower score bound.
 * @param beta upper score bound.
 * @param depth depth.
 * @param n_moves Move count.
 * @param parent The parent node.
 */
void node_init(Node* node, Search *search, const int alpha, const int beta, const int depth, const int n_moves, Node* parent)
{
	assert(node != NULL);
	assert(SCORE_MIN <= alpha && alpha <= SCORE_MAX);
	assert(SCORE_MIN <= beta && beta <= SCORE_MAX);
	assert(alpha < beta);

	lock_init(node);
	condition_init(node);

	node->alpha = alpha;
	node->beta = beta;
	node->depth = depth;
	node->height = search->height;
	node->move = NULL;
	node->pv_node = false;
	node->bestmove = NOMOVE;
	node->bestscore = -SCORE_INF;
	node->n_moves_todo = n_moves;
	node->n_moves_done = 0;
	node->parent = parent;
	node->search = search;
	node->slave = NULL;
	node->has_slave = false;
	node->is_waiting = false;
	node->is_helping = false;
	node->stop_point = false;
}

/**
 * @brief Free Resources allocated by a node
 *
 * @param node Node.
 */
void node_free(Node *node)
{
	lock_free(node);
	condition_free(node);
}

/**
 * @brief Seek for & use an helper node.
 *
 * When asking for an idle task, this function look if a parent node is waiting
 * and has a task available to help the current search.
 *
 * @param master A parent node with an idle task.
 * @param node Searched node.
 * @param move Searched move.
 * @return true if an helper task has been launch, false otherwise.
 */
static bool get_helper(Node *master, Node *node, Move *move)
{
	Task *task;
	bool found = false;

	if (master) {
		if (master->is_waiting && !master->is_helping) {
			lock(master);
			if (master->has_slave && master->is_waiting && !master->is_helping) {
				master->is_helping = true;
				task = master->help;
				task_init(task) ;
				task->is_helping = true;
				task->node = node;
				task->move = move;
				search_clone(task->search, node->search);
				node->has_slave = true;
				node->slave = task->search;
				task->run = true;
				found = true;

				condition_broadcast(master);
			}
			unlock(master);
		} else {
			found = get_helper(master->parent, node, move);
		}
	}

	return found;
}

/**
 * @brief Node split.
 *
 * Here is the heart of the YBWC algorithm. It splits a node into two tasks.
 * Splitting occurs if the following conditions are met:
 *   -# <b> the first move has been already searched</b>. The main principle of the
 * YBWC algorithm. It avoids to split a cut-type node, for which searching a
 * single move is enough, and thus diminish search-overhead.
 *   -# search to do is deep enough. In order to diminish the overhead of the
 * parallelism. Can be tuned through SPLIT_MIN_DEPTH.
 *   -# the node has not been splitted yet. For a single position, only two tasks
 * can run in parallel. The idea is to favor splitting elsewhere in the tree.
 *   -# This is not the last move. The idea is to lose less time in
 * waiting for the parallel task to terminate. Can be tuned through SPLIT_MIN_MOVES_TODO.
 * If these conditions are met, an idle task is requested, first from an idle task of a
 * parent node; then, if none is available, from the idle task stack storage. If no idle task
 * is found, the node splitting fails.
 *
 * @param node Master node to split.
 * @param move move to search.
 * @return true if the split was a success, false otherwise.
 */
bool node_split(Node *node, Move *move)
{
	Task *task;
	Search *search = node->search;

	if (search->allow_node_splitting // split only if parallelism is on
	 && node->depth >= SPLIT_MIN_DEPTH // split if we are deep enough
	 && node->n_moves_done // do not split first move (ybwc main principle).
	 && node->has_slave == false // Not splitted yet.
	 && node->n_moves_todo >=  SPLIT_MIN_MOVES_TODO) {  // do not split the last move(s), to diminish waiting time
		YBWC_STATS(atomic_add(&statistics.n_split_try, 1);)

		if (get_helper(node->parent, node, move)) {
			YBWC_STATS(atomic_add(&statistics.n_master_helper, 1);)
			return true;
		} else if ((task = task_stack_get_idle_task(search->tasks)) != NULL) {
			task->node = node;
			task->move = move;
			search_clone(task->search, search);
			node->has_slave = true;
			node->slave = task->search;
			YBWC_STATS(atomic_add(&statistics.n_split_success, 1);)

			lock(task);
				task->run = true;
				condition_signal(task);
			unlock(task);

			return true;
		}
	}
	return false;
}

/**
 * @brief Wait for slaves termination.
 *
 * Actually, three steps are performed here:
 *   -# Stop slaves node in case their scores are unneeded.
 *   -# Wait for slaves' termination.
 *   -# Wake-up the master thread that may have been stopped.
 *
 * @param node Node.
 */
void node_wait_slaves(Node* node)
{
	lock(node);
	// stop slave ?
	if ((node->alpha >= node->beta || node->search->stop) && node->has_slave) {
		search_stop_all(node->slave, STOP_PARALLEL_SEARCH);
		YBWC_STATS(atomic_add(&statistics.n_stopped_slave, 1);)
	}

	// wait slaves
	YBWC_STATS(atomic_add(&statistics.n_waited_slave, node->has_slave);)
	while (node->has_slave) {
		node->is_waiting = true;
		assert(node->is_helping == false);
		condition_wait(node);

		if (node->is_helping) {
			assert(node->help->run);
			task_search(node->help);
			task_free(node->help);
			node->is_helping = false;
		} else {
			node->is_waiting = false;
		}
	}

	// wake-up master thread!
	if (node->search->stop == STOP_PARALLEL_SEARCH && node->stop_point) {
		node->search->stop = RUNNING;
		node->stop_point = false;
		YBWC_STATS(atomic_add(&statistics.n_wake_up, 1);)
	}
	unlock(node);
}


/**
 * @brief Update a node.
 *
 * Update bestmove, bestscore and alpha value of the node, in case the move is the bestmove found so far.
 * The function is thread-safe although it updates a shared resource. Double check lock is used as an optimization.
 *
 * @param node current node.
 * @param move last evaluated move.
 */
void node_update(Node* node, Move *move)
{
	Search *search = node->search;
	const int score = move->score;

	lock(node);
	if (!search->stop && score > node->bestscore) {
		node->bestscore = score;
		node->bestmove = move->x;
		if (node->height == 0) {
			record_best_move(search, search->board, move, node->alpha, node->beta, node->depth);
			search->result->n_moves_left--;
		}
		if (score > node->alpha) node->alpha = score;
	}
	if (node->alpha >= node->beta  && node->has_slave) { // stop slave ?
		search_stop_all(node->slave, STOP_PARALLEL_SEARCH);
		YBWC_STATS(atomic_add(&statistics.n_stopped_slave, 1);)
	}
	unlock(node);	
}

/**
 * @brief Get the first move of the move list.
 *
 * This is thread/safe getter of the first move. If the search is stopped,
 * or an alphabeta cut has been found or no move is available the function
 * returns NULL.
 *
 * @param node Node data.
 * @param movelist List of moves.
 * @return the first move of the list or NULL if none is available.
 */
Move* node_first_move(Node *node, MoveList *movelist)
{
	Move *move;
	lock(node);
		node->n_moves_todo = movelist->n_moves;
		node->n_moves_done = 0;
		node->move = movelist_first(movelist);
		if (node->move && !node->search->stop) {
			assert(node->alpha < node->beta);
			move = node->move;
		} else {
			move = NULL;
		}
	unlock(node);
	return move;
}

/**
 * @brief Get the next move of the move list.
 *
 * This is a thread/safe getter of the next move. If the search is stopped,
 * or an alphabeta cut has been found or no move is available the function
 * returns NULL.
 *
 * @param node Node data.
 * @return the next move of the list or NULL if none is available.
 */
static Move* node_next_move_lockless(Node *node)
{
	Move *move;
	if (node->move && node->alpha < node->beta && !node->search->stop) {
		++node->n_moves_done; --node->n_moves_todo;
		move = node->move = move_next(node->move);
	} else {
		move = NULL;
	}

	return move;
}

/**
 * @brief Get the next move of the move list.
 *
 * This is a thread/safe getter of the next move.
 *
 * @param node Node data.
 * @return the next move of the list.
 */
Move* node_next_move(Node *node)
{
	Move *move;
	lock(node);
		move = node_next_move_lockless(node);
	unlock(node);

	return move;
}

/**
 * @brief A parallel search within a Task structure.
 *
 * Here we share the search with a main parent task.
 *
 * @param task The task to search with.
 */
void task_search(Task *task)
{

	Node *node = task->node;
	Search *search = task->search;
	Move *move = task->move;
	int i;

	search_set_state(search, node->search->stop);

	YBWC_STATS(++task->n_calls;)

	while (move && !search->stop) {
		const int alpha = node->alpha;
		if (alpha >= node->beta) break;

		search_update_midgame(search, move);
			move->score = -NWS_midgame(search, -alpha - 1, node->depth - 1, node);
			if (alpha < move->score && move->score < node->beta) {
				move->score = -PVS_midgame(search, -node->beta, -alpha, node->depth - 1, node);
				assert(node->pv_node == true);
			}
		search_restore_midgame(search, move);
		if (node->height == 0) {
			move->cost = search_get_pv_cost(search);
			move->score = search_bound(search, move->score);
			if (log_is_open(search_log)) show_current_move(search_log->f, search, move, alpha, node->beta, true);
		}

		lock(node);
		if (!search->stop && move->score > node->bestscore) {
			node->bestscore = move->score;
			node->bestmove = move->x;
			if (node->height == 0) {
				record_best_move(search, search->board, move, alpha, node->beta, node->depth);
				search->result->n_moves_left--;
				if (search->options.verbosity == 4) pv_debug(search, move, stdout);
			}
			if (node->bestscore > node->alpha) {
				node->alpha = node->bestscore;
				if (node->alpha >= node->beta && node->search->stop == RUNNING) { // stop the master thread?
					node->stop_point = true;
					node->search->stop = STOP_PARALLEL_SEARCH;
					YBWC_STATS(atomic_add(&statistics.n_stopped_master, 1);)
				}
			}
		}
		move = node_next_move_lockless(node);
		unlock(node);
	}

	search_set_state(search, STOP_END);

	spin_lock(search->parent);
		for (i = 0; i < search->parent->n_child; ++i) {
			if (search->parent->child[i] == search) {
				--search->parent->n_child;
				search->parent->child[i] = search->parent->child[search->parent->n_child];
				break;
			}
		}
		search->parent->child_nodes += search_count_nodes(search);
		YBWC_STATS(task->n_nodes += search->n_nodes;)
	spin_unlock(search->parent);

	lock(node);
		task->run = false;
		node->has_slave = false;
		condition_broadcast(node);
	unlock(node);
}


/**
 * @brief The main loop runned by a task.
 *
 * When task->run is set to true, the task starts a parallel search.
 * In order to diminish the parallelism overhead, we do not launch a new
 * thread at each new splitted node. Instead the threads are created at the
 * beginning of the program and run a waiting loop who enters/quits a
 * parallel search when requested.
 *
 * @param param The task.
 * @return NULL.
 */
void* task_loop(void *param)
{
	Task *task = (Task*) param;

	lock(task);
	task->loop = true;

	while (task->loop) {
		if (!task->run) {
			condition_wait(task);
		}
		if (task->run) {
			task_search(task);
			task_stack_put_idle_task(task->container, task);
		}
	}

	unlock(task);

	return NULL;
}

/**
 * @brief Create a search structure for a task.
 *
 * @param task The task.
 * @return a search structure.
 */
static Search* task_search_create(Task *task)
{
	Search *search;

	search = (Search*) malloc(sizeof (Search)); // allocate the search attached to this task
	if (search == NULL) {
		fatal_error("task_init: cannot allocate the search position.\n");
	}
	search->n_nodes = 0;
	search->n_child = 0;
	search->parent = NULL;
	eval_init(search->eval);
	spin_init(search);
	search->task = task;
	search->stop = STOP_END;

	return search;
}

/**
 * @brief Free a search structure of a task.
 *
 * @param search The search structure.
 */
static void task_search_destroy(Search *search)
{
	eval_free(search->eval);
	spin_free(search);
	free(search);
}

/**
 * @brief Initialize a task.
 *
 * Initialize task data members and start the task
 * main loop task_loop() within a thread.
 *
 * @param task The task.
 */
void task_init(Task *task)
{
	lock_init(task);
	condition_init(task);

	task->loop = false;
	task->run = false;
	task->node = NULL;
	task->move = NULL;
	task->n_calls = 0;
	task->n_nodes = 0;
	task->search = task_search_create(task);
}


/**
 * @brief Free resources used by a task.
 *
 * @param task The task.
 */
void task_free(Task *task)
{
	assert(task->run == false);
	if (task->loop) {
		task->loop = false; // stop the main loop
		condition_signal(task);
		thread_join(task->thread);
	}
	lock_free(task);
	condition_free(task);
	task_search_destroy(task->search); // free other resources
	task->search = NULL;
}

/**
 * @brief Initialize the stack of tasks
 *
 * @param stack The stack of tasks.
 * @param n Stack size (number of tasks).
 */
void task_stack_init(TaskStack *stack, const int n)
{
	int i;

	spin_init(stack);

	stack->n = n; // number of additional task
	stack->n_idle = 0;

	if (stack->n) {
		// allocate the tasks
		stack->task = (Task*) malloc(stack->n * sizeof (Task));
		stack->stack = (Task**) malloc(stack->n * sizeof (Task*));
		if (stack->task == NULL) {
			fatal_error("Cannot allocate an array of %d tasks\n", stack->n);
		}
		if (stack->task == NULL) {
			fatal_error("Cannot allocate a stack of %d entries\n", stack->n);
		}

		// init the tasks.
		for (i = 0; i < stack->n; ++i) {
			if (i) {
				task_init(stack->task + i);
				thread_create(&stack->task[i].thread, task_loop, stack->task + i);
				if (options.cpu_affinity) thread_set_cpu(stack->task[i].thread, i); /* CPU 0 to n - 1 */
			}
			stack->task[i].container = stack;
			stack->stack[i] = NULL;
		}

		// put the tasks onto stack;
		for (i = 1; i < stack->n; ++i) {
			task_stack_put_idle_task(stack, stack->task + i);
		}

	} else { // No parallel search
		stack->task = NULL;
		stack->stack = NULL;
	}
}

/**
 * @brief Free resources used by the stack of tasks
 *
 * @param stack The stack of tasks.
 */
void task_stack_free(TaskStack *stack)
{
	int i;
	for (i = 1; i < stack->n; ++i) {
		task_free(stack->task + i);
	}
	free(stack->task); stack->task = NULL;
	free(stack->stack); stack->stack = NULL;
	stack->n = 0;
	stack->n_idle = 0;
	spin_free(stack);
}

/**
 * @brief Free resources used by the stack of tasks
 *
 * @param stack Stack to resize.
 * @param n New stack size.
 */
void task_stack_resize(TaskStack *stack, const int n)
{
	task_stack_free(stack);
	task_stack_init(stack, n);
}

/**
 * @brief Return, if available, an idle task.
 *
 * @param stack The stack of tasks.
 * @return An idle task.
 */
Task* task_stack_get_idle_task(TaskStack *stack)
{
	Task *task;

	spin_lock(stack);

	if (stack->n_idle) {
		task = stack->stack[--stack->n_idle];
	} else {
		task = NULL;
	}

	spin_unlock(stack);

	return task;
}

/**
 * @brief Put back an idle task after using it.
 *
 * @param stack The stack of tasks.
 * @param task An idle task.
 */
void task_stack_put_idle_task(TaskStack *stack, Task *task)
{
	spin_lock(stack);

	stack->stack[stack->n_idle++] = task;

	spin_unlock(stack);
}

