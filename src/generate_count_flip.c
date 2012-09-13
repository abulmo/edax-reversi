/*
 * Generate magic number to compute the number of flips of the last move
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>

typedef unsigned long long uint64;
typedef unsigned char uint8;

// usefull constant
enum {
	A1, B1, C1, D1, E1, F1, G1, H1,
	A2, B2, C2, D2, E2, F2, G2, H2,
	A3, B3, C3, D3, E3, F3, G3, H3,
	A4, B4, C4, D4, E4, F4, G4, H4,
	A5, B5, C5, D5, E5, F5, G5, H5,
	A6, B6, C6, D6, E6, F6, G6, H6,
	A7, B7, C7, D7, E7, F7, G7, H7,
	A8, B8, C8, D8, E8, F8, G8, H8, PASS
};

const uint64 BIT[64] = {
	0x0000000000000001ULL, 0x0000000000000002ULL, 0x0000000000000004ULL, 0x0000000000000008ULL,
	0x0000000000000010ULL, 0x0000000000000020ULL, 0x0000000000000040ULL, 0x0000000000000080ULL,
	0x0000000000000100ULL, 0x0000000000000200ULL, 0x0000000000000400ULL, 0x0000000000000800ULL,
	0x0000000000001000ULL, 0x0000000000002000ULL, 0x0000000000004000ULL, 0x0000000000008000ULL,
	0x0000000000010000ULL, 0x0000000000020000ULL, 0x0000000000040000ULL, 0x0000000000080000ULL,
	0x0000000000100000ULL, 0x0000000000200000ULL, 0x0000000000400000ULL, 0x0000000000800000ULL,
	0x0000000001000000ULL, 0x0000000002000000ULL, 0x0000000004000000ULL, 0x0000000008000000ULL,
	0x0000000010000000ULL, 0x0000000020000000ULL, 0x0000000040000000ULL, 0x0000000080000000ULL,
	0x0000000100000000ULL, 0x0000000200000000ULL, 0x0000000400000000ULL, 0x0000000800000000ULL,
	0x0000001000000000ULL, 0x0000002000000000ULL, 0x0000004000000000ULL, 0x0000008000000000ULL,
	0x0000010000000000ULL, 0x0000020000000000ULL, 0x0000040000000000ULL, 0x0000080000000000ULL,
	0x0000100000000000ULL, 0x0000200000000000ULL, 0x0000400000000000ULL, 0x0000800000000000ULL,
	0x0001000000000000ULL, 0x0002000000000000ULL, 0x0004000000000000ULL, 0x0008000000000000ULL,
	0x0010000000000000ULL, 0x0020000000000000ULL, 0x0040000000000000ULL, 0x0080000000000000ULL,
	0x0100000000000000ULL, 0x0200000000000000ULL, 0x0400000000000000ULL, 0x0800000000000000ULL,
	0x1000000000000000ULL, 0x2000000000000000ULL, 0x4000000000000000ULL, 0x8000000000000000ULL,
};

/* compute the first bit set to one in a 64-bit integer */
int first_one(uint64 b) {
	const int magic_first_one[64]={
		63, 0, 58, 1, 59, 47, 53, 2,
		60, 39, 48, 27, 54, 33, 42, 3,
		61, 51, 37, 40, 49, 18, 28, 20,
		55, 30, 34, 11, 43, 14, 22, 4,
		62, 57, 46, 52, 38, 26, 32, 41,
		50, 36, 17, 19, 29, 10, 13, 21,
		56, 45, 25, 31, 35, 16, 9, 12,
		44, 24, 15, 8, 23, 7, 6, 5
	};
	
	b &= (-b);
	return magic_first_one[(b * 0x07EDD5E59A4E28C2ULL) >> 58];
}

/* clear the first bit set and search the next one */
static inline int next_one(uint64 *b, int x) {
	*b ^= (1ULL << x);
	return first_one(*b);
}

#define foreach_bits(i, b) for (i = first_one(b); b; i = next_one(&b, i))

