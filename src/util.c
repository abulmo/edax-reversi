/**
 * @file util.c
 *
 * @brief Various utilities.
 *
 * This should be the only file with linux/windows
 * dedicated code.
 *
 * @date 1998 - 2013
 * @author Richard Delorme
 * @version 4.3
 */

#include "bit.h"
#include "board.h"
#include "move.h"
#include "util.h"
#include "options.h"
#include "const.h"

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#if defined(__unix__) || defined(__APPLE__)

#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>

#endif // __unix__ || __APPLE__

#if defined(__linux__)

#include <sys/sysinfo.h>
#include <sched.h>

#endif // __linux__

#if defined(_WIN32)

#include <winsock2.h>
#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>

#define fileno _fileno

#ifndef S_ISREG
#define S_ISREG(x) (x & _S_IFREG)
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(x) (x & _S_IFIFO)
#endif

#endif // _WIN32

#if defined(__unix__) || defined(__APPLE__)

/**
 * @brief real_clock
 *
 * Measure wall clock time.
 * @return time in milliseconds.
 */
long long real_clock(void)
{
#if _POSIX_TIMERS > 0
	struct timespec tv;
	clock_gettime(CLOCK_MONOTONIC, &tv);
	return tv.tv_sec * 1000ULL + tv.tv_nsec / 1000000ULL;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

/**
 * @brief cpu_clock
 *
 * Measure cpu time.
 * @return time in milliseconds.
 */
long long cpu_clock(void)
{
	struct rusage u;
	getrusage(RUSAGE_SELF, &u);
	return 1000 * u.ru_utime.tv_sec + u.ru_utime.tv_usec / 1000;
}

#elif defined (_WIN32)

long long real_clock(void)
{
	return GetTickCount();
}

long long cpu_clock(void)
{
	return GetTickCount();
}

#endif

/**
 * @brief Time clock.
 *
 * Can be set as a real_clock or a cpu_clock.
 *
 * @return time in milliseconds.
 */
long long (*time_clock)(void) = real_clock;

/**
 * @brief Print time as "D:HH:MM:SS.CC".
 *
 * @param t Time in milliseconds.
 * @param justified add spaces or not before the text.
 * @param f Stream.
 */
void time_print(long long t, bool justified, FILE* f)
{
	int d, h, m, s, c;
	int sign;
	const char *space = justified ? "   " : "";


	if (t < 0) {sign = -1; t = -t;} else sign = +1;
	d = t / 86400000; t -= d * 86400000LL;
	h = t / 3600000; t -= h * 3600000;
	m = t / 60000; t -= m * 60000;
	s = t / 1000;
	c = (t - s * 1000);

	if (d) fprintf(f, "%2d:%02d:%02d:%02d.%03d", sign*d, h, m, s, c);
	else if (h) fprintf(f, "%s%2d:%02d:%02d.%03d", space, sign*h, m, s, c);
	else fprintf(f, "%s%s%2d:%02d.%03d", space, space, sign*m, s, c);
}

/**
 * @brief read time as "D:HH:MM:SS.C".
 *
 * @param f Input stream.
 * @return time in milliseconds.
 */
long long time_read(FILE *f)
{
	long long t = 0;
	int n, c;

	while ((c = getc(f)) != EOF && isspace(c)) ; ungetc(c, f);
	n = 0; while (isdigit(c = getc(f))) n = n * 10 + (c - '0');
	if (c == ':') {
		t = 60 * n; //  time has the form MM:SS ?
		n = 0; while (isdigit(c = getc(f))) n = n * 10 + (c - '0');
		if (c == ':') {
			t = 60 * (t + n); // 	time has the form HH:MM:SS ?
			n = 0; while (isdigit(c = getc(f))) n = n * 10 + (c - '0');
			if (c == ':') {
				t = 24 * (t + n); // 	time has the form D:HH:MM:SS ?
				n = 0; while (isdigit(c = getc(f))) n = n * 10 + (c - '0');
			}
		}
	}

	t = (t + n) * 1000;
	if (c != '.') return t;
	n = 0; while (isdigit(c = getc(f))) n = n * 10 + (c - '0');
	while (n > 1000) n /= 10;
	t += n;
	return t;
}

/**
 * @brief Print local time.
 * @param f Output stream.
 */
void time_stamp(FILE *f)
{
	time_t t = time(NULL);
	struct tm *tm;
	tm = localtime(&t);

	fprintf(f, "[%4d/%2d/%2d %2d:%2d:%2d] ", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
}


/**
 * @brief sleep for t ms.
 * @param t time in ms.
 */
void relax(int t)
{
#if defined(__unix__) || defined(__APPLE__)
	struct timespec ts;
	ts.tv_sec = t / 1000;
	ts.tv_nsec = (t % 1000) * 1000000;
	nanosleep(&ts, NULL);
#elif defined(_WIN32)
	Sleep(t);
#endif
}

/**
 * @brief Format a value with a unit.
 *
 * @param v Value to print.
 * @param unit Unit.
 * @param f Output string.
 */
char* format_scientific(double v, const char *unit, char *f)
{
	static const char *multiple = "EPTGMk mµnpfa"; //
	int u;

	if (fabs(v) < 1e-24) {
		u = 0;
	} else {
		u = floor(log10(fabs(v)) / 3.0);
		if (u > 6) u = 6; else if (u < -6) u = -6;
		v /= pow(10, 3 * u);
	}
	
	if (fabs(v) - floor(fabs(v)) < 0.01) sprintf(f, " %5.1f %c%s", v, multiple[6 - u], unit);
	else if (fabs(v + 0.05) < 10.0) sprintf(f, " %5.3f  %c%s", v, multiple[6 - u], unit);
	else if (fabs(v + 0.5) < 100.0) sprintf(f, " %5.2f  %c%s", v, multiple[6 - u], unit);
	else sprintf(f, " %5.1f  %c%s", v, multiple[6 - u], unit);

	return f;
}

/**
 * @brief Print a value with a unit.
 *
 * @param v Value to print.
 * @param unit Unit.
 * @param f Output stream.
 */
void print_scientific(double v, const char *unit, FILE *f)
{
	char s[32];
	fprintf(f, "%s", format_scientific(v, unit, s));
}


/**
 * @brief Read a line.
 *
 * Read a line from an input string. The needed memory is allocated.
 *
 * @param f Input stream.
 * @return NULL if the line is empty, the read line otherwise.
 */
char* string_read_line(FILE *f)
{
	int c, i, n;
	char *line;

	n = 256; i = 0;
	line = (char*) malloc(n);
	if (line == NULL) fatal_error("Allocation error\n");

	while ((c = getc(f)) != EOF && c != '\n') {
		line[i++] = (char) c;
		if (i == n) {
			n *= 2;
			line = (char*) realloc(line, n);
			if (line == NULL) fatal_error("Allocation error\n");
		}
	}
	line[i] = '\0';

	if (i == 0 && c == EOF) {
		free(line);
		line = NULL;
	}

	return line;
}


/**
 * @brief Duplicate a string.
 *
 * @param s string to duplicate.
 * @return a copy of the string.
 */
char* string_duplicate(const char *s)
{
	char *d;

	if (s) {
		const int n = strlen(s) + 1;
		d = (char*) malloc(n);
		if (d) memcpy(d, s, n);
	} else {
		d = NULL;
	}

	return d;
}

/**
 * @brief Read time as "D:HH:MM:SS.C".
 *
 * @param string Time as a string.
 * @return Time in milliseconds.
 */
long long string_to_time(const char *string)
{
	long long t = 0;
	int n = 0;
	double x = 0.0;

	if (string) {
		const char *s = parse_skip_spaces(string);
		s = parse_int(s, &n);
		if (*s == ':') {
			t = 60 * n; //  time has the form MM:SS ?
			s = parse_int(s + 1, &n);
			if (*s == ':') {
				t = 60 * (t + n); // 	time has the form HH:MM:SS ?
				s = parse_int(s + 1, &n);
				if (*s == ':') {
					t = 24 * (t + n); // 	time has the form D:HH:MM:SS ?
					s = parse_int(s + 1, &n);
				}
			}
		}
		if (*s == '.' && isdigit(s[1])) {
			s = parse_real(s, &x);
		}
		t = (t + n + x) * 1000;
	}

	return t;
}

/**
 * @brief Change all char of a string to lowercase.
 *
 * @param s string.
 */
void string_to_lowercase(char *s)
{
	if (s) {
		do {
			*s = tolower(*s);
		} while (*s++);
	}
}

/**
 * @brief Change all char of a string to uppercase.
 *
 * @param s Input string.
 */
void string_to_uppercase(char *s)
{
	if (s) {
		do {
			*s = toupper(*s);
		} while (*s++);
	}
}

/**
 * @brief Convert the two first chars of a string into a coordinate.
 *
 * @param s String.
 * @return coordinate value (A1 to H8 or PASS or NOMOVE).
 */
int string_to_coordinate(const char *s)
{
	int x = NOMOVE;

	if (s && *s) {
		if (tolower(s[0]) == 'p' && s[1] == '@') s += 2;
		else if (s[0] == '@') ++s;

		if (tolower(s[0]) == 'p' && (tolower(s[1]) == 'a' || tolower(s[1]) == 's')) x = PASS;
		else if (s[0] == '@' && s[1] == '@') x = PASS; // xboard pass = "@@@@"
		else {
			int c = tolower(s[0]) - 'a';
			int r = s[1] - '1';
			if (0 <= c && c <= 7 && 0 <= r && r <= 7) x = r * 8 + c;
		}
	}

	return x;
}

/**
 * @brief remove spaces from a string.
 *
 * @param s string.
 * @return a word.
 */

char* string_to_word(char *s)
{
	char *w = NULL;

	if (s) {
		while (*s && isspace(*s++)) ;
		w = --s;
		while (*s && !isspace(*s++)) ;
		*s = '\0';
	}
	return w;
}

/**
 * @brief Convert a string into a boolean.
 *
 * Parse a string into a boolean (false/true).
 *
 * @param s String.
 * @return a word.
 */
bool string_to_boolean(const char *s)
{
	if (strcmp(s, "false") == 0
	||  strcmp(s, "off") == 0
	||  strcmp(s, "no") == 0
	||  strcmp(s, "0") == 0) return false;

	if (strcmp(s, "true") == 0
	||  strcmp(s, "on") == 0
	||  strcmp(s, "yes") == 0
	||  strcmp(s, "1") == 0) return true;

	errno = EINVAL;
	return false;
}

/**
 * @brief Convert a string into an integer.
 *
 * Parse a string into an int.
 *
 * @param s String.
 * @param default_value default value if parsing failed.
 * @return an integer.
 */
int string_to_int(const char *s, const int default_value)
{
	char *end = (char*) s;
	long n = 0;

	if (s) n = strtol(s, &end, 10);

	if (end == s) {
		errno = EINVAL;
		n = default_value;
	}
	if (n < INT_MIN) {
		n = INT_MIN;
		errno = ERANGE;
	} else if (n > INT_MAX) {
		n = INT_MAX;
		errno = ERANGE;
	}

	return (int) n;
}

/**
 * @brief Convert a string into a real number.
 *
 * Parse a string into a double.
 *
 * @param s String.
 * @param default_value default value if parsing failed.
 * @return a real number.
 */
double string_to_real(const char *s, const double default_value)
{
	char *end = (char*) s;
	double x = 0.0;

	if (s) x = strtod(s, &end);

	if (end == s) {
		errno = EINVAL;
		x = default_value;
	}

	return x;
}

/* Parsing functions
 * They differs from string conversion as they convert a
 * string but return the next position from that string.
 */

/**
 * @brief Skip spaces.
 *
 * @param string String to parse.
 * @return The remaining of the input string.
 */
char* parse_skip_spaces(const char *string)
{
	if (string) {
		while (*string && isspace(*string)) ++string;
	}
	return (char*) string;
}

/**
 * @brief Skip word.
 *
 * @param string String to parse.
 * @return The remaining of the input string.
 */
char* parse_skip_word(const char *string)
{
	if (string) {
		while (*string && isspace(*string)) ++string;
		while (*string && !isspace(*string)) ++string;
		while (*string && isspace(*string)) ++string;
	}
	return (char*) string;
}

/**
 * @brief Find a char.
 *
 * @param string String to parse.
 * @param c Seeked char.
 * @return The input string at the char position or an empty string.
 */
char* parse_find(const char *string, const int c)
{
	if (string) {
		while (*string && *string != c) ++string;
	}
	return (char*) string;
}

/**
 * @brief Parse a word.
 *
 * @param string String to parse
 * @param word String receiving a copy of the parsed word.
 * @param n word string capacity.
 * NOTE: It is assumed that w is big enough to contains the word copy.
 * @return The remaining of the input string.
 */
char* parse_word(const char *string, char *word, unsigned int n)
 {
	if (string) {
		string = parse_skip_spaces(string);
		while (*string && !isspace(*string) && n--) *word++ = *string++;
		*word = '\0';
	}
	return (char*) string;
}

/**
 * @brief Parse a field.
 *
 * @param string String to parse
 * @param word String receiving a copy of the parsed word.
 * @param n word string capacity.
 * @param separator field separator.
 * NOTE: It is assumed that w is big enough to contains the word copy.
 * @return The remaining of the input string.
 */
char* parse_field(const char *string, char *word, unsigned int n, char separator)
 {
	if (string) {
		string = parse_skip_spaces(string);
		while (*string && *string != separator && n--) *word++ = *string++;
		if (*string == separator) ++string;
		*word = '\0';
	}
	return (char*) string;
}

/**
 * @brief Parse a line.
 *
 * @param string string to parse
 * @param line string receiving a copy of the parsed line.
 * @param n line string capacity.
 * NOTE: It is assumed that line is big enough to contains 'n' characters.
 * if the line to be parsed contains more than 'n' characters, only 'n' caracters will be
 * copied, but the whole line will be parsed.
 * @return The remaining of the input string until the next new line char.
 */
char* parse_line(const char *string, char *line, unsigned int n)
{
	const char *s = string;
	if (s) {
		while (*s &&  *s != '\n' && *s != '\r' && n--) *line++ = *s++;
		if (*s == '\0') s = string;
		else {
			while (*s && *s != '\n' && *s != '\r' ) ++s;
			while (*s == '\r' || *s == '\n') ++s;
		}
	}
	*line = '\0';
	return (char*) s;
}

/**
 * @brief Parse a move.
 *
 * @param string String to parse
 * @param board Board where to play the move.
 * @param move A move.
 * @return The remaining of the input string.
 */
char* parse_move(const char *string, const Board *board, Move *move)
{

	*move = MOVE_INIT;

	if (string) {
		char *word = parse_skip_spaces(string);
		int x = string_to_coordinate(word);
		move->x = x;
		move->flipped = flip[x](board->player, board->opponent);
		if ((x == PASS && board_is_pass(board)) || (move->flipped && !board_is_occupied(board, x))) return word + 2;
		else if (board_is_pass(board)) {
			move->x = PASS;
			move->flipped = 0;
		} else {
			move->x = NOMOVE;
			move->flipped = 0;
		}
	}
	return (char*) string;
}

/**
 * @brief Parse a sequence of moves.
 *
 * @param string String to parse.
 * @param board_init Initial board from where to play the moves.
 * @param line A move sequence.
 * @return The remaining of the input string.
 */
char* parse_game(const char *string, const Board *board_init, Line *line)
{
	const char *next;
	Board board[1];
	Move move[1];

	*board = *board_init;

	while ((next = parse_move(string, board, move)) != string || move->x == PASS) {
		line_push(line, move->x);
		board_update(board, move);
		string = next;
	}

	return (char *) string;
}

/**
 * @brief Parse a board.
 *
 * @param string String to parse
 * @param board Board.
 * @param player Player to move color.
 * @return The remaining of the input string.
 */
char* parse_board(const char *string, Board *board, int *player)
{
	if (string) {
		int i;
		const char *s = parse_skip_spaces(string);

		board->player = board->opponent = 0;
		for (i = A1; i <= H8; ++i) {
			if (*s == '\0') return (char*) string;
			switch (tolower(*s)) {
			case 'b':
			case 'x':
			case '*':
				board->player |= x_to_bit(i);
				break;
			case 'o':
			case 'w':
				board->opponent |= x_to_bit(i);
				break;
			case '-':
			case '.':
				break;
			default:
				if (!isspace(*s)) return (char*) string;
				i--;
				break;
			}
			++s;
		}
		board_check(board);

		for (;*s; ++s) {
			switch (tolower(*s)) {
			case 'b':
			case 'x':
			case '*':
				*player = BLACK;
				return (char*) s + 1;
			case 'o':
			case 'w':
				board_swap_players(board);
				*player = WHITE;
				return (char*) s + 1;
			default:
				break;
			}
		}
	}

	return (char*) string;
}

/**
 * @brief Parse a boolean.
 *
 * @param string String to parse
 * @param result Boolean output value.
 * @return The remaining of the input string.
 */
char* parse_boolean(const char *string, bool *result)
{
	if (string) {
		char word[6];
		bool r;
		errno = 0;
		string = parse_word(parse_skip_spaces(string), word, 6);
		r = string_to_boolean(word);
		if (errno != EINVAL) *result = r;
	}
	return (char*) string;
}

/**
 * @brief Parse an integer.
 *
 * @param string String to parse
 * @param result Integer output value.
 * @return The remaining of the input string.
 */
char* parse_int(const char *string, int *result)
{
	if (string) {
		char *s = parse_skip_spaces(string);
		char *end = s;
		long long n = 0;

		if (s) n = strtol(s, &end, 10);

		if (end == s) {
			errno = EINVAL;
			n = *result;
		}
		if (n < INT_MIN) {
			n = INT_MIN;
			errno = ERANGE;
		} else if (n > INT_MAX) {
			n = INT_MAX;
			errno = ERANGE;
		}

		*result = n;

		return end;
	}
	return (char*) string;
}

/**
 * @brief Parse a real number (as a double floating point).
 *
 * @param string String to parse.
 * @param result Real number (double) output value.
 * @return The remaining of the input string.
 */
char* parse_real(const char *string, double *result)
{
	if (string) {
		char *s = parse_skip_spaces(string);
		char *end = s;
		double d = 0;

		if (s) d = strtod(s, &end);

		if (end == s) {
			errno = EINVAL;
			d = *result;
		}

		*result = d;

		return end;
	}
	return (char*) string;
}

/**
 * @brief parse time as "D:HH:MM:SS.C".
 *
 * @param string Time as a string.
 * @param t Output time in milliseconds.
 * @return The remaining of the input string.
 */
char* parse_time(const char *string, long long *t)
{
	int n = 0;
	double x = 0.0;

	*t = 0;
	
	if (string) {
		const char *s = parse_skip_spaces(string);
		s = parse_int(s, &n);
		if (*s == ':') {
			*t = 60 * n; //  time has the form MM:SS ?
			s = parse_int(s + 1, &n);
			if (*s == ':') {
				*t = 60 * (*t + n); // 	time has the form HH:MM:SS ?
				s = parse_int(s + 1, &n);
				if (*s == ':') {
					*t = 24 * (*t + n); // 	time has the form D:HH:MM:SS ?
					s = parse_int(s + 1, &n);
				}
			}
		}
		if (*s == '.' && isdigit(s[1])) {
			s = parse_real(s, &x);
			assert(0.0 <= x && x < 1.0);
		}
		*t = (*t + n + x) * 1000;
		if (errno != EINVAL && errno != ERANGE) string = s;
	}

	return (char*) string;
}


/**
 * @brief Parse a command.
 *
 * @param string String to parse.
 * @param cmd Output command string (first word of the input string).
 * @param param Output parameters (remaining of the line).
 * @param size cmd string capacity.
 * @return The remaining of the input string (next line).
 */
char* parse_command(const char *string, char *cmd, char *param, const unsigned int size)
{
	string = parse_word(string, cmd, size);
	string_to_lowercase(cmd);
	if (strcmp(cmd, "set") == 0) {
		string = parse_word(string, cmd, size);
		string_to_lowercase(cmd);
	}
	string = parse_skip_spaces(string);
	if (*string == '=') string = parse_skip_spaces(string + 1);
	string = parse_line(string, param, size);

	return (char*) string;
}

/**
 * @brief Extract the directory of a file path.
 *
 * @param path Full path.
 * @param dir directory part of the path.
 */
void path_get_dir(const char *path, char *dir)
{
	const char *c;

	for (c = path; *c; ++c) ;
	for (;c >= path && *c != '/'; --c) ;

	while (path <= c) *dir++ = *path++;
	*dir = '\0';
}

/**
 * @brief Add an extension to a string.
 * 
 * @param base Base name.
 * @param ext  Extension (.dat, .ext, .bin, etc.)
 * @param file Output file name.
 * @return The output file name.
 */
char* file_add_ext(const char *base, const char *ext, char *file)
{
	while (*base) *file++ = *base++;
	while (*ext) *file++ = *ext++;
	*file = '\0';
	return file;
}

/**
 * @brief Create a thread.
 *
 * @param thread Thread.
 * @param function Function to run in parallel.
 * @param data Data for the function.
 */
void thread_create(Thread *thread, void* (*function)(void*), void *data)
{
#if defined(__unix__) || (defined(_WIN32) && defined(USE_PTHREAD)) || defined(__APPLE__)
	pthread_create(thread, NULL, function, data);
#elif defined(_WIN32)
	DWORD id;
	*thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) function, data, 0, &id);
#endif
}

