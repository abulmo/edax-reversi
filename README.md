# Edax

Edax is a very strong othello program. Its main features are:
- fast bitboard based & multithreaded engine.
- accurate midgame-evaluation function.
- opening book learning capability.
- text based rich interface.
- multi-protocol support to connect to graphical interfaces or play on Internet (GGS).
- multi-OS support to run under MS-Windows, Linux and Mac OS X.

## Installation
From [the release section of github](https://github.com/abulmo/edax-reversi/releases), you must 7unzip both an executable of your favorite OS and the source files.
Only 64 bit executable are provided.

## Build

```sh
cd bin
make -C ../src pgo-build ARCH=native
```

## Document
You can extract the comments from the source code using Doxygen:

```sh
cd src
doxygen
open ../doc/html/index.html
```

## Version 4.7
version 4.7 is an evolution of version 4.6 that:
 - Fixes bugs
 - Clean the code. Fix spelling mistake in the comments and variable names, etc.
 - Enhance the speed by about 20%, particularly on multi-threaded run.
 
## Speed comparison:
For all the tests I used the command edax-VERSION -l 60 -h 24 -n 32 -solve problem/fforum-40-59.obf -q. The test are done on a PC with a Ryzen 9 5950x running at 4.2Ghz.
I repeated the test 5 time & put the median time here:

| VERSION |   4.0   |   4.4   |   4.6   |  4.5.5  |   4.7   |
|---------|---------|---------|---------|---------|---------|
|         | 1:07.92 | 0:43.45 | 0:29.89 | 0:24.77 | 0:22.61 |
|---------|---------|---------|---------|---------|---------|