/* count the number of bits set */
static inline int count(uint64 b) {
	b -=  (b >> 1) & 0x5555555555555555ULL;
	b  = ((b >> 2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
	b  = ((b >>4) + b) & 0x0f0f0f0f0f0f0f0fULL;
	b *= 0x0101010101010101ULL;

	return  (int)(b >> 56);
}

/* othello: flip count of the last move */

/* get a player disc bitboard included in mask */
uint64 get_P(uint64 mask, const uint64 flat) {
	int i, n = 0, bit_i[64];

	uint64 ret = 0;
	foreach_bits(i, mask) bit_i[n++] = i;
	for (i = 0; i < n; i++)
		if (flat & (1ULL << i)) ret |= (1ULL << bit_i[i]);

	return ret;
}


void print_bitboard(uint64 b, FILE *f) {
	char s[] = "01?";
	int i, bit;
	for (i = 0; i < 64; ++i) {
		if (b & (1ULL << i)) bit = 1; else bit = 0;
		putc(s[bit], f);
	}
	fflush(f);
}

void print_bitline(uint8 l, FILE *f) {
	char s[] = "01?";
	int i, bit;
	for (i = 0; i < 8; ++i) {
		if (l & (1ULL << i)) bit = 1; else bit = 0;
		putc(s[bit], f);
	}
	fflush(f);
}

uint64 h_mask(int x) {
	uint64 m = 0;
	int h = x / 8;
	int i;

	for (i = 0; i < 8; ++i) m |= (1ULL << (h * 8 + i));

	return m;
}

uint64 v_mask(int x) {
	uint64 m = 0;
	int v = x % 8;
	int i;

	for (i = 0; i < 8; ++i) m |= (1ULL << (i * 8 + v));
	
	return m;
}
	
uint64 d7_mask(int x) {
	uint64 m = 0;
	int r = x / 8;
	int c = x % 8;
	int i, j;

	for (i = r, j = c; i < 8 && j >= 0; ++i, --j) m |= (1ULL << (i * 8 + j));
	for (i = r - 1, j = c + 1; i >= 0 && j < 8; --i, ++j) m |= (1ULL << (i * 8 + j));

	return m;
}

uint64 d9_mask(int x) {
	uint64 m = 0;
	int r = x / 8;
	int c = x % 8;
	int i, j;

	for (i = r, j = c; i >= 0 && j >= 0; --i, --j) m |= (1ULL << (i * 8 + j));
	for (i = r + 1, j = c + 1; i < 8 && j < 8; ++i, ++j) m |= (1ULL << (i * 8 + j));

	return m;
}

uint64 d_mask(int x) {
	uint64 m = d7_mask(x) | d9_mask(x);
	int r = x / 8;
	int c = x % 8;
	if (r == 1) m &= 0xffffffffffffff00ULL;
	if (r == 6) m &= 0x00ffffffffffffffULL;
	if (c == 1) m &= 0xfefefefefefefefeULL;
	if (c == 6) m &= 0x7f7f7f7f7f7f7f7fULL;
	return m;
}

uint64 d_add(int x) {
	return (0x8080808080808080ULL - d_mask(x)) & 0x7f7f7f7f7f7f7f7fULL;
}

uint8 h_to_line(uint64 b, int x) {
	int h = x / 8;

	 return (b >> (8 * h)) & 0xff;
}

uint8 v_to_line(uint64 b, int x) {
	int v = x % 8;
	
	 return ((b & (v_mask(x))) * (0x0102040810204080ULL >> v)) >> 56;
}

uint8 d7_to_line(uint64 b, int x) {
	 return ((b & d7_mask(x)) * (0x0101010101010101ULL)) >> 56;
}

uint8 d9_to_line(uint64 b, int x) {
	 return ((b & d9_mask(x)) * (0x0101010101010101ULL)) >> 56;
}

uint8 d_to_line(uint64 b, int x) {
	 return ((b & d_mask(x)) * (0x0101010101010101ULL)) >> 56;
}

int h_flip_index(int x) {
	return first_one(h_to_line(1ULL << x, x));
}

int v_flip_index(int x) {
	return first_one(v_to_line(1ULL << x, x));
}

int d7_flip_index(int x) {
	return first_one(d7_to_line(1ULL << x, x));
}

int d_flip_index(int x) {
	return first_one(d_to_line(1ULL << x, x));
}


int d9_flip_index(int x) {
	return first_one(d9_to_line(1ULL << x, x));
}

int get_flip_count(uint8 l, int x) {
	int y;
	int i, n = 0;

	for (y = x - 1, i = 0; y >= 0 && !(l & (1 << y)); --y, ++i);
	if (y >= 0) n += i;
	for (y = x + 1, i = 0; y < 8 && !(l & (1 << y)); ++y, ++i);
	if (y < 8) n += i;

	return n;
}

/* othello: flip count of the last move */
int count_flip_h(const uint64 P, int x) {
	int i, n = 0;
	uint64 b, O;

	O = (~P & h_mask(x) & ~(1ULL << x));
	b = (1ULL << x); i = 0;
	while ((b <<= 1) & O) ++i;
	if (b & P) n += i;
	b = (1ULL << x); i = 0;
	while ((b >>= 1) & O) ++i;
	if (b & P) n += i;

	return n;
}

int count_flip_v(const uint64 P, int x) {
	int i, n = 0;
	uint64 b, O;

	O = (~P & v_mask(x) & ~(1ULL << x));
	b = (1ULL << x); i = 0;
	while ((b <<= 8) & O) ++i;
	if (b & P) n += i;
	b = (1ULL << x); i = 0;
	while ((b >>= 8) & O) ++i;
	if (b & P) n += i;

	return n;
}

int count_flip_d7(const uint64 P, int x) {
	int i, n = 0;
	uint64 b, O;

	O = (~P & d7_mask(x) & ~(1ULL << x));
	b = (1ULL << x); i = 0;
	while ((b <<= 7) & O) ++i;
	if (b & P) n += i;
	b = (1ULL << x); i = 0;
	while ((b >>= 7) & O) ++i;
	if (b & P) n += i;

	return n;
}

int count_flip_d9(const uint64 P, int x) {
	int i, n = 0;
	uint64 b, O;

	O = (~P & d9_mask(x) & ~(1ULL << x));
	b = (1ULL << x); i = 0;
	while ((b <<= 9) & O) ++i;
	if (b & P) n += i;
	b = (1ULL << x); i = 0;
	while ((b >>= 9) & O) ++i;
	if (b & P) n += i;

	return n;
}

int count_flip(const uint64 P, int x) {
	return count_flip_h(P, x) + count_flip_v(P, x) + count_flip_d7(P, x) + count_flip_d9(P, x);
}

void check(int x) {
	int i, n;
	uint64 P, mask;
	

	mask = h_mask(x) & ~(1ULL << x);
	n = 1 << count(mask);
	for (i = 0; i < n; ++i) {
		P = get_P(mask, i);
		if (get_flip_count(h_to_line(P, x), h_flip_index(x)) != count_flip_h(P, x)) {
			fprintf(stderr, "\nwrong h count\n");
			print_bitboard(P, stderr);
			fprintf(stderr, " (%d) -> ", x);
			print_bitline(h_to_line(P, x), stderr);
			fprintf(stderr, " (%d) : ", h_flip_index(x));
			fprintf(stderr, "%d != %d\n", count_flip_h(P, x), get_flip_count(h_to_line(P, x), h_flip_index(x)));
			abort();
		}
	}

	mask = v_mask(x) & ~(1ULL << x);
	n = 1 << count(mask);
	for (i = 0; i < n; ++i) {
		P = get_P(mask, i);
		if (get_flip_count(v_to_line(P, x), v_flip_index(x)) != count_flip_v(P, x)) {
			fprintf(stderr, "wrong v count\n");
			print_bitboard(P, stderr);
			fprintf(stderr, " (%d) -> ", x);
			print_bitline(v_to_line(P, x), stderr);
			fprintf(stderr, " (%d) : ", v_flip_index(x));
			fprintf(stderr, "%d != %d\n", count_flip_v(P, x), get_flip_count(v_to_line(P, x), v_flip_index(x)));
			abort();
		}
	}

	mask = d7_mask(x) & ~(1ULL << x);
	n = 1 << count(mask);
	for (i = 0; i < n; ++i) {
		P = get_P(mask, i);
		if (get_flip_count(d7_to_line(P, x), d7_flip_index(x)) != count_flip_d7(P, x)) {
			fprintf(stderr, "wrong d7 count\n");
			print_bitboard(P, stderr);
			fprintf(stderr, " (%d) -> ", x);
			print_bitline(d7_to_line(P, x), stderr);
			fprintf(stderr, " (%d) : ", d7_flip_index(x));
			fprintf(stderr, " (%d) : ", x);
			fprintf(stderr, "%d != %d\n", count_flip_d7(P, x), get_flip_count(d7_to_line(P, x), d7_flip_index(x)));
			abort();
		}
	}

	mask = d9_mask(x) & ~(1ULL << x);
	n = 1 << count(mask);
	for (i = 0; i < n; ++i) {
		P = get_P(mask, i);
		if (get_flip_count(d9_to_line(P, x), d9_flip_index(x)) != count_flip(P, x)) {
			fprintf(stderr, "wrong d9 count\n");
			print_bitboard(P, stderr);
			fprintf(stderr, " (%d) -> ", x);
			print_bitline(d9_to_line(P, x), stderr);
			fprintf(stderr, " (%d) : ", d9_flip_index(x));
			fprintf(stderr, "%d != %d\n", get_flip_count(d9_to_line(P, x), d9_flip_index(x)), count_flip(P, x));
			abort();
		}
	}
}

int main() {
	int i, n, x, y;
	char s[3] = "--";
	FILE *f;

	int merge_diagonals[] = {
		0, 0, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 0, 0,
		2, 2, 0, 0, 0, 0, 2, 2,
		2, 2, 0, 0, 0, 0, 2, 2,
		2, 2, 0, 0, 0, 0, 2, 2,
		2, 2, 0, 0, 0, 0, 2, 2,
		0, 0, 1, 1, 1, 1, 0, 0,
		0, 0, 1, 1, 1, 1, 0, 0,
	};

	f = fopen("count_last_flip_kindergarten.c", "w");
	if (f == NULL) {
		fprintf(stderr, "cannot open 'count_flip.c'\n");
		exit(EXIT_FAILURE);
	}

	// check the algorithm...
	for (x = 0; x < 64; ++x) {
		check(x);
	}

	puts("Generating code..."); fflush(stdout);

	fprintf(f, "/**\n");
	fprintf(f, " * @file count_last_flip_kindergarten.c\n");
	fprintf(f, " *\n");
	fprintf(f, " *\n");
	fprintf(f, " * A function is provided to count the number of fipped disc of the last move\n");
	fprintf(f, " * for each square of the board. These functions are gathered into an array of\n");
	fprintf(f, " * functions, so that a fast access to each function is allowed. The generic\n");
	fprintf(f, " * form of the function take as input the player bitboard and return twice\n");
	fprintf(f, " * the number of flipped disc of the last move.\n");
	fprintf(f, " *\n");
	fprintf(f, " * The basic principle is to read into an array a precomputed result. Doing\n");
	fprintf(f, " * this is easy for a single line ; as we can use arrays of the form:\n");
	fprintf(f, " *  - COUNT_FLIP[square where we play][8-bits disc pattern].\n");
	fprintf(f, " * The problem is thus to convert any line of a 64-bits disc pattern into an\n");
	fprintf(f, " * 8-bits disc pattern. A fast way to do this is to select the right line,\n");
	fprintf(f, " * with a bit-mask, to gather the masked-bits into a continuous set by a simple\n");
	fprintf(f, " * multiplication and to right-shift the result to scale it into a number\n");
	fprintf(f, " * between 0 and 255.\n");
	fprintf(f, " * Once we get our 8-bits disc patterns, we directly get the number of\n");
	fprintf(f, " * flipped discs from the precomputed array, and add them from each flipping\n");
	fprintf(f, " * lines.\n");
	fprintf(f, " * For optimization purpose, the value returned is twice the number of flipped\n");
	fprintf(f, " * disc, to facilitate the computation of disc difference.\n");
	fprintf(f, " *\n");
	fprintf(f, " * With Modifications by Valéry ClaudePierre (merging diagonals).\n");
	fprintf(f, " *\n");
	fprintf(f, " * @date 1998 - 2012\n");
	fprintf(f, " * @author Richard Delorme\n");
	fprintf(f, " * @version 4.3\n");
	fprintf(f, " *\n");
	fprintf(f, " */\n\n");

	fprintf(f, "/** precomputed count flip array */\n");
	fprintf(f, "const char COUNT_FLIP[8][256] = {\n");
	for (i = 0; i < 8; ++i) {
		fprintf(f, "\t{\n");
		for (n = 0; n < 256; ++n) {
			if (n % 32 == 0) fprintf(f, "\t\t"); else putc(' ', f);
			fprintf(f, "%2d,", 2 * get_flip_count(n, i));
			if (n % 32 == 31) fprintf(f, "\n");
		}
		fprintf(f, "\t},\n");
	}
	fprintf(f, "};\n\n");

	for (n = 0; n < 64; n++) {
		x = n % 8; s[0] = 'A' + x;
		y = n / 8; s[1] = '1' + y;

		fprintf(f, "/**\n");
		fprintf(f, " * Count last flipped discs when playing on square %s.\n", s);
		fprintf(f, " *\n");
		fprintf(f, " * @param P player's disc pattern.\n");
		fprintf(f, " * @return flipped disc count.\n");
	 	fprintf(f, " */\n");
		fprintf(f, "static int count_last_flip_%s(const unsigned long long P)\n{\n", s);
		fprintf(f, "\tregister int n_flipped;\n");
		fprintf(f, "\n");

		fprintf(f, "\tn_flipped  = COUNT_FLIP[%d][((P & 0x%016llxULL) * 0x%016llxULL) >> 56];\n", v_flip_index(n), v_mask(n), (0x0102040810204080ull >> x));

		if (y == 0)
			fprintf(f, "\tn_flipped += COUNT_FLIP[%d][P & 0xff];\n", h_flip_index(n));
		else if (y == 7)
			fprintf(f, "\tn_flipped += COUNT_FLIP[%d][P >> %d];\n", h_flip_index(n), y * 8);
		else
			fprintf(f, "\tn_flipped += COUNT_FLIP[%d][(P >> %d) & 0xff];\n", h_flip_index(n), y * 8);

		if (merge_diagonals[n] == 1) {
			fprintf(f, "\tn_flipped += COUNT_FLIP[%d][(P & 0x%016llxULL) * 0x%016llxULL >> 56];\n", d_flip_index(n), d_mask(n), 0x0101010101010101ULL);
		} else if (merge_diagonals[n] == 2) {
			fprintf(f, "\tn_flipped += COUNT_FLIP[%d][(((P & 0x%016llxULL) + 0x%016llxULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56];\n", v_flip_index(n), d_mask(n), d_add(n));
		} else {
			if ((x < 6 && y > 1) || (x > 1 && y < 6)) {
				if (n == H6 || n == G6 || n == C2 || n == C1) {
					fprintf(f, "\tn_flipped += ((P & 0x%016llxULL) == 0x%016llxULL);\n", BIT[n] << 7 | BIT[n] << 14, BIT[n] << 14);
				} else if (n == A3 || n == B3 || n == F7 || n == F8) {
					fprintf(f, "\tn_flipped += ((P & 0x%016llxULL) == 0x%016llxULL);\n", BIT[n] >> 7 | BIT[n] >> 14, BIT[n] >> 14);
				} else {
					fprintf(f, "\tn_flipped += COUNT_FLIP[%d][((P & 0x%016llxULL) * 0x%016llxULL) >> 56];\n", d7_flip_index(n), d7_mask(n), 0x0101010101010101ull);
				}
			}
			if ((x < 6 && y < 6) || (x > 1 && y > 1)) {
				if (n == A6 || n == B6 || n == F1 || n == F2 ) {
					fprintf(f, "\tn_flipped += ((P & 0x%016llxULL) == 0x%016llxULL);\n",  BIT[n] << 9 | BIT[n] << 18, BIT[n] << 18);
				} else if (n == C8 || n == C7 || n == H3 || n == G3) {
					fprintf(f, "\tn_flipped += ((P & 0x%016llxULL) == 0x%016llxULL);\n", BIT[n] >> 9 | BIT[n] >> 18, BIT[n] >> 18);
				} else {
					fprintf(f, "\tn_flipped += COUNT_FLIP[%d][((P & 0x%016llxULL) * 0x%016llxULL) >> 56];\n", d9_flip_index(n), d9_mask(n), 0x0101010101010101ull);
				}
			}
		}
		fprintf(f, "\n");
		fprintf(f, "\treturn n_flipped;\n");
		fprintf(f, "}\n");
		fprintf(f, "\n");
	}
	fprintf(f, "/**\n");
	fprintf(f, " * Count last flipped discs when plassing.\n");
	fprintf(f, " *\n");
	fprintf(f, " * @param P player's disc pattern (unused).\n");
	fprintf(f, " * @return zero.\n");
	fprintf(f, " */\n");
	fprintf(f, "static int count_last_flip_pass(const unsigned long long P)\n{\n");
	fprintf(f, "\t(void) P; // useless code to shut-up compiler warning\n");
	fprintf(f, "\treturn 0;\n");
	fprintf(f, "}\n\n");

	fprintf(f, "/** Array of functions to count flipped discs of the last move */\n");
	fprintf(f, "int (*count_last_flip[])(const unsigned long long) = {\n");
	for (n = 0; n < 64; n++) {
		x = n % 8; s[0] = 'A' + x;
		y = n / 8; s[1] = '1' + y;
		if (n % 4 == 0) fprintf(f, "\t"); else putc(' ', f);
		fprintf(f, "count_last_flip_%s,", s);
		if (n % 4 == 3) fprintf(f, "\n");
	}
	fprintf(f, "\tcount_last_flip_pass,\n};\n\n");

	fclose(f);
}
