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
make pgo-build ARCH=armv8-5a COMP=clang OS=osx
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
make build ARCH=x86-64-v3 COMP=clang OS=linux

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
## version 4.6
version 4.6 is an evolution of version 4.4 that tried to incorporate changes made by Toshihiko Okuhara in version 4.5.3 and :
 - keep the code encapsulated: I revert many pieces of code from version 4.5.3 with manually inlined code.
 - remove assembly code (intrinsics are good enough)
 - make some changes easily reversible with a macro switch (USE_SIMD, USE_SOLID, etc.)
 - remove buggy code and/or buggy file path.
 - disable code (#if 0) that I found too slow on my cpu.
 - make soft CRC32c behave the same as the hardware CRC32c (version 4.5.3 is buggy here).
 - the code switch from c99 to c17 and use stdatomic.h threads.h (if available) stdalign.h
 - remove bench.c: most of the functions get optimized out and could not be measured.
 - support only 64 bit OSes. 

## makefile
the major change is that the ARCH options are no longer the same, as they are too many possible options to enable avx2, avx512, CRC32c, etc.
Use make -help for a list of options. 


