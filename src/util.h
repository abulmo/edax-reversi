/**
 * @file util.h
 *
 * @brief Miscellaneous utilities header.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#ifndef EDAX_UTIL_H
#define EDAX_UTIL_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

struct Board;
struct Move;
struct Line;

/*
 * Time management
 */
extern long long (*time_clock)(void);
long long real_clock(void);
long long cpu_clock(void);
void time_print(long long, bool, FILE*);
long long time_read(FILE*);
void time_stamp(FILE*);
void relax(int);

/*
 * Special printing function
 */
char* format_scientific(double, const char*, char*);
void print_scientific(double, const char*, FILE*);

/*
 * Strings
 */
char* string_duplicate(const char*);
long long string_to_time(const char*);
char* string_read_line(FILE*);
char* string_copy_line(FILE*);
void string_to_lowercase(char*);
void string_to_uppercase(char*);
int string_to_coordinate(const char*);
char* string_to_word(char*);
bool string_to_boolean(const char*);
int string_to_int(const char*, const int);
double string_to_real(const char*, const double);
/*
 * Parsing
 */
char* parse_word(const char*, char*, unsigned int);
char* parse_field(const char*, char*, unsigned int, char);
char* parse_line(const char*, char*, unsigned int);
char* parse_board(const char*, struct Board*, int*);
char* parse_move(const char*, const struct Board*, struct Move*);
char* parse_game(const char*, const struct Board*, struct Line*);
char* parse_boolean(const char*, bool*);
char* parse_int(const char*, int*);
char* parse_real(const char*, double*);
char* parse_command(const char*, char*, char*, const unsigned int);
char* parse_find(const char*, const int);
char* parse_skip_spaces(const char*);
char* parse_skip_word(const char*);

/*
 * File.
 */
void path_get_dir(const char*, char*);
char* file_add_ext(const char*, const char*, char*); 
bool is_stdin_keyboard(void);

/*
 * random
 */
typedef struct Random {
	unsigned long long x;
} Random;

unsigned long long random_get(Random*);
void random_seed(Random*, const unsigned long long);

/*
 * Usefull macros
 */
/** Maximum of two values */
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/** Minimum of two values */
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/** Constrain a variable to a range of values. */
#define BOUND(var, min, max, name) do {\
	if (var < min && min <= max) {\
		if (name) fprintf(stderr, "\nWARNING: %s = %lld is out of range. Set to %lld\n", name, (long long)var, (long long)min);\
		var = min;\
	} else if (var > max) {\
		if (name) fprintf(stderr, "\nWARNING: %s = %lld is out of range. Set to %lld\n", name, (long long)var, (long long)max);\
		var = max;\
	}\
} while (0)


/*
 *Threads
 */
#if defined(__unix__) || (defined(_WIN32) && defined(USE_PTHREAD)) || defined(__APPLE__)

#include <pthread.h>

/** Typedef mutex to a personalized type for portability */
typedef pthread_t Thread;

/** Typedef mutex to a personalized type for portability */
typedef pthread_mutex_t Lock;

/** typedef conditional variable */
typedef pthread_cond_t Condition;

/** @macro to detach a thread */
#define thread_detach(thread) pthread_detach(thread)

#if DEBUG && defined(__unix__) && !defined(__APPLE__)
/** @macro Lock a mutex with a macro for genericity */
#define lock(c) if (pthread_mutex_lock(&(c)->lock)) { \
	error("lock"); \
} else (void) 0

/** @macro unlock a mutex with a macro for genericity */
#define unlock(c) if (pthread_mutex_unlock(&(c)->lock)) { \
	error("unlock"); \
} else (void) 0

/** @macro Initialize a mutex with a macro for genericity. */
#define lock_init(c) do {\
	pthread_mutexattr_t attr;\
	pthread_mutexattr_init(&attr);\
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP);\
	pthread_mutex_init(&(c)->lock, &attr);\
} while (0)

/** @macro Free a mutex with a macro for genericity. */
#define lock_free(c) if (pthread_mutex_destroy(&(c)->lock)) { \
	error("lock_free"); \
} else (void) 0

#else //NDEBUG

/** @macro Lock a mutex with a macro for genericity */
#define lock(c) pthread_mutex_lock(&(c)->lock)

/** @macro unlock a mutex with a macro for genericity */
#define unlock(c) pthread_mutex_unlock(&(c)->lock)

/** @macro Initialize a mutex with a macro for genericity. */
#define lock_init(c) pthread_mutex_init(&(c)->lock, NULL)

/** @macro Free a mutex with a macro for genericity. */
#define lock_free(c) pthread_mutex_destroy(&(c)->lock)

#endif

#ifdef __APPLE__ // Mac OS spinlock

#include <libkern/OSAtomic.h>

/** Typedef spinlock (=fast mutex) to a personalized type for portability */
typedef OSSpinLock SpinLock;

