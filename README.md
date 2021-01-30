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
