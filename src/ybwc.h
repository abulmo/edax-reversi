/**
 * @file ybwc.h
 *
 * @brief Parallel search header.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 *
 */

#ifndef EDAX_YBWC_H
#define EDAX_YBWC_H

#include "util.h"
#include "const.h"
#include "settings.h"

#include <stdatomic.h>
#include <stdbool.h>

struct Search;
struct Move;
struct MoveList;
struct Task;

/**
 * A Task is a parallel search thread.
 */
typedef struct Task {
	struct Search *search;       /**< search data */
	struct Node *node;           /**< node splitted */
	struct Move *move;           /**< move to search */
	struct TaskStack *container; /**< link to its container */
	uint64_t n_calls;            /**< call counter */
	uint64_t n_nodes;            /**< nodes counter */
	thrd_t thread;               /**< thread */
	mtx_t mutex;                 /**< mutex (thread lock) */
	cnd_t condition;             /**< condition variable */
	bool loop;                   /**< loop flag */
	bool run;                    /**< run flag */
} Task;

/**
 * A Node is a position in the search tree, containing information shared with
 * parallel threads.
 */
typedef struct Node {
	struct Search *search;                  /**< master search structure */
	struct Search *slave[SPLIT_MAX_SLAVES]; /**< slave search structure */
	struct Node *parent;                    /**< master node */
	struct Move *move;                      /**< move to search */
	Task help;          /**< helper task */
	mtx_t mutex;        /**< mutex */
	cnd_t condition;    /**< condition variable */
	int bestmove;       /**< bestmove */
	int bestscore;      /**< bestscore */
	int alpha;          /**< alpha lower bound */
	int beta;           /**< beta upper bound (is constant after initialisation) */
	int n_slave;	    /**< number of slaves splitted flag */
	int depth;          /**< depth */
	int height;         /**< height */
	int n_moves_done;   /**< search done */
	int n_moves_todo;   /**< search todo */
	bool is_helping;    /**< waiting flag */
	bool pv_node;       /**< pv_node */
	bool stop_point;    /**< stop point flag */
	bool is_waiting;	/**< waiting flag */
} Node;

/* node function declaration */
void node_init(Node*, struct Search*, const int, const int, const int, const int, Node*);
void node_free(Node*);
bool node_split(Node*, struct Move*);
void node_stop_slaves(Node*);
void node_wait_slaves(Node*);
void node_update(Node*, struct Move*);
struct Move * node_first_move(Node*, struct MoveList*);
struct Move * node_next_move(Node*);

/* task function declaration */
int task_loop(void*);
int task_help(void*);
void task_init(Task*);
void task_free(Task*);
void task_update(Task*);
void task_search(Task *task);

/** @struct TaskStack
 *
 * A FILO of tasks
 */
typedef struct TaskStack {
	Task **stack;                /**< stack of tasks */
	Task *task;                  /**< set of tasks */
	mtx_t mutex;                 /**< mutex */
	int n;                       /**< maximal number of idle tasks */
	int n_idle;                  /**< number of idle tasks */
} TaskStack;

/* task stack function declaration */
void task_stack_init(TaskStack*, const int);
void task_stack_free(TaskStack*);
void task_stack_resize(TaskStack*, const int);
void task_stack_stop(TaskStack*, const Stop);
Task* task_stack_get_idle_task(TaskStack*);
void task_stack_put_idle_task(TaskStack*, Task*);
void task_stack_clear(TaskStack*);
uint64_t task_stack_count_nodes(TaskStack*);

#endif