/** @macro Lock a spinlock with a macro for genericity */
#define spin_lock(c) OSSpinLockLock(&(c)->spin)

/** @macro unlock a spinlock with a macro for genericity */
#define spin_unlock(c) OSSpinLockUnlock(&(c)->spin)

/** @macro Initialize a spinlock with a macro for genericity. */
#define spin_init(c)  do {(c)->spin = OS_SPINLOCK_INIT;} while (0)

/** @macro Free a spinlock with a macro for genericity. */
#define spin_free(c)   // FIXME ?? should this stay empty ?


#elif defined(__USE_XOPEN2K) // Posix spinlock

/** Typedef spinlock (=fast mutex) to a personalized type for portability */
typedef pthread_spinlock_t SpinLock;

/** @macro Lock a spinlock with a macro for genericity */
#define spin_lock(c) pthread_spin_lock(&(c)->spin)

/** @macro unlock a spinlock with a macro for genericity */
#define spin_unlock(c) pthread_spin_unlock(&(c)->spin)

/** @macro Initialize a spinlock with a macro for genericity. */
#define spin_init(c) pthread_spin_init(&(c)->spin, PTHREAD_PROCESS_PRIVATE)

/** @macro Free a spinlock with a macro for genericity. */
#define spin_free(c) pthread_spin_destroy(&(c)->spin)

#else // No spin lock available, use mutex instead

/** Typedef spinlock (=fast mutex) to a personalized type for portability */
typedef pthread_mutex_t SpinLock;

/** @macro Lock a spinlock with a macro for genericity */
#define spin_lock(c) pthread_mutex_lock(&(c)->spin)

/** @macro unlock a mutex with a macro for genericity */
#define spin_unlock(c) pthread_mutex_unlock(&(c)->spin)

/** @macro Initialize a mutex with a macro for genericity. */
#define spin_init(c) pthread_mutex_init(&(c)->spin, NULL)

/** @macro Free a mutex with a macro for genericity. */
#define spin_free(c) pthread_mutex_destroy(&(c)->spin)

#endif

/** init a condition */
#define condition_init(c) pthread_cond_init(&(c)->cond, NULL)

/** wait for a condition change */
#define condition_wait(c) pthread_cond_wait(&(c)->cond, &(c)->lock)

/** signal a condition change */
#define condition_signal(c) pthread_cond_signal(&(c)->cond)

/** boardcast a condition change */
#define condition_broadcast(c) pthread_cond_broadcast(&(c)->cond)

/** free a condition */
#define condition_free(c) pthread_cond_destroy(&(c)->cond)

#elif defined(_WIN32)

#include <winsock2.h>
#include <windows.h>

/** Typedef to a personalized Thread type for portability */
typedef HANDLE Thread;

/** Typedef to a personalized Lock type for portability */
typedef CRITICAL_SECTION Lock;

/** Typedef to a personalized SpinLock type for portability */
typedef CRITICAL_SECTION SpinLock;

/** Some buggy compilers need the following declarations */
#if defined(_WIN64)

#ifndef _MSC_VER

typedef DWORD CONDITION_VARIABLE;
void InitializeConditionVariable(CONDITION_VARIABLE*);
void WakeConditionVariable(CONDITION_VARIABLE*);
void WakeAllConditionVariable(CONDITION_VARIABLE*);
BOOL SleepConditionVariableCS(CONDITION_VARIABLE*, CRITICAL_SECTION*, DWORD);

#endif

/** Typedef to a personalized Condition type for portability */
typedef CONDITION_VARIABLE Condition;

/** init a condition */
#define condition_init(c) InitializeConditionVariable(&(c)->cond)

/** wait for a condition change */
#define condition_wait(c) SleepConditionVariableCS(&(c)->cond, &(c)->lock, INFINITE)

/** signal a condition change */
#define condition_signal(c) WakeConditionVariable(&(c)->cond)

/** signal a condition change */
#define condition_broadcast(c) WakeAllConditionVariable(&(c)->cond)

/** free a condition */
#define condition_free(c)

#endif

/** @macro to detach a thread */
#define thread_detach(thread) CloseHandle(thread)

/** @macro Lock a spinlock with a macro for genericity */
#define lock(c) EnterCriticalSection(&(c)->lock)

/** @macro unlock a mutex with a macro for genericity */
#define unlock(c) LeaveCriticalSection(&(c)->lock)

/** @macro Initialize a mutex with a macro for genericity. */
#define lock_init(c) InitializeCriticalSection(&(c)->lock)

/** @macro Initialize a mutex with a macro for genericity. */
#define lock_free(c) DeleteCriticalSection(&(c)->lock)

/** @macro Lock a spinlock with a macro for genericity */
#define spin_lock(c) EnterCriticalSection(&(c)->spin)

