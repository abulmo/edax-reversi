#
# makefile to Compile Edax
#
# Copyright 1998 - 2024
# Richard Delorme
# Version 4.6
#

# Default settings
BIN = ../bin
LIBS = -lm
PGO = echo

ifeq ($(BUILD),)
	BUILD = optimize
endif

ifeq ($(OS),)
	OS = linux
endif

ifeq ($(OS),win)
	OS = windows
endif

ifeq ($(CC),)
	CC=clang
endif

ifeq ($(CC),cc)
	CC=clang
endif

#clang
ifeq ($(CC),clang)
	CFLAGS = -std=c17 -pedantic -W -Wall -Wno-invalid-source-encoding -D_GNU_SOURCE=1
	PGO_GEN = -fprofile-instr-generate
	PGO_USE = -fprofile-instr-use=edax.profdata
	PGO = llvm-profdata merge -output=edax.profdata $(BIN)/*.profraw


	ifeq ($(OS),win)
		OS = windows
	endif

	ifeq ($(BUILD),optimize)
		CFLAGS += -O3 -flto -ffast-math -fomit-frame-pointer -DNDEBUG
	endif
	ifeq ($(BUILD),profile)
		CFLAGS += -O2 -g -DNDEBUG -fno-inline-functions
	endif	
	ifeq ($(BUILD),coverage)
		CFLAGS += -O2 -fprofile-instr-generate -fcoverage-mapping -mllvm -runtime-counter-relocation -DNDEBUG
	endif	
	ifeq ($(BUILD),debug)
		CFLAGS += -O0 -g -DDEBUG
	endif

	CFLAGS += -march=$(ARCH)

	ifeq ($(OS),osx)
		CFLAGS += -mdynamic-no-pic
	endif
endif

# gcc
ifeq ($(CC),gcc)
	CFLAGS = -std=c17 -pedantic -W -Wall -Wextra -pipe -D_GNU_SOURCE=1
	PGO_GEN = -fprofile-generate
	PGO_USE = -fprofile-correction -fprofile-use
	
	ifeq ($(BUILD),optimize)
		CFLAGS += -Ofast -fwhole-program -flto=auto -fomit-frame-pointer -DNDEBUG
	endif
	ifeq ($(BUILD),profile)
		CFLAGS += -O2 -inline-level=0 -g -DNDEBUG
	endif	
	ifeq ($(BUILD),coverage)
		CFLAGS += -O2 -fcov -g -DNDEBUG
	endif	
	ifeq ($(BUILD),debug)
		CFLAGS += -O0 -g -DDEBUG
	endif

	CFLAGS += -march=$(ARCH)

	ifeq ($(OS),osx)
		CFLAGS += -mdynamic-no-pic
	endif
	ifeq ($(OS),windows)
		CFLAGS += -D__USE_MINGW_ANSI_STDIO
	endif

endif

#EXE & LIBS
ifeq ($(OS),linux)
	EXE = lEdax-$(ARCH)
 	LIBS += -lrt
endif
ifeq ($(OS),android)
	EXE = aEdax-$(ARCH)
endif
ifeq ($(OS),windows)
	EXE = wEdax-$(ARCH).exe
	LIBS += -lws2_32
endif
ifeq ($(OS),osx)
	EXE = mEdax-$(ARCH)
endif

#SRC
SRC= bit.c board.c move.c crc32c.c hash.c ybwc.c eval.c endgame.c midgame.c root.c search.c \
book.c opening.c game.c base.c perft.c obftest.c util.c event.c histogram.c \
stats.c options.c play.c ui.c edax.c cassio.c gtp.c ggs.c nboard.c xboard.c main.c   

# RULES
help:
	@echo ""
	@echo "To compile Edax:"
	@echo ""
	@echo "[CC=compiler] make target [ARCH=cpu] [OS=os]"
	@echo ""
	@echo "Targets:"
	@echo "   build      Build optimized version"
	@echo "   pgo-build  Build PGO-optimized version"
	@echo "   debug      Build debug version."
	@echo "   clean      Clean up."
	@echo "   help*      Print this message"
	@echo ""
	@echo "Archs:"
	@echo " x86-64-v4       x64 with sse2, avx, sse4 & popcount & avx2 & avx512 support"
	@echo " x86-64-v3       x64 with sse2, avx, sse4 & popcount & avx2 support"
	@echo " x86-64-v2       with sse2, avx, sse4 & popcount support"
	@echo " x86-64          x64 with sse2 support"
	@echo " armv8-a         ARM v8-a"
	@echo " armv8.?-a       ARM v8.?.a with ? = 1, 2, 3, 4, 5, 6, 7 or 8"
	@echo " armv9-a         ARM v9-a"
	@echo " native          Your cpu"
	@echo ""
	@echo "Oses:"
	@echo "   linux*"
	@echo "   windows"
	@echo "   osx"
	@echo "   android"
	@echo ""
	@echo "* default setting"

build:
	@echo "building edax..."
	$(CC) $(CFLAGS) $(DFLAGS) all.c -o $(BIN)/$(EXE) $(LIBS)

pgo-build:
	@echo "building edax with pgo..."
	$(MAKE) clean
	$(CC) $(CFLAGS) $(PGO_GEN) $(DFLAGS) all.c -o $(BIN)/$(EXE) $(LIBS)
	cd $(BIN); ./$(EXE) -l 60 -bench 10 -n 1
#	cd $(BIN); ./$(EXE) -l 60 -solve problem/fforum-20-39.obf -n 4
#	cd $(BIN); ./$(EXE) -l 18 -auto-store on -auto-start on -repeat 2 -auto-quit on -mode 2 -book-file book.pgo
	rm -f ../bin/book.pgo ../bin/book.pgo.store
	$(PGO)	
#	$(CC) $(CFLAGS) $(PGO_USE) $(DFLAGS) -S all.c
	$(CC) $(CFLAGS) $(PGO_USE) $(DFLAGS) all.c -o $(BIN)/$(EXE) $(LIBS)

pgo-rebuild:
	@echo "rebuilding edax with pgo..."
	$(CC) $(CFLAGS) $(PGO_USE) all.c -o $(BIN)/$(EXE) $(LIBS)

prof:
	@echo "building edax for profiling..."
	make ARCH=$(ARCH) COMP=$(COMP) OS=$(OS) BUILD=profile build

cov:
	@echo "building edax for coverage..."
	make ARCH=$(ARCH) COMP=$(COMP) OS=$(OS) BUILD=coverage build

debug:
	make ARCH=$(ARCH) COMP=$(COMP) OS=$(OS) BUILD=debug build

android:
	$(MAKE) build ARCH=armv8 OS=android COMP=gcc CC='arm-linux-androideabi-gcc'

clean:
	rm -f pgopti* *.dyn *.gc* *~ *.o *.prof*

default:
	help
