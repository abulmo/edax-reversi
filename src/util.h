/**
 * @file util.h
 *
 * @brief Miscellaneous utilities header.
 *
 * @date 1998 - 2024
 * @author Richard Delorme
 * @version 4.6
 */

#ifndef EDAX_UTIL_H
#define EDAX_UTIL_H

#include <errno.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct Board;
struct Move;
struct Line;

/*
 * Memory
 */
size_t adjust_size(const size_t, const size_t);
#if defined __APPLE__ && __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 101300

void* aligned_alloc(size_t, size_t);

#elif defined(_MSC_VER)

#include <malloc.h>

#define aligned_alloc(alignment, size) _aligned_malloc((size), (alignment))
#define free _aligned_free
#define malloc(size) _aligned_malloc((size), 16)
#define realloc(ptr, size) _aligned_realloc((ptr), (size), 16)

#endif

/*
 * Time management
 */
int64_t real_clock(void);
int64_t cpu_clock(void);
void time_print(int64_t, bool, FILE*);
int64_t time_read(FILE*);
void time_stamp(FILE*);
void relax(int);
void rest(void);

/*
 * stdc thread
 */
#ifdef __STDC_NO_THREADS__

#include <pthread.h>

typedef pthread_t thrd_t;
enum {
	thrd_success  = 0,
	thrd_busy     = 1,
	thrd_error    = 2,
	thrd_nomem    = 3,
	thrd_timedout = 4,
};
typedef int (*thrd_start_t)(void *);
int thrd_create (thrd_t *thr, thrd_start_t, void*);
int thrd_detach(thrd_t);
int	thrd_join(thrd_t, int*);

typedef pthread_mutex_t mtx_t;
enum {
	mtx_plain     = 0,
	mtx_recursive = 1,
	mtx_timed     = 2,
};
int mtx_init(mtx_t*, int);
void mtx_destroy(mtx_t*);
int mtx_lock(mtx_t*);
int mtx_unlock(mtx_t*);

typedef pthread_cond_t cnd_t;
int cnd_init(cnd_t *);
void cnd_destroy(cnd_t *);
int cnd_broadcast(cnd_t *);
int cnd_signal(cnd_t *);
int cnd_timedwait(cnd_t *__restrict, mtx_t *__restrict, const struct timespec *__restrict);
int cnd_wait(cnd_t *, mtx_t *);

#else
	#include <threads.h>
#endif

/*
 * spin lock
 */
typedef struct SpinLock {
	int _Atomic lock;
} SpinLock;
void spinlock_init(SpinLock*);
void spinlock_lock(SpinLock*);
void spinlock_unlock(SpinLock*);

/*
 * Special printing function
 */
char* format_scientific(double, const char*, char*);
void print_scientific(double, const char*, FILE*);

/*
 * Strings
 */
char* string_duplicate(const char*);
int64_t string_to_time(const char*);
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
	uint64_t x;
} Random;

uint64_t random_get(Random*);
void random_seed(Random*, const uint64_t);

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
		if (name) fprintf(stderr, "\nWARNING: %s = %" PRId64 " is out of range. Set to %" PRId64 "\n", name, (int64_t)var, (int64_t)min);\
		var = min;\
	} else if (var > max) {\
		if (name) fprintf(stderr, "\nWARNING: %s = %" PRId64 "is out of range. Set to %" PRId64 "\n", name, (int64_t)var, (int64_t)max);\
		var = max;\
	}\
} while (0)


void cpu(void);
int get_cpu_number(void);

/**
 * @brief LogFile.
 */
typedef struct Log {
	FILE *f;
	mtx_t mutex;
} Log;

/** @brief open a log file if allowed. */
#define log_open(l, file) if ((l)->f == NULL && file != NULL) { \
	mtx_init(&(l)->mutex, mtx_plain); \
	(l)->f = fopen(file, "w"); \
} else (void) 0

/** @brief Close an opened log file. */
#define log_close(l) if ((l)->f) { \
	mtx_destroy(&(l)->mutex); \
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
	fprintf((l)->f, " >>> "); \
	fprintf((l)->f, __VA_ARGS__); \
	fflush((l)->f); \
} else (void) 0

/** @brief log a sending */
#define log_send(l, title, ...)  if ((l)->f) { \
	fprintf((l)->f, "%s", title); \
	time_stamp((l)->f); \
	fprintf((l)->f, " <<< "); \
	fprintf((l)->f, __VA_ARGS__); \
	fflush((l)->f); \
} else (void) 0

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
	extern Log ggs_log; \
	fprintf(stderr, __VA_ARGS__); \
} else (void) 0

/**
 * @brief Display a message in cassio's "fenetre de rapport" .
 */
#define cassio_debug(...) if (options.debug_cassio) { \
	extern Log engine_log; \
	printf("DEBUG: "); \
	printf(__VA_ARGS__); \
	log_print(&engine_log, "DEBUG: "); \
	log_print(&engine_log, __VA_ARGS__);\
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
#define debug(...) do { \
	fprintf(stderr, "\nDEBUG : "); \
	fprintf(stderr, __VA_ARGS__); \
} while (0)

#else
#define trace(...)
#define debug(...)
#endif

/**
 * @brief test a equality
 */
#define expect_eq(a, b, msg) if (a != b) { \
	fprintf(stderr, "expectation failed at file: %s, function: %s, line: %d :", __FILE__, __func__, __LINE__); \
	fprintf(stderr, "0x%08lx != 0x%08lx - %s\n", (long) (a), (long) (b), msg); \
}

#endif // EDAX_UTIL_H
