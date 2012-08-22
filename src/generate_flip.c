/**
 * @file generate_flip.c
 *
 * This program generates the flip_kindergarten.c file.
 *
 * @date 1998 - 2012
 * @author Richard Delorme
 * @version 4.3
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>

// define this to mix the diagonals
//  0 = do not mix
//  1 = up & down borders
//  2 = left & right borders
//  3 = 1 + 2 
#define MERGE_DIAGONALS 3

// usefull type
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
	A8, B8, C8, D8, E8, F8, G8, H8,
};

const uint64 BIT[64] = {
	0x0000000000000001, 0x0000000000000002, 0x0000000000000004, 0x0000000000000008,
	0x0000000000000010, 0x0000000000000020, 0x0000000000000040, 0x0000000000000080,
	0x0000000000000100, 0x0000000000000200, 0x0000000000000400, 0x0000000000000800,
	0x0000000000001000, 0x0000000000002000, 0x0000000000004000, 0x0000000000008000,
	0x0000000000010000, 0x0000000000020000, 0x0000000000040000, 0x0000000000080000,
	0x0000000000100000, 0x0000000000200000, 0x0000000000400000, 0x0000000000800000,
	0x0000000001000000, 0x0000000002000000, 0x0000000004000000, 0x0000000008000000,
	0x0000000010000000, 0x0000000020000000, 0x0000000040000000, 0x0000000080000000,
	0x0000000100000000, 0x0000000200000000, 0x0000000400000000, 0x0000000800000000,
	0x0000001000000000, 0x0000002000000000, 0x0000004000000000, 0x0000008000000000,
	0x0000010000000000, 0x0000020000000000, 0x0000040000000000, 0x0000080000000000,
	0x0000100000000000, 0x0000200000000000, 0x0000400000000000, 0x0000800000000000,
	0x0001000000000000, 0x0002000000000000, 0x0004000000000000, 0x0008000000000000,
	0x0010000000000000, 0x0020000000000000, 0x0040000000000000, 0x0080000000000000,
	0x0100000000000000, 0x0200000000000000, 0x0400000000000000, 0x0800000000000000,
	0x1000000000000000, 0x2000000000000000, 0x4000000000000000, 0x8000000000000000
};

//line to bitboard conversion
uint64 B1G1[64];
uint64 B2G2[64];
uint64 B3G3[64];
uint64 B4G4[64];
uint64 B5G5[64];
uint64 B6G6[64];
uint64 B7G7[64];
uint64 B8G8[64];

uint64 A2A7[64];
uint64 B2B7[64];
uint64 C2C7[64];
uint64 D2D7[64];
uint64 E2E7[64];
uint64 F2F7[64];
uint64 G2G7[64];
uint64 H2H7[64];

uint64 B2B2[2];
uint64 C2B3[4];
uint64 D2B4[8];
uint64 E2B5[16];
uint64 F2B6[32];
uint64 G2B7[64];
uint64 G3C7[32];
uint64 G4D7[16];
uint64 G5E7[8];
uint64 G6F7[4];
uint64 G7G7[2];

uint64 B7B7[2];
uint64 B6C7[4];
uint64 B5D7[8];
uint64 B4E7[16];
uint64 B3F7[32];
uint64 B2G7[64];
uint64 C2G6[32];
uint64 D2G5[16];
uint64 E2G4[8];
uint64 F2G3[4];
uint64 G2G2[2];

uint64 G2H3D7[64];
uint64 F2H4E7[64];
uint64 E2H5F7[64];
uint64 D2H6G7[64];
uint64 F2G3C7[64];
uint64 E2G4D7[64];
uint64 D2G5E7[64];
uint64 C2G6F7[64];

uint64 B2A3E7[64];
uint64 C2A4D7[64];
uint64 D2A5C7[64];
uint64 E2A6B7[64];
uint64 C2B3F7[64];
uint64 D2B4E7[64];
uint64 E2B5D7[64];
uint64 F2B6C7[64];

uint64 B2C1G5[64];
uint64 B3D1G4[64];
uint64 B4E1G3[64];
uint64 B5F1G2[64];
uint64 B3C2G6[64];
uint64 B4D2G5[64];
uint64 B5E2G4[64];
uint64 B6F2G3[64];

uint64 B7C8G4[64];
uint64 B6D8G5[64];
uint64 B5E8G6[64];
uint64 B4F8G7[64];
uint64 B6C7G3[64];
uint64 B5D7G4[64];
uint64 B4E7G5[64];
uint64 B3F7G6[64];


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
inline int next_one(uint64 *b, int x) {
	*b ^= (1ULL << x);
	return first_one(*b);
}

#define foreach_bits(i, b) for (i = first_one(b); b; i = next_one(&b, i))

/* count the number of bits set */
inline int count(uint64 b) {
	b -=  (b >> 1) & 0x5555555555555555ULL;
	b  = ((b >> 2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
	b  = ((b >>4) + b) & 0x0f0f0f0f0f0f0f0fULL;
	b *= 0x0101010101010101ULL;

	return  (int)(b >> 56);
}

/* othello: flip count of the last move */

/* get a player disc bitboard included in mask */
uint64 P(uint64 mask, const uint64 flat) {
	int i, n = 0, bit_i[64];

	uint64 ret = 0;
	foreach_bits(i, mask) bit_i[n++] = i;
	for (i = 0; i < n; i++)
		if (flat & (1ULL << i)) ret |= (1ULL << bit_i[i]);

	return ret;
}


void print_bitboard(uint64 b, FILE *f) {
	char s[] = ".X?";
	int i, bit;
	for (i = 0; i < 64; ++i) {
		if (b & (1ULL << i)) bit = 1; else bit = 0;
		putc(s[bit], f);
		if ((i & 7) == 7) putc('\n', f);
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

uint64 h_mask(int x, int a, int b) {
	uint64 m = 0;
	int h = x / 8;
	int i;

	for (i = a; i <= b; ++i) m |= (1ULL << (h * 8 + i));

	return m;
}

uint64 v_mask(int x, int a, int b) {
	uint64 m = 0;
	int v = x % 8;
	int i;

	for (i = a; i <= b; ++i) m |= (1ULL << (i * 8 + v));
	
	return m;
}
	
uint64 d7_mask(int x, int a, int b) {
	uint64 m = 0;
	int r = x / 8;
	int c = x % 8;
	int i, j;

	for (i = r, j = c; i <= b && j >= a; ++i, --j) m |= (1ULL << (i * 8 + j));
	for (i = r - 1, j = c + 1; i >= a && j <= b; --i, ++j) m |= (1ULL << (i * 8 + j));

	return m;
}

uint64 d9_mask(int x, int a, int b) {
	uint64 m = 0;
	int r = x / 8;
	int c = x % 8;
	int i, j;

	for (i = r, j = c; i >= a && j >= a; --i, --j) m |= (1ULL << (i * 8 + j));
	for (i = r + 1, j = c + 1; i <= b && j <= b; ++i, ++j) m |= (1ULL << (i * 8 + j));

	return m;
}

uint64 d_mask(int x, int a, int b) {
	uint64 m = d7_mask(x, a, b) | d9_mask(x, a, b);
	int r = x / 8;
	int c = x % 8;
	if (r == 1) m &= 0xffffffffffffff00ULL;
	if (r == 6) m &= 0x00ffffffffffffffULL;
	if (c == 1) m &= 0xfefefefefefefefeULL;
	if (c == 6) m &= 0x7f7f7f7f7f7f7f7fULL;
	return m;
}

uint64 d_add(int x, int a, int b) {
	return (0x8080808080808080ULL - d_mask(x, a, b)) & 0x7f7f7f7f7f7f7f7fULL;
}

/* select a line to flip discs on
 * i.e. convert a 64 bit board to a 8 bit line
 */
// horizontal case (row selection)
uint8 h_to_line(uint64 b, int x) {
	int h = x / 8;

	 return (b >> (8 * h)) & 0xff;
}

// vertical case (file selection)
uint8 v_to_line(uint64 b, int x) {
	int v = x % 8;
	uint64 m = v_mask(x, 0, 7);
	
	return ((b & m) * (0x0102040810204080 >> v)) >> 56;
}

// diagonal case. Warning! the bits are inverted here
uint8 d7_to_line(uint64 b, int x) {	
	uint64 m = d7_mask(x, 0, 7);
	return ((b & m) * (0x0101010101010101)) >> 56;
}

// diagonal case. (bits are ok)
uint8 d9_to_line(uint64 b, int x) {
	uint64 m = d9_mask(x, 0, 7);
	return ((b & m) * (0x0101010101010101)) >> 56;
}

// diagonal case. 
uint8 d_to_line(uint64 b, int x) {
	uint64 m = d_mask(x, 0, 7);
	return ((b & m) * (0x0101010101010101)) >> 56;
}

/* compute the index of the played square
 */
int flip_index_h(int x) {
	return first_one(h_to_line(1ULL << x, x));
}

int v_flip_index(int x) {
	return first_one(v_to_line(1ULL << x, x));
}

int d7_flip_index(int x) {
	return first_one(d7_to_line(1ULL << x, x));
}

int d9_flip_index(int x) {
	return first_one(d9_to_line(1ULL << x, x));
}

int d_flip_index(int x) {
	return first_one(d_to_line(1ULL << x, x));
}

/* count the # of flipped discs
 */ 
int flip_count(int l, int x) {
	int y;
	int i, n = 0;

	for (y = x - 1, i = 0; y >= 0 && !(l & (1 << y)); --y, ++i);
	if (y >= 0) n += i;
	for (y = x + 1, i = 0; y < 8 && !(l & (1 << y)); ++y, ++i);
	if (y < 8) n += i;

	return n;
}

int outflank(int o, int x) {
	int y, of = 0;

	if (!(o & (1 << x))) {
		for (y = x - 1; y >= 0 && (o & (1 << y)); --y);
		if (y >= 0 && y < x - 1) of |= (1 << y);
		for (y = x + 1; y < 8 && (o & (1 << y)); ++y);
		if (y < 8 && y > x + 1) of |= (1 << y);
	}
//	printf("outflank: "); print_bitline(o, stdout); printf(" --%d--> ", x); print_bitline(of, stdout); putchar('\n');

	return of;
}

int flip(int of, int x) {
	int y;
	int t, f = 0;

	if (!(of & (1 << x))) {
		for (y = x - 1, t = 0; y >= 0 && !(of & (1 << y)); --y) t |= (1 << y);
		if (y >= 0) f |= t;
		for (y = x + 1, t = 0; y < 8 && !(of & (1 << y)); ++y) t |= (1 << y);
		if (y < 8) f |= t;
	}
//	printf("flip: "); print_bitline(of, stdout); printf(" --%d--> ", x); print_bitline(f, stdout); putchar('\n');

	return f >> 1;
}

char* h_name(int x) {
	static char s[5] = "B-G-";
	int i = x / 8;
	s[3] = s[1] = '1' + i;
	return s;
}

char* v_name(int x) {
	static char s[5] = "-2-7";
	int i = x % 8;
	s[2] = s[0] = 'A' + i;
	return s;
}

char* d7_name(int x) {
	int d7[64] = {
		0, 1, 2, 3, 4, 5, 6, 7,
		1, 2, 3, 4, 5, 6, 7, 8,
		2, 3, 4, 5, 6, 7, 8, 9,
		3, 4, 5, 6, 7, 8, 9,10,
		4, 5, 6, 7, 8, 9,10,11,
		5, 6, 7, 8, 9,10,11,12,
		6, 7, 8, 9,10,11,12,13,
		7, 8, 9,10,11,12,13,14
	};
	static char s[15][5] = {
		"", "", "B2B2", "C2B3", "D2B4", "E2B5", "F2B6",
		"G2B7",
		"G3C7", "G4D7", "G5E7", "G6F7", "G7G7", "", ""
	};

	return s[d7[x]];
}

int d7_shift_index(int x) {
	int d7[64] = {
		0, 1, 2, 3, 4, 5, 6, 7,
		1, 2, 3, 4, 5, 6, 7, 8,
		2, 3, 4, 5, 6, 7, 8, 9,
		3, 4, 5, 6, 7, 8, 9,10,
		4, 5, 6, 7, 8, 9,10,11,
		5, 6, 7, 8, 9,10,11,12,
		6, 7, 8, 9,10,11,12,13,
		7, 8, 9,10,11,12,13,14
	};
	int s[15] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		1, 2, 3, 4, 5, 0, 0
	};

	return s[d7[x]];
}
	

char* d9_name(int x) {
	int d9[64] = {
		7, 8, 9,10,11,12,13,14,
		6, 7, 8, 9,10,11,12,13,
		5, 6, 7, 8, 9,10,11,12,
		4, 5, 6, 7, 8, 9,10,11,
		3, 4, 5, 6, 7, 8, 9,10,
		2, 3, 4, 5, 6, 7, 8, 9,
		1, 2, 3, 4, 5, 6, 7, 8,
		0, 1, 2, 3, 4, 5, 6, 7
	};
	static char s[15][5] = {
 		"", "", "B7B7", "B6C7", "B5D7", "B4E7", "B3F7",
		"B2G7",
		"C2G6", "D2G5", "E2G4", "F2G3", "G2G2", "", ""
	};

	return s[d9[x]];
}

int d9_shift_index(int x) {
	int d9[64] = {
		7, 8, 9,10,11,12,13,14,
		6, 7, 8, 9,10,11,12,13,
		5, 6, 7, 8, 9,10,11,12,
		4, 5, 6, 7, 8, 9,10,11,
		3, 4, 5, 6, 7, 8, 9,10,
		2, 3, 4, 5, 6, 7, 8, 9,
		1, 2, 3, 4, 5, 6, 7, 8,
		0, 1, 2, 3, 4, 5, 6, 7
	};
	int s[15] = {
		0, 0, 0, 0, 0, 0, 0, 0,
		1, 2, 3, 4, 5, 0, 0
	};

	return s[d9[x]];
}

char* d_name(const int x) {
	static char a[64][7];
	char *s = a[x];
	int f, r, h, v;
		
	f = x % 8; r = x / 8;
	s[2] = 'A' + f; s[3] = '1' + r; s[6] = '\0';
	if (f < 2 && r > 1 && r < 6) {
		for (h = r, v = f; h > 1 && v < 6; --h, ++v);
		s[0]='A' + v; s[1] = '1' + h;
		for (h = r, v = f; h < 6 && v < 6; ++h, ++v);
		s[4]='A' + v; s[5] = '1' + h;
	} else if (f > 5 && r > 1 && r < 6) {
		for (h = r, v = f; h > 1 && v > 1; --h, --v);
		s[0]='A' + v; s[1] = '1' + h;
		for (h = r, v = f; h < 6 && v > 1; ++h, --v);
		s[4]='A' + v; s[5] = '1' + h;
	} else if (r < 2 && f > 1 && f < 6) {
		for (h = r, v = f; h < 6 && v > 1; ++h, --v);
		s[0]='A' + v; s[1] = '1' + h;
		for (h = r, v = f; h < 6 && v < 6; ++h, ++v);
		s[4]='A' + v; s[5] = '1' + h;
	} else if (r > 5 && f > 1 && f < 6) {
		for (h = r, v = f; h > 1 && v > 1; --h, --v);
		s[0]='A' + v; s[1] = '1' + h;
		for (h = r, v = f; h > 1 && v < 6; --h, ++v);
		s[4]='A' + v; s[5] = '1' + h;
	} else {
		s[0] = '\0';
	}
	return s;
}

// init unsigned long long
void init_index_to_bitarray() {
	int i, n;
	for (n = 0; n < 64; ++n) {
		B1G1[n] = 0;
		B2G2[n] = 0;
		B3G3[n] = 0;
		B4G4[n] = 0;
		B5G5[n] = 0;
		B6G6[n] = 0;
		B7G7[n] = 0;
		B8G8[n] = 0;

		A2A7[n] = 0;
		B2B7[n] = 0;
		C2C7[n] = 0;
		D2D7[n] = 0;
		E2E7[n] = 0;
		F2F7[n] = 0;
		G2G7[n] = 0;
		H2H7[n] = 0;

		G2B7[n] = 0;
		B2G7[n] = 0;

		for (i = 0; i < 6; ++i) {
			if (n & BIT[i]) {
				B1G1[n] |= BIT[B1 + i] ;
				B2G2[n] |= BIT[B2 + i] ;
				B3G3[n] |= BIT[B3 + i] ;
				B4G4[n] |= BIT[B4 + i] ;
				B5G5[n] |= BIT[B5 + i] ;
				B6G6[n] |= BIT[B6 + i] ;
				B7G7[n] |= BIT[B7 + i] ;
				B8G8[n] |= BIT[B8 + i] ;

				A2A7[n] |= BIT[A2 + 8 * i];
				B2B7[n] |= BIT[B2 + 8 * i];
				C2C7[n] |= BIT[C2 + 8 * i];
				D2D7[n] |= BIT[D2 + 8 * i];
				E2E7[n] |= BIT[E2 + 8 * i];
				F2F7[n] |= BIT[F2 + 8 * i];
				G2G7[n] |= BIT[G2 + 8 * i];
				H2H7[n] |= BIT[H2 + 8 * i];

				G2B7[n] |= BIT[B7 - 7 * i];
				B2G7[n] |= BIT[B2 + 9 * i];

				B2C1G5[n] |= (i < 1 ? BIT[B2 - 7 * i] : BIT[C1 + 9 * (i - 1)]);
				B3D1G4[n] |= (i < 2 ? BIT[B3 - 7 * i] : BIT[D1 + 9 * (i - 2)]);
				B4E1G3[n] |= (i < 3 ? BIT[B4 - 7 * i] : BIT[E1 + 9 * (i - 3)]);
				B5F1G2[n] |= (i < 4 ? BIT[B5 - 7 * i] : BIT[F1 + 9 * (i - 4)]);

				B3C2G6[n] |= (i < 1 ? BIT[B3 - 7 * i] : BIT[C2 + 9 * (i - 1)]);
				B4D2G5[n] |= (i < 2 ? BIT[B4 - 7 * i] : BIT[D2 + 9 * (i - 2)]);
				B5E2G4[n] |= (i < 3 ? BIT[B5 - 7 * i] : BIT[E2 + 9 * (i - 3)]);
				B6F2G3[n] |= (i < 4 ? BIT[B6 - 7 * i] : BIT[F2 + 9 * (i - 4)]);

				B7C8G4[n] |= (i < 1 ? BIT[B7 + 9 * i] : BIT[C8 - 7 * (i - 1)]);
				B6D8G5[n] |= (i < 2 ? BIT[B6 + 9 * i] : BIT[D8 - 7 * (i - 2)]);
				B5E8G6[n] |= (i < 3 ? BIT[B5 + 9 * i] : BIT[E8 - 7 * (i - 3)]);
				B4F8G7[n] |= (i < 4 ? BIT[B4 + 9 * i] : BIT[F8 - 7 * (i - 4)]);

				B6C7G3[n] |= (i < 1 ? BIT[B6 + 9 * i] : BIT[C7 - 7 * (i - 1)]);
				B5D7G4[n] |= (i < 2 ? BIT[B5 + 9 * i] : BIT[D7 - 7 * (i - 2)]);
				B4E7G5[n] |= (i < 3 ? BIT[B4 + 9 * i] : BIT[E7 - 7 * (i - 3)]);
				B3F7G6[n] |= (i < 4 ? BIT[B3 + 9 * i] : BIT[F7 - 7 * (i - 4)]);

				G2H3D7[n] |= (i < 1 ? BIT[G2 + 9 * i] : BIT[H3 + 7 * (i - 1)]);
				F2H4E7[n] |= (i < 2 ? BIT[F2 + 9 * i] : BIT[H4 + 7 * (i - 2)]);
				E2H5F7[n] |= (i < 3 ? BIT[E2 + 9 * i] : BIT[H5 + 7 * (i - 3)]);
				D2H6G7[n] |= (i < 4 ? BIT[D2 + 9 * i] : BIT[H6 + 7 * (i - 4)]);

				F2G3C7[n] |= (i < 1 ? BIT[F2 + 9 * i] : BIT[G3 + 7 * (i - 1)]);;
				E2G4D7[n] |= (i < 2 ? BIT[E2 + 9 * i] : BIT[G4 + 7 * (i - 2)]);
				D2G5E7[n] |= (i < 3 ? BIT[D2 + 9 * i] : BIT[G5 + 7 * (i - 3)]);
				C2G6F7[n] |= (i < 4 ? BIT[C2 + 9 * i] : BIT[G6 + 7 * (i - 4)]);

				B2A3E7[n] |= (i < 1 ? BIT[B2 + 7 * i] : BIT[A3 + 9 * (i - 1)]);
				C2A4D7[n] |= (i < 2 ? BIT[C2 + 7 * i] : BIT[A4 + 9 * (i - 2)]);
				D2A5C7[n] |= (i < 3 ? BIT[D2 + 7 * i] : BIT[A5 + 9 * (i - 3)]);
				E2A6B7[n] |= (i < 4 ? BIT[E2 + 7 * i] : BIT[A6 + 9 * (i - 4)]);

				C2B3F7[n] |= (i < 1 ? BIT[C2 + 7 * i] : BIT[B3 + 9 * (i - 1)]);
				D2B4E7[n] |= (i < 2 ? BIT[D2 + 7 * i] : BIT[B4 + 9 * (i - 2)]);
				E2B5D7[n] |= (i < 3 ? BIT[E2 + 7 * i] : BIT[B5 + 9 * (i - 3)]);
				F2B6C7[n] |= (i < 4 ? BIT[F2 + 7 * i] : BIT[B6 + 9 * (i - 4)]);
			}
		}
	}


	for (n = 0; n < 32; ++n) {
		F2B6[n] = 0;
		G3C7[n] = 0;
		B3F7[n] = 0;
		C2G6[n] = 0;
		for (i = 0; i < 5; ++i) {
			if (n & BIT[i]) {
				F2B6[n] |= BIT[B6 - 7 * i];
				G3C7[n] |= BIT[C7 - 7 * i];
				B3F7[n] |= BIT[B3 + 9 * i];
				C2G6[n] |= BIT[C2 + 9 * i];
			}
		}
	}
	for (n = 0; n < 16; ++n) {
		E2B5[n] = 0;
		G4D7[n] = 0;
		B4E7[n] = 0;
		D2G5[n] = 0;
		for (i = 0; i < 4; ++i) {
			if (n & BIT[i]) {
				E2B5[n] |= BIT[B5 - 7 * i];
				G4D7[n] |= BIT[D7 - 7 * i];
				B4E7[n] |= BIT[B4 + 9 * i];
				D2G5[n] |= BIT[D2 + 9 * i];
			}
		}
	}
	for (n = 0; n < 8; ++n) {
		D2B4[n] = 0;
		G5E7[n] = 0;
		B5D7[n] = 0;
		E2G4[n] = 0;
		for (i = 0; i < 3; ++i) {
			if (n & BIT[i]) {
				D2B4[n] |= BIT[B4 - 7 * i];
				G5E7[n] |= BIT[E7 - 7 * i];
				B5D7[n] |= BIT[B5 + 9 * i];
				E2G4[n] |= BIT[E2 + 9 * i];
			}
		}
	}
	for (n = 0; n < 4; ++n) {
		C2B3[n] = 0;
		G6F7[n] = 0;
		B6C7[n] = 0;
		F2G3[n] = 0;
		for (i = 0; i < 2; ++i) {
			if (n & BIT[i]) {
				C2B3[n] |= BIT[B3 - 7 * i];
				G6F7[n] |= BIT[F7 - 7 * i];
				B6C7[n] |= BIT[B6 + 9 * i];
				F2G3[n] |= BIT[F2 + 9 * i];
			}
		}
	}
	for (n = 0; n < 2; ++n) {
		B2B2[n] = 0;
		G7G7[n] = 0;
		B7B7[n] = 0;
		G2G2[n] = 0;
		for (i = 0; i < 1; ++i) {
			if (n & BIT[i]) {
				B2B2[n] |= BIT[B2 - 7 * i];
				G7G7[n] |= BIT[G7 - 7 * i];
				B7B7[n] |= BIT[B7 + 9 * i];
				G2G2[n] |= BIT[G2 + 9 * i];
			}
		}
	}
}

void print_bitarray_(FILE *f, uint64 array[], int size, char *name) {
	int n;
	fprintf(f, "/** conversion from an 8-bit line to the %.2s-%.2s-%.2s line */\n", name, name + 2, name + 4);
	fprintf(f, "unsigned long long %s[%d] = {\n", name, size);
	for (n = 0; n < size; ++n) {
		if (n % 8 == 0) fprintf(f, "\t\t");
		fprintf(f, "0x%016llxULL,", array[n]);
		if (n % 8 == 7) fputc('\n', f); else fputc(' ', f);
	}
 	fprintf(f, "};\n\n");
}

#define print_bitarray(a) (print_bitarray_)(f, a, (sizeof a)/(sizeof a[0]), #a)

int main() {
	int i, n, x, y, shift;
	char s[3] = "--";
	FILE *f;
	int flipped[8][256], of;

	int has_diagonal_d9[] = {
		1, 1, 1, 1, 1, 0, 0, 0,
		1, 1, 1, 1, 1, 0, 0, 0,
		1, 1, 1, 1, 1, 0, 0, 0,
		1, 1, 1, 1, 1, 1, 1, 1, 
		1, 1, 1, 1, 1, 1, 1, 1, 
		0, 0, 0, 1, 1, 1, 1, 1,
		0, 0, 0, 1, 1, 1, 1, 1,
		0, 0, 0, 1, 1, 1, 1, 1,
	};

	int has_diagonal_d7[] = {
		0, 0, 0, 1, 1, 1, 1, 1,
		0, 0, 0, 1, 1, 1, 1, 1,
		0, 0, 0, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1,
		1, 1, 1, 1, 1, 1, 1, 1, 
		1, 1, 1, 1, 1, 0, 0, 0,
		1, 1, 1, 1, 1, 0, 0, 0,
		1, 1, 1, 1, 1, 0, 0, 0,
	};

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

	setvbuf(stdout, NULL, _IOLBF, 0);

	f = fopen("flip_kindergarten.c", "w");
	if (f == NULL) {
		fprintf(stderr, "cannot open 'flip.c'\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < 8; ++i)
	for (n = 0; n < 256; ++n) flipped[i][n] = 0;

	init_index_to_bitarray();

	puts("Generating code..."); fflush(stdout);

	fputs("/**\n", f);
	fputs(" * @file flip_kindergarten.c\n", f);
	fputs(" *\n", f);
	fputs(" * This module deals with flipping discs.\n", f);
	fputs(" *\n", f);
	fputs(" * A function is provided for each square of the board. These functions are\n", f);
	fputs(" * gathered into an array of functions, so that a fast access to each function\n", f);
	fputs(" * is allowed. The generic form of the function take as input the player and\n", f);
	fputs(" * the opponent bitboards and return the flipped squares into a bitboard.\n", f);
	fputs(" *\n", f);
	fputs(" * Given the following notation:\n", f);
	fputs(" *  - x = square where we play,\n", f);
	fputs(" *  - P = player's disc pattern,\n", f);
	fputs(" *  - O = opponent's disc pattern,\n", f);
	fputs(" * the basic principle is to read into an array the result of a move. Doing\n", f);
	fputs(" * this is easier for a single line ; so we can use arrays of the form:\n", f);
	fputs(" *  - ARRAY[x][8-bits disc pattern].\n", f);
	fputs(" * The problem is thus to convert any line of a 64-bits disc pattern into an\n", f);
	fputs(" * 8-bits disc pattern. A fast way to do this is to select the right line,\n", f);
	fputs(" * with a bit-mask, to gather the masked-bits into a continuous set by a simple\n", f);
	fputs(" * multiplication and to right-shift the result to scale it into a number\n", f);
	fputs(" * between 0 and 255.\n", f);
	fputs(" * Once we get our 8-bits disc patterns,a first array (OUTFLANK) is used to\n", f);
	fputs(" * get the player's discs that surround the opponent discs:\n", f);
	fputs(" *  - outflank = OUTFLANK[x][O] & P\n", f);
	fputs(" * The result is then used as an index to access a second array giving the\n", f);
	fputs(" * flipped discs according to the surrounding player's discs:\n", f);
	fputs(" *  - flipped = FLIPPED[x][outflank].\n", f);
	fputs(" * Finally, a precomputed array transform the 8-bits disc pattern back into a\n", f);
	fputs(" * 64-bits disc pattern, and the flipped squares for each line are gathered and\n", f);
	fputs(" * returned to generate moves.\n", f);
	fputs(" *\n", f);
	fputs(" * File automatically generated\n", f);
	fputs(" * @date 1998 - 2012\n", f);
	fputs(" * @author Richard Delorme\n", f);
	fputs(" * @version 4.3\n", f);
	fputs(" */\n\n", f);

	fprintf(f, "/** outflank array */\n");
	fprintf(f, "const unsigned char OUTFLANK[8][64] = {\n");
	for (i = 0; i < 8; ++i) {
		fprintf(f, "\t{\n");
		for (n = 0; n < 64; ++n) {
			if (n % 16 == 0) fprintf(f, "\t\t");
			of = outflank(n << 1, i);
			if (of) flipped[i][of] = flip(of, i);
			fprintf(f, "0x%02x,", of);
			if (n % 16 == 15) fputc('\n', f); else fputc(' ', f);
	}
		fprintf(f, "\t},\n");
	}
	fprintf(f, "};\n\n");
	fprintf(f, "/** flip array */\n");
	fprintf(f, "const unsigned char FLIPPED[8][144] = {\n");
	for (i = 0; i < 8; ++i) {
		fprintf(f, "\t{\n");
		for (n = 0; n < 144; ++n) {
			if (n % 16 == 0) fprintf(f, "\t\t");
			fprintf(f, "0x%02x,", flipped[i][n]);
			if (n % 16 == 15) fputc('\n', f); else fputc(' ', f);
		}
		fprintf(f, "\t},\n");
	}
	fprintf(f, "};\n\n");

	print_bitarray(B1G1);
	print_bitarray(B2G2);
	print_bitarray(B3G3);
	print_bitarray(B4G4);
	print_bitarray(B5G5);
	print_bitarray(B6G6);
	print_bitarray(B7G7);
	print_bitarray(B8G8);

	print_bitarray(A2A7);
	print_bitarray(B2B7);
	print_bitarray(C2C7);
	print_bitarray(D2D7);
	print_bitarray(E2E7);
	print_bitarray(F2F7);
	print_bitarray(G2G7);
	print_bitarray(H2H7);

#if MERGE_DIAGONALS != 3
	print_bitarray(C2B3);
	print_bitarray(D2B4);
	print_bitarray(G5E7);
	print_bitarray(G6F7);
	print_bitarray(B6C7);
	print_bitarray(B5D7);
	print_bitarray(E2G4);
	print_bitarray(F2G3);
#endif
	print_bitarray(E2B5);
	print_bitarray(F2B6);
	print_bitarray(G2B7);
	print_bitarray(G3C7);
	print_bitarray(G4D7);

	print_bitarray(B4E7);
	print_bitarray(B3F7);
	print_bitarray(B2G7);
	print_bitarray(C2G6);
	print_bitarray(D2G5);
	fputc('\n', f);

#if MERGE_DIAGONALS & 1
	print_bitarray(B2C1G5);
	print_bitarray(B3D1G4);
	print_bitarray(B4E1G3);
	print_bitarray(B5F1G2);
	print_bitarray(B3C2G6);
	print_bitarray(B4D2G5);
	print_bitarray(B5E2G4);
	print_bitarray(B6F2G3);

	print_bitarray(B6C7G3);
	print_bitarray(B5D7G4);
	print_bitarray(B4E7G5);
	print_bitarray(B3F7G6);
	print_bitarray(B7C8G4);
	print_bitarray(B6D8G5);
	print_bitarray(B5E8G6);
	print_bitarray(B4F8G7);
#endif

#if MERGE_DIAGONALS & 2
	print_bitarray(G2H3D7);
	print_bitarray(F2H4E7);
	print_bitarray(E2H5F7);
	print_bitarray(D2H6G7);
	print_bitarray(F2G3C7);
	print_bitarray(E2G4D7);
	print_bitarray(D2G5E7);
	print_bitarray(C2G6F7);

	print_bitarray(B2A3E7);
	print_bitarray(C2A4D7);
	print_bitarray(D2A5C7);
	print_bitarray(E2A6B7);
	print_bitarray(C2B3F7);
	print_bitarray(D2B4E7);
	print_bitarray(E2B5D7);
	print_bitarray(F2B6C7);
#endif

	fputc('\n', f);

	for (n = 0; n < 64; n++) {
		x = n % 8; s[0] = 'A' + x;
		y = n / 8; s[1] = '1' + y;

		fprintf(f, "/**\n");
		fprintf(f, " * Compute flipped discs when playing on square %s.\n", s);
		fprintf(f, " *\n");
		fprintf(f, " * @param P player's disc pattern.\n");
		fprintf(f, " * @param O opponent's disc pattern.\n");
		fprintf(f, " * @return flipped disc pattern.\n");
		fprintf(f, " */\n");
		fprintf(f, "static unsigned long long flip_%s(const unsigned long long P, const unsigned long long O)\n{\n", s);
		fprintf(f, "\tregister int index_h, index_v");
		if (merge_diagonals[n] & MERGE_DIAGONALS) fprintf(f, ", index_d");
		else {
			if (has_diagonal_d7[n]) fprintf(f, ", index_d7");
			if (has_diagonal_d9[n]) fprintf(f, ", index_d9");
		}
		fprintf(f, ";\n");
		fprintf(f, "\tregister unsigned long long flipped;\n");
		fprintf(f, "\n");

		// start by vertical because the first index of OUTFLANK & FLIPPED differ from the other direction
		fprintf(f, "\tindex_v = OUTFLANK[%d][(O & 0x%016llx) * 0x%016llx >> 57] & (P & 0x%016llx) * 0x%016llx >> 56;\n", v_flip_index(n), v_mask(n, 1, 6), 0x0002040810204000ULL >> x, v_mask(n, 0, 7), 0x0102040810204080ULL >> x);
		fprintf(f, "\tindex_v = FLIPPED[%d][index_v];\n", v_flip_index(n));
		fprintf(f, "\tflipped = %s[index_v];\n\n", v_name(n));

		if (y) fprintf(f, "\tindex_h = OUTFLANK[%d][(O >> %d) & 0x3f] & (P >> %d);\n", flip_index_h(n), 1 + y * 8, y * 8);
		else fprintf(f, "\tindex_h = OUTFLANK[%d][(O >> 1) & 0x3f] & P;\n", flip_index_h(n));
		fprintf(f, "\tflipped |= ((unsigned long long) FLIPPED[%d][index_h])", flip_index_h(n));
		fprintf(f," << %d", y * 8 + 1);
		fprintf(f, ";\n\n");

		if ((merge_diagonals[n] & MERGE_DIAGONALS) == 1) {
			fprintf(f, "\tindex_d = OUTFLANK[%d][(O & 0x%016llxULL) * 0x%016llxULL >> 57] & (P & 0x%016llxULL) * 0x%016llxULL >> 56;\n", d_flip_index(n), d_mask(n, 1, 6), 0x0101010101010101ULL, d_mask(n, 0, 7), 0x0101010101010101ULL);
			fprintf(f, "\tindex_d = FLIPPED[%d][index_d];\n", d_flip_index(n));
			fprintf(f, "\tflipped |= %s[index_d];\n\n", d_name(n));
		} else if ((merge_diagonals[n] & MERGE_DIAGONALS) == 2) {
			fprintf(f, "\tindex_d = OUTFLANK[%d][(((O & 0x%016llxULL) + 0x%016llxULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 57] & (((P & 0x%016llxULL) + 0x%016llxULL) & 0x8080808080808080ULL) * 0x0002040810204081ULL >> 56;\n", v_flip_index(n), d_mask(n, 1, 6), d_add(n, 1, 6), d_mask(n, 0, 7), d_add(n, 0, 7));
			fprintf(f, "\tindex_d = FLIPPED[%d][index_d];\n", v_flip_index(n));
			fprintf(f, "\tflipped |= %s[index_d];\n\n", d_name(n));
		} else {

			if ((x < 6 && y > 1) || (x > 1 && y < 6)) {
				if (n == C3 || n == F6 ) {
					fprintf(f, "\tflipped |= ((P >> 7) & 0x%016llxULL & O) | ((P << 7) & 0x%016llxULL & O);\n\n", BIT[n] << 7, BIT[n] >> 7);
				} else if (n == H6 || n == G6 || n == C1 || n == C2) {
					fprintf(f, "\tflipped |= ((P >> 7) & 0x%016llxULL & O);\n\n", BIT[n] << 7);
				} else if (n == A3 || n == B3 || n == F8 || n == F7) {
					fprintf(f, "\tflipped |= ((P << 7) & 0x%016llxULL & O);\n\n", BIT[n] >> 7);
				} else {
					fprintf(f, "\tindex_d7 = OUTFLANK[%d][(O & 0x%016llxULL)* 0x%016llxULL >> 57] & (P & 0x%016llxULL) * 0x%016llxULL >> 56;\n",
						d7_flip_index(n), d7_mask(n, 1, 6), 0x0101010101010101ULL, d7_mask(n, 0, 7), 0x0101010101010101ULL);
					fprintf(f, "\tindex_d7 = FLIPPED[%d][index_d7];\n", d7_flip_index(n));
					if ((shift = d7_shift_index(n))) fprintf(f, "\tflipped |= %s[index_d7 >> %d];\n\n", d7_name(n), shift);
					else fprintf(f, "\tflipped |= %s[index_d7];\n\n", d7_name(n));
				}
			}

			if ((x < 6 && y < 6) || (x > 1 && y > 1)) {
				if (n == C6 || n == F3 ) {
					fprintf(f, "\tflipped |= ((P >> 9) & 0x%016llxULL & O) | ((P << 9) & 0x%016llxULL & O);\n\n", BIT[n] << 9, BIT[n] >> 9);
				} else if (n == A6 || n == B6 || n == F1 || n == F2 ) {
					fprintf(f, "\tflipped |= ((P >> 9) & 0x%016llxULL & O);\n\n", BIT[n] << 9);
				} else if (n == C8 || n == C7 || n == H3 || n == G3) {
					fprintf(f, "\tflipped |= ((P << 9) & 0x%016llxULL & O);\n\n", BIT[n] >> 9);
				} else {
					fprintf(f, "\tindex_d9 = OUTFLANK[%d][(O & 0x%016llxULL)* 0x%016llxULL >> 57] & (P & 0x%016llxULL) * 0x%016llxULL >> 56;\n",
						d9_flip_index(n), d9_mask(n, 1, 6), 0x0101010101010101ULL, d9_mask(n, 0, 7), 0x0101010101010101ULL);
					fprintf(f, "\tindex_d9 = FLIPPED[%d][index_d9];\n", d9_flip_index(n));
					if ((shift = d9_shift_index(n)))fprintf(f, "\tflipped |= %s[index_d9 >> %d];\n\n", d9_name(n), shift);
					else fprintf(f, "\tflipped |= %s[index_d9];\n\n", d9_name(n));
				}
			}
		}
	if (0) {
			fprintf(f, "\tif (test_generator(flipped, P, O, %s)) {\n", s);
			fprintf(f, "\t\tabort();\n");
			fprintf(f, "\t}\n");
	}
		fprintf(f, "\n");
		fprintf(f, "\treturn flipped;\n");
		fprintf(f, "}\n\n");
	}

	fprintf(f, "/**\n");
	fprintf(f, " * Compute (zero-) flipped discs when plassing.\n");
	fprintf(f, " *\n");
	fprintf(f, " * @param P player's disc pattern.\n");
	fprintf(f, " * @param O opponent's disc pattern.\n");
 	fprintf(f, " * @return flipped disc pattern.\n");
	fprintf(f, " */\n");
	fprintf(f, "static unsigned long long flip_pass(const unsigned long long P, const unsigned long long O)\n{\n");
	fprintf(f, "\t(void) P; // useless code to shut-up compiler warning\n");
	fprintf(f, "\t(void) O;\n");
	fprintf(f, "\treturn 0;\n");
	fprintf(f, "}\n\n\n");

	fprintf(f, "/** Array of functions to compute flipped discs */\n");
	fprintf(f, "unsigned long long (*flip[])(const unsigned long long, const unsigned long long) = {\n");
	for (n = 0; n < 64; n++) {
		x = n % 8; s[0] = 'A' + x;
		y = n / 8; s[1] = '1' + y;
		if (n % 4 == 0) fprintf(f, "\t");
		fprintf(f, "flip_%s,", s);
		if (n % 4 == 3) fputc('\n', f); else fputc(' ', f);
	}
	fprintf(f, "\tflip_pass, flip_pass\n};\n\n");

	fclose(f);
}