/**
 * @brief Join a thread.
 *
 * Wait for the thread termination. this function also frees the resources
 * allocated by a thread that has not been detached.
 *
 * @param thread Thread.
 */
void thread_join(Thread thread)
{
#if defined(__unix__) || (defined(_WIN32) && defined(USE_PTHREAD)) || defined(__APPLE__)
	pthread_join(thread, NULL);
#elif defined(_WIN32)
	WaitForSingleObject(thread, INFINITE);
	CloseHandle(thread);
#endif
}
/**
 * @brief Current thread.
 *
 * @return current thread.
 */
Thread thread_self(void)
{
#if defined(__unix__) || (defined(_WIN32) && defined(USE_PTHREAD)) || defined(__APPLE__)
	return pthread_self();
#elif defined(_WIN32)
	return GetCurrentThread();
#endif
}

/**
 * @brief Choose a single core or cpu to run on, under linux systems, to avoid
 * context changes
 */
void thread_set_cpu(Thread thread, int i)
{
#if defined(__linux__) && !defined(ANDROID)
	cpu_set_t cpu;

	CPU_ZERO(&cpu);
	CPU_SET(i, &cpu);
	pthread_setaffinity_np(thread, sizeof (cpu_set_t), &cpu);
#elif defined(_WIN32) && !defined(USE_PTHREAD)
	SetThreadIdealProcessor(thread, i);
#else
	(void) thread; (void) i;
#endif
}