/** @macro unlock a mutex with a macro for genericity */
#define spin_unlock(c) LeaveCriticalSection(&(c)->spin)

/** @macro Initialize a mutex with a macro for genericity. */
#define spin_init(c) InitializeCriticalSection(&(c)->spin)

/** @macro Initialize a mutex with a macro for genericity. */
#define spin_free(c) DeleteCriticalSection(&(c)->spin)


#endif

void thread_create(Thread*, void* (*f)(void*), void*);
void thread_join(Thread);
void thread_set_cpu(Thread, int);
Thread thread_self(void);

/** atomic addition */
static inline void atomic_add(volatile unsigned long long *value, long long i)
{
#if defined(USE_GAS_X64)
  __asm__ __volatile__("lock xaddq %1, %0":"=m"(*value) :"r"(i), "m" (*value));
#elif defined(USE_MSVC_X64)
	_InterlockedAdd64(value, i);
#else
	*value += i;
#endif
}

void cpu(void);
int get_cpu_number(void);

/*
 * Error management
 */

/**
 * @brief Display an error message as "FATAL_ERROR : file name : function name : line number : ..."
 * and abort the program.
 */
#define fatal_error(...) \
	do { \
		fprintf(stderr, "\nFATAL ERROR: %s : %s : %d : ", __FILE__, __func__, __LINE__); \
		if (errno) fprintf(stderr, "\terror #%d : %s", errno, strerror(errno)); \
		fputc('\n', stderr); \
		fprintf(stderr, __VA_ARGS__); \
		abort(); \
	} while (0)

/**
 * @brief Display an error message as "ERROR : filename : funcname : line number : ..."
 */
#define error(...) \
	do { \
		fprintf(stderr, "\nERROR: %s : %s : %d :", __FILE__, __func__, __LINE__); \
		if (errno) fprintf(stderr, " error #%d : %s", errno, strerror(errno)); \
		fputc('\n', stderr); \
		fprintf(stderr, __VA_ARGS__); \
		errno = 0; \
	} while (0)

/**
 * @brief Display a warning message as "WARNING : ... "
 */
#define warn(...) \
	do { \
		fprintf(stderr, "\nWARNING: "); \
		fprintf(stderr, __VA_ARGS__); \
	} while (0)

/**
 * @brief Display a message.
 */
#define info(...) if (options.info) { \
	fprintf(stderr, __VA_ARGS__); \
} else (void) 0

/**
 * @brief Display a message in cassio's "fenetre de rapport" .
 */
#define cassio_debug(...) if (options.debug_cassio) { \
	printf("DEBUG: "); \
	printf(__VA_ARGS__); \
	log_print(engine_log, "DEBUG: "); \
	log_print(engine_log, __VA_ARGS__);\
} else (void) 0


#ifndef NDEBUG
/**
 * @brief trace indicate if a piece of code is visited
 */
#define trace(...) do {\
	fprintf(stderr, "trace %s : %s : %d : ", __FILE__, __func__, __LINE__); \
	fprintf(stderr, __VA_ARGS__); \
} while (0)

/**
 * @brief Display a debuging message as "DEBUG : ... "
 */
#define debug(...) \
	do { \
		fprintf(stderr, "\nDEBUG : "); \
		fprintf(stderr, __VA_ARGS__); \
	} while (0)

#else
#define trace(...)
#define debug(...)
#endif

/**
 * @brief LogFile.
 */
typedef struct Log {
	FILE *f;
	Lock lock;
} Log;

/** @brief open a log file if allowed. */
#define log_open(l, file) if ((l)->f == NULL && file != NULL) { \
	lock_init(l); \
	(l)->f = fopen(file, "w"); \
} else (void) 0

/** @brief Close an opened log file. */
#define log_close(l) if ((l)->f) { \
	lock_free(l); \
	fclose((l)->f); \
	(l)->f = NULL; \
} else (void) 0

/** @brief Print into the log file */
#define log_print(l, ...) if ((l)->f) { \
	fprintf((l)->f, __VA_ARGS__); \
	fflush((l)->f); \
} else (void) 0

/** @brief Check if the log stream can be used */
#define log_is_open(l) ((l)->f != NULL)

/** @brief log a reception */
#define log_receive(l, title, ...)  if ((l)->f) { \
	fprintf((l)->f, "%s", title); \
	time_stamp((l)->f); \
	fprintf((l)->f, " <== "); \
	fprintf((l)->f, __VA_ARGS__); \
	fflush((l)->f); \
} else (void) 0

/** @brief log a sending */
#define log_send(l, title, ...)  if ((l)->f) { \
	fprintf((l)->f, "%s", title); \
	time_stamp((l)->f); \
	fprintf((l)->f, " ==> "); \
	fprintf((l)->f, __VA_ARGS__); \
	fflush((l)->f); \
} else (void) 0

#endif // EDAX_UTIL_H
