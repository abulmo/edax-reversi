<<<<<<< HEAD
# Edax

Edax is a very strong othello program. Its main features are:
- fast bitboard based & multithreaded engine.
- accurate midgame-evaluation function.
- opening book learning capability.
- text based rich interface.
- multi-protocol support to connect to graphical interfaces or play on Internet (GGS).
- multi-OS support to run under MS-Windows, Linux and Mac OS X.

## Installation
From [the release section of github](https://github.com/abulmo/edax-reversi/releases), you must 7unzip both an executable of your favorite OS, and the evaluation weights (data/eval.dat) in the same directory.
Only 64 bit executable with popcount support are provided.

## Run

### local

```sh
mkdir -p bin
cd src

# e.g. OS X sample
make build ARCH=x64 COMP=gcc OS=osx
cd ..
./bin/mEdax
```

### docker

```sh
docker build . -t edax
docker run --name "edax" -v "$(pwd)/:/home/edax/" -it edax

cd /home/edax/
mkdir -p bin
cd src
make build ARCH=x64 COMP=gcc OS=linux

cd ..
curl -OL https://github.com/abulmo/edax-reversi/releases/download/v4.4/eval.7z # e.g. use v4.4 eval.dat
7z x eval.7z

./bin/lEdax-x64
```

## Document

```sh
cd src
doxygen
open ../doc/html/index.html
```
=======
# edax-reversi-AVX
Automatically exported from code.google.com/p/okuharaandroid-edax-reversi

This is SSE/AVX optimized version of Edax 4.4.0. Functionally equivalent to the parent project, provided no bugs are introduced.

64 bit version solves fforum-20-39 7% to 9% faster than the original 4.4.0 on my test. Thanks to AVX2, x64-modern build runs 14% faster on Haswell. 32 bit version runs 9% (Core2) to 20% (Athlon) faster than the original.

All SSE/AVX/MMX stuff belongs to board.c are moved into separate board_sse.c (for x64 and x86) and board_mmx.c (for x86) files.

I used GCC 4.7.2, Athlon X4 605e, Windows 8 (64) / XP (32), Clang 3.4, Core i5-4260U (Haswell), OSX 10.9.4 and Clang 1.7, Core2, OSX 10.6.8 for the benchmark.

##1. Mobility (board_sse.c, board_mmx.c)
###1.1 new SSE2 version of get_moves
Diagonals are SIMD'd using vertical mirroring by bswap.

    Athlon -get_moves_sse
    problem\fforum-20-39.obf: 111349635 nodes in 0:07.998 (13922185 nodes/s).
    mobility: 81.10 < 81.28 +/- 0.17 < 82.03
    Athlon +get_moves_sse
    problem\fforum-20-39.obf: 111349635 nodes in 0:07.889 (14114544 nodes/s).
    mobility: 71.08 < 71.72 +/- 0.34 < 73.53
    Core2 -get_moves_sse
    problem/fforum-20-39.obf: 111349635 nodes in 0:10.180 (10938078 nodes/s).
    mobility: 78.06 < 78.18 +/- 0.08 < 78.41
    Core2 +get_moves_sse
    problem/fforum-20-39.obf: 111349635 nodes in 0:09.978 (11159514 nodes/s).
    mobility: 60.84 < 61.19 +/- 0.13 < 61.47

###1.2 can_move
Now calls SIMD'd get_moves for x86/x64 build.

##2. Stability (board.c, board_sse.c, board_mmx.c)
###2.1 get_full_lines_h, get_full_lines_v
get_full_lines for horizontal and vertical are simplified. The latter is compiled into rotation instrunction.

###2.2 rearranged loop
The last while loop is rearranged not to call bit_count in case stable == 0.

###2.3 new SSE2 version with bswap and pcmpeqb
    Athlon -get_stability_sse
    stability: 90.10 < 90.28 +/- 0.24 < 91.20
    Athlon +get_stability_sse
    stability: 81.59 < 81.93 +/- 0.73 < 86.25
    Core2 -get_stability_sse
    stability: 79.24 < 79.39 +/- 0.15 < 79.93
    Core2 +get_stability_sse
    stability: 71.80 < 71.85 +/- 0.06 < 72.07

###2.4 get_corner_stability
Kindergarten version eliminates bit_count call.

###2.5 find_edge_stable
Loop optimization and flip using carry propagation. One time execution but affect total solving time.

##3. eval.c
Switch cases and table sizes are optimized. This small change slightly imploves endgame solving. I guess this should improve midgame too.

    Athlon -eval
    problem\fforum-20-39.obf: 111349635 nodes in 0:07.954 (13999200 nodes/s).
    Athlon +eval
    problem\fforum-20-39.obf: 111349635 nodes in 0:07.889 (14114544 nodes/s).
    Core2 -eval
    problem/fforum-20-39.obf: 111349635 nodes in 0:10.161 (10958531 nodes/s).
    Core2 +eval
    problem/fforum-20-39.obf: 111349635 nodes in 0:09.978 (11159514 nodes/s).

##4. hash.c
I think hash->data.move[0] on line 677 should be hash->data.move[1].

##5. board_symetry, board_unique (board.c, board_sse.c)
SSE optimization and mirroring reduction. (Not used in solving game)

##6. AVX2 versions (x64-modern build only)
In many cases AVX2 version is simplest, thanks to variable shift instructions (although they are 3 micro-op instructions).

Benchmarks are on Core i5-4260U (Haswell) 1.4GHz (TB 2.7GHz) single thread.

    4.4.0 original x64-modern clang
    problem/fforum-20-39.obf: 111349635 nodes in 0:05.726 (19446321 nodes/s).
    +optimizations 1-5 above, no-avx2
    problem/fforum-20-39.obf: 111349635 nodes in 0:05.342 (20844185 nodes/s).
    +get_moves (board_sse.c)
    problem/fforum-20-39.obf: 111349635 nodes in 0:05.142 (21654927 nodes/s).
    +flip_avx.c
    problem/fforum-20-39.obf: 111349635 nodes in 0:04.946 (22513068 nodes/s).
    +count_last_flip_sse.c
    problem/fforum-20-39.obf: 111349635 nodes in 0:04.906 (22696624 nodes/s).

##7. makefile
gcc-old, x86 build should be -m32, not -m64. Some flags and defines added for optimization.
>>>>>>> b9d48c1 (Create README.md)