/**
 * @brief Get the number of cpus or cores on the machine.
 * @return Cpu/Core number
 */
int get_cpu_number(void)
{
	int n = 0;

#if defined(ANDROID)
	char file[64];
	FILE *f;

	for (n = 0; n < MAX_THREADS; ++n) {
		sprintf(file, "/sys/devices/system/cpu/cpu%d", n);
		f = fopen(file, "r");
		if (f == NULL) {
			break;
		}
		fclose(f);
	}

#elif defined(_SC_NPROCESSORS_ONLN)

	n = sysconf(_SC_NPROCESSORS_ONLN);

#elif defined(_WIN32)

	SYSTEM_INFO info;

	GetSystemInfo(&info);
	n = info.dwNumberOfProcessors;

#elif defined(__APPLE__) /* should also works on any bsd system */

	int mib[4];
	size_t len; 

	mib[0] = CTL_HW;
	mib[1] = HW_AVAILCPU;
	sysctl(mib, 2, &n, &len, NULL, 0);
	if (n < 1) {
	     mib[1] = HW_NCPU;
	     sysctl( mib, 2, &n, &len, NULL, 0 );
	}

#endif

	if (n < 1) n = 1;

	return n;
}

/**
 * @brief Pseudo-random number generator.
 *
 * A good pseudo-random generator (derived from rand48 or Java's one) to set the
 * hash codes.
 * @param random Pseudo-Random generator state.
 * @return a 64-bits pseudo-random unsigned int integer.
 */
unsigned long long random_get(Random *random)
{
	const unsigned long long MASK48 = 0xFFFFFFFFFFFFull;
	const unsigned long long A = 0x5DEECE66Dull;
	const unsigned long long B = 0xBull;
	register unsigned long long r;

	random->x = ((A * random->x + B) & MASK48);
	r = random->x >> 16;
	random->x = ((A * random->x + B) & MASK48);
	return (r << 32) | (random->x >> 16);
}

/**
 * @brief Pseudo-random number seed.
 *
 * @param random Pseudo-Random generator state.
 * @param seed a 64-bits integer used as seed.
 */
void random_seed(Random *random, const unsigned long long seed)
{
	const unsigned long long MASK48 = 0xFFFFFFFFFFFFull;
	random->x = (seed & MASK48);
}
