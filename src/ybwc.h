/**
 * @file ybwc.h
 *
 * @brief Parallel search header.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 *
 */

#ifndef EDAX_YBWC_H
#define EDAX_YBWC_H

#include "util.h"
#include "const.h"
#include "settings.h"

#include <stdbool.h>

struct Search;
struct Move;
struct MoveList;
struct Task;

/**
 * A Task is a parallel search thread.
 */
typedef struct Task {
	volatile bool loop;          /**< loop flag */
	volatile bool run;           /**< run flag */
	volatile bool is_helping;    /**< is helping */
	struct Search *search;       /**< search data */
	struct Node *node;           /**< node splitted */
	struct Move *move;           /**< move to search */
	Thread thread;               /**< thread */
	unsigned long long n_calls;  /**< call counter */
	unsigned long long n_nodes;  /**< nodes counter */
	Lock lock;                   /**< lock */
	Condition cond;              /**< condition */
	struct TaskStack *container; /**< link to its container */
} Task;

/**
 * A Node is a position in the search tree, containing information shared with
 * parallel threads.
 */
typedef struct Node {
	volatile int bestmove;       /**< bestmove */
	volatile int bestscore;      /**< bestscore */
	volatile int alpha;          /**< alpha lower bound */
	int beta;                    /**< beta upper bound (is constant after initialisation) */
	bool pv_node;                /**< pv_node */
	volatile bool has_slave;	 /**< slave flag */
	volatile bool stop_point;    /**< stop point flag */
	volatile bool is_waiting;	 /**< waiting flag */
	int depth;                   /**< depth */
	int height;                  /**< height */
	struct Search *search;       /**< master search structure */
	struct Search *slave;        /**< slave search structure */
	struct Node *parent;         /**< master node */
	struct Move *move;           /**< move to search */
	volatile int n_moves_done;   /**< search done */
	volatile int n_moves_todo;   /**< search todo */
	volatile bool is_helping;	 /**< waiting flag */
	Task help[1];                /**< helper task */
	Lock lock;                   /**< mutex */
	Condition cond;              /**< condition variable */
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
void* task_loop(void*);
void* task_help(void*);
void task_init(Task*);
void task_free(Task*);
void task_update(Task*);
void task_search(Task *task);

/** @struct TaskStack
 *
 * A FILO of tasks
 */
typedef struct TaskStack {
	SpinLock spin;               /**< mutex */
	Task *task;                  /**< set of tasks */
	Task **stack;                /**< stack of tasks */
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
unsigned long long task_stack_count_nodes(TaskStack*);

#endif

