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

=======
# edax-reversi-AVX
Automatically exported from code.google.com/p/okuharaandroid-edax-reversi

Edax is a strong othello program. Its main features are:

    fast bitboard based & multithreaded engine.
    accurate midgame-evaluation function.
    opening book learning capability.
    text based rich interface.
    multi-protocol support to connect to graphical interfaces or play on Internet (GGS).
    multi-OS support to run under MS-Windows, Linux and Mac OS X.

>>>>>>> 81dec96 (Kindergarten last flip for arm32; MSVC arm Windows build (not tested))
This is SSE/AVX optimized version of Edax 4.4.0. Functionally equivalent to the parent project, provided no bugs are introduced.

Thanks to AVX2, x64-modern build solves fforum-40-59.obf 60% faster than official edax-4.4 on Haswell, and runs level 30 autoplay 80% faster.

See http://www.amy.hi-ho.ne.jp/okuhara/bitboard.htm and http://www.amy.hi-ho.ne.jp/okuhara/edaxopt.htm for optimization details in Japanese.

## 1. Mobility (board_sse.c, board_mmx.c)

### 1.1 new SSE2 version of get_moves
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

### 1.2 can_move
Now calls SIMD'd get_moves for x86/x64 build.

## 2. Stability (board.c, board_sse.c, board_mmx.c)

### 2.1 get_full_lines_h, get_full_lines_v
get_full_lines for horizontal and vertical are simplified. The latter is compiled into rotation instrunction.

### 2.2 rearranged loop
The last while loop is rearranged not to call bit_count in case stable == 0.

### 2.3 new SSE2 version with bswap and pcmpeqb
    Athlon -get_stability_sse
    stability: 90.10 < 90.28 +/- 0.24 < 91.20
    Athlon +get_stability_sse
    stability: 81.59 < 81.93 +/- 0.73 < 86.25
    Core2 -get_stability_sse
    stability: 79.24 < 79.39 +/- 0.15 < 79.93
    Core2 +get_stability_sse
    stability: 71.80 < 71.85 +/- 0.06 < 72.07

### 2.4 get_corner_stability
Kindergarten version eliminates bit_count call.

### 2.5 find_edge_stable
Loop optimization and flip using carry propagation. One time execution but affect total solving time.

## 3. eval.c (4.4.5)
Eval feature calculation using SSE2 / AVX2 (now in eval_sse.c) improves midgame by 15-30% and endgame by 8-12%.
Restoring eval from backup instead of rewinding.
eval_open (one time execution) is also optimized.

## 4. hash.c
I think hash->data.move[0] on line 677 should be hash->data.move[1].

## 5. board_symetry, board_unique (board.c, board_sse.c)
SSE optimization and mirroring reduction. (Not used in solving game)

## 6. endgame_sse.c (4.4.7)
Keep more variables in SSE registers.  SSE optimized count_last_flip.  Parity sort by shuffle.

## 7. board_get_hash_code (4.5.0)
Changed to use CRC32c.  This enables hardware acceleration on modern build.

## 8. AVX2 versions (x64-modern build only)
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

## 9. makefile
gcc-old, x86 build should be -m32, not -m64. Some flags and defines added for optimization.
<<<<<<< HEAD
>>>>>>> b9d48c1 (Create README.md)
=======
>>>>>>> 81dec96 (Kindergarten last flip for arm32; MSVC arm Windows build (not tested))
