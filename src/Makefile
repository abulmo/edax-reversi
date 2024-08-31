#
# makefile to Compile Edax
#
# Copyright 1998 - 2024
# Richard Delorme
# Version 4.5
#

# Default settings
BIN = ../bin
LIBS = -lm
PGO = echo

ifeq ($(BUILD),)
	BUILD = optimize
endif

ifeq ($(ARCH),)
	ARCH = x64-modern
endif

ifeq ($(COMP),)
	COMP = icc
endif

ifeq ($(OS),)
	OS = linux
endif

ifeq ($(OS),win)
	OS = windows
endif

ifeq ($(CC),cc)
	CC = $(COMP)
endif


# gcc 4.x (x >= 7)
ifeq ($(COMP),gcc)
	CFLAGS = -std=c99 -pedantic -W -Wall -Wextra -pipe -D_GNU_SOURCE=1 -DUNICODE
	PGO_GEN = -fprofile-generate
	PGO_USE = -fprofile-correction -fprofile-use

	ifeq ($(BUILD),optimize)
		CFLAGS += -Ofast -fwhole-program -DNDEBUG
		LTOFLAG = -flto
	else
		CFLAGS += -O1 -g -DDEBUG
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -m64 -march=core-avx2 -DUSE_GAS_X64 -DPOPCOUNT -DLASTFLIP_HIGHCUT
	endif
	ifeq ($(ARCH),x64-avx512)
		CFLAGS += -m64 -march=skylake-avx512 -DUSE_GAS_X64 -DPOPCOUNT -DLASTFLIP_HIGHCUT
	endif
	ifeq ($(ARCH),x64-popcnt)
		CFLAGS += -m64 -mpopcnt -mtune=generic -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x64-k10)
		CFLAGS += -m64 -march=amdfam10 -DUSE_GAS_X64 -DPOPCOUNT -DMOVE_GENERATOR=MOVE_GENERATOR_BITSCAN
	endif
	ifeq ($(ARCH),x32-modern)
		CFLAGS += -mx32 -march=core-avx2 -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -m64 -mtune=generic -DUSE_GAS_X64
	endif
	ifeq ($(ARCH),x32)
		CFLAGS += -mx32 -mtune=generic -DUSE_GAS_X64
	endif
	ifeq ($(ARCH),x86-sse)
		CFLAGS += -m32 -march=pentium-m -mfpmath=sse -DUSE_GAS_X86 -DUSE_GAS_MMX -DhasSSE2
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer
		endif
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -m32 -march=i386 -mtune=generic -DUSE_GAS_X86 -DUSE_GAS_MMX
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer
		endif
	endif
	ifeq ($(ARCH),arm)
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer
		endif
	endif
	ifeq ($(ARCH),armv7)
		CFLAGS += -march=armv7-a
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer
		endif
	endif
	ifeq ($(ARCH),arm-neon)
		CFLAGS += -march=armv7-a+simd -mfloat-abi=softfp
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer
		endif
	endif
	ifeq ($(ARCH),arm-sve)
		CFLAGS += -march=armv8.2-a+sve
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer
		endif
	endif

	ifeq ($(OS),osx)
		CFLAGS += -mmacosx-version-min=10.4
		ifeq ($(ARCH),x86)
			CFLAGS += -msse2 -DhasSSE2
		endif
		ifeq ($(ARCH),arm)
			CFLAGS += -march=armv8.3-a
		endif
	endif
	ifeq ($(OS),android)
		CFLAGS += -DANDROID=1
	endif
	ifeq ($(OS),windows)
		CFLAGS += -D__USE_MINGW_ANSI_STDIO -DWINVER=0x0501
		ifneq (,$(findstring x86,$(ARCH)))
			CFLAGS += -DUSE_PTHREAD
		endif
	endif

endif

ifeq ($(COMP),gcc-old)
	CC = gcc
	CFLAGS = -std=c99 -pedantic -W -Wall -Wextra -pipe -D_GNU_SOURCE=1 -DUNICODE

	ifeq ($(BUILD),optimize)
		CFLAGS += -O3 -fwhole-program -DNDEBUG
	else
		CFLAGS += -O0 -g -DDEBUG
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -m64 -march=native -DUSE_GAS_X64 -DPOPCOUNT -DLASTFLIP_HIGHCUT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -m64 -mtune=generic -DUSE_GAS_X64 
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -m32 -mtune=generic -DUSE_GAS_X86 -DUSE_GAS_MMX
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer
		endif
	endif
	ifeq ($(ARCH),arm)
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer -DUSE_GCC_ARM
		endif
	endif
	ifeq ($(ARCH),armv7)
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer -march=armv7-a -mfpu=neon -DUSE_GCC_ARM
		endif
	endif

	ifeq ($(OS),osx)
		CFLAGS += -mmacosx-version-min=10.4 -mdynamic-no-pic
		ifeq ($(ARCH),x86)
			CFLAGS += -msse2 -DhasSSE2
		endif
	endif
	ifeq ($(OS),android)
		CFLAGS += -DANDROID=1
	endif
	ifeq ($(OS),windows)
		CFLAGS += -D__USE_MINGW_ANSI_STDIO
		ifeq ($(ARCH),x86)
			CFLAGS += -DUSE_PTHREAD
		endif
	endif

endif

# g++
ifeq ($(COMP),g++)
	CFLAGS = -x c++ -std=c++11 -pedantic -W -Wall -Wextra -pipe -D_GNU_SOURCE=1 -DUNICODE
	PGO_GEN = -fprofile-generate
	PGO_USE = -fprofile-correction -fprofile-use

	ifeq ($(BUILD),optimize)
		CFLAGS += -Ofast -fwhole-program -DNDEBUG
		LTOFLAG = -flto
	else
		CFLAGS += -O0 -g -DDEBUG
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -m64 -march=native -DUSE_GAS_X64 -DPOPCOUNT -DLASTFLIP_HIGHCUT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -m64 -mtune=generic -DUSE_GAS_X64 
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -m32 -mtune=generic -DUSE_GAS_X86 -DUSE_GAS_MMX
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer
		endif
	endif

	ifeq ($(OS),osx)
		CFLAGS += -mmacosx-version-min=10.4 -mdynamic-no-pic
		ifeq ($(ARCH),x86)
			CFLAGS += -msse2 -DhasSSE2
		endif
	endif
	ifeq ($(OS),windows)
		CFLAGS += -D__USE_MINGW_ANSI_STDIO -DWINVER=0x501
		ifeq ($(ARCH),x86)
			CFLAGS += -DUSE_PTHREAD
		endif
	endif

endif

#icc
ifeq ($(COMP),icc)
	CFLAGS = -std=c99 -Wall -Wcheck -wd2259 -D_GNU_SOURCE=1 -DUNICODE -Qoption,cpp,--unicode_source_kind,"UTF-8"
	PGO_GEN = -prof_gen
	PGO_USE = -prof_use -wd11505

	ifeq ($(BUILD),optimize)
		CFLAGS += -Ofast -DNDEBUG -rcd -DRCD=0.0 -ansi-alias
	else
		CFLAGS += -O0 -g -DDEBUG -rcd -DRCD=0.0
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -m64 -march=core-avx2 -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x64-avx512)
		CFLAGS += -m64 -march=skylake-avx512 -DUSE_GAS_X64 -DPOPCOUNT -DLASTFLIP_HIGHCUT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -m64 -DUSE_GAS_X64
	endif
	ifeq ($(ARCH),x32-modern)
		CFLAGS += -m64 -ipo -auto-ilp32 -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -m32 -DUSE_GAS_X86
	endif
endif

#pcc
ifeq ($(COMP),pcc)
	CFLAGS = - -D_GNU_SOURCE=1 -DUNICODE

	ifeq ($(BUILD),optimize)
		CFLAGS += -O4 -DNDEBUG
	else
		CFLAGS += -O0 -g -DDEBUG
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -DUSE_GAS_X64 -DPOPCOUNT -DLASTFLIP_HIGHCUT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -DUSE_GAS_X64
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -DUSE_GAS_X86
	endif
endif

#clang
ifeq ($(COMP),clang)
	CFLAGS = -std=c99 -pedantic -W -Wall -D_GNU_SOURCE=1 -DUNICODE
	PGO_GEN = -fprofile-instr-generate
	PGO_USE = -fprofile-instr-use=edax.profdata
	PGO = llvm-profdata merge -output=edax.profdata $(BIN)/*.profraw

	ifeq ($(BUILD),optimize)
		CFLAGS += -O3 -ffast-math -fomit-frame-pointer -DNDEBUG
		LTOFLAG = -flto
	else
		CFLAGS += -O0 -g -DDEBUG
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -m64 -march=core-avx2 -DUSE_GAS_X64 -DPOPCOUNT -DLASTFLIP_HIGHCUT
	endif
	ifeq ($(ARCH),x64-avx512)
		CFLAGS += -m64 -march=skylake-avx512 -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -m64 -DUSE_GAS_X64 
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -m32 -DUSE_GAS_X86 -DUSE_GAS_MMX
	endif

	ifeq ($(OS),osx)
		CFLAGS += -mmacosx-version-min=10.4 -mdynamic-no-pic
		ifeq ($(ARCH),x86)
			CFLAGS += -msse2 -DhasSSE2
		endif
		ifeq ($(ARCH),arm)
			CFLAGS += -march=armv8.3-a
		endif
	endif
endif

#EXE & LIBS
ifeq ($(OS),linux)
	EXE = lEdax-$(ARCH)
 	LIBS += -lrt -lpthread
endif
ifeq ($(OS),android)
	EXE = aEdax
endif
ifeq ($(OS),windows)
	EXE = wEdax-$(ARCH).exe
	LIBS += -lws2_32
	ifneq (,$(findstring x86,$(ARCH)))
		LIBS += -Bstatic -Wl,-Bstatic,-lpthread
	endif
endif
ifeq ($(OS),osx)
	EXE = mEdax-$(ARCH)
	LIBS += -lpthread
endif

ifneq (,$(findstring x64,$(ARCH))$(findstring x32,$(ARCH)))
	CFLAGS += -DHAS_CPU_64
endif


#SRC
SRC= bit.c board.c move.c hash.c ybwc.c eval.c endgame.c midgame.c root.c search.c \
book.c opening.c game.c base.c bench.c perft.c obftest.c util.c event.c histogram.c \
stats.c options.c play.c ui.c edax.c cassio.c gtp.c ggs.c nboard.c xboard.c main.c   

# RULES
help:
	@echo ""
	@echo "To compile Edax:"
	@echo ""
	@echo "make target [ARCH=cpu] [COMP=compiler] [OS=os]"
	@echo ""
	@echo "Targets:"
	@echo "   build*     Build optimized version"
	@echo "   pgo-build  Build PGO-optimized version"
	@echo "   release    Cross compile for linux/windows/mac (from fedora only)"
	@echo "   debug      Build debug version."
	@echo "   clean      Clean up."
	@echo "   help       Print this message"
	@echo ""
	@echo "Archs:"
	@echo " x64-modern*  x64 with popcount"
	@echo " x64          x64"
	@echo " x32-modern   x32 with popcount"
	@echo " x32          x32"
	@echo " x86          x86"
	@echo " arm          arm v5 & up"
	@echo " armv7        arm v7-a"
	@echo ""
	@echo "Compilers:"
	@echo "   gcc        GNU C compiler version >= 4.6"
	@echo "   gcc-old    GNU C compiler version >= 3.0"
	@echo "   g++        GNU C++ compiler version >= 4.6"
	@echo "   icc*       Intel C Compiler"
	@echo "   clang      Clang Compiler"
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
	$(CC) $(CFLAGS) $(LTOFLAG) all.c -s -o $(BIN)/$(EXE) $(LIBS)

source:
	$(CC) $(CFLAGS) -S all.c

pgo-build:
	@echo "building edax with pgo..."
	$(MAKE) clean
	$(CC) $(CFLAGS) $(LTOFLAG) $(PGO_GEN) all.c -o $(BIN)/$(EXE) $(LIBS)
	cd $(BIN); echo -e 'count games 10\ncount positions 9\n' | ./$(EXE) 
	cd $(BIN); ./$(EXE) -l 60 -solve ../problem/fforum-20-39.obf
	cd $(BIN); ./$(EXE) -l 18 -auto-store on -auto-start on -repeat 2 -auto-quit on -mode 2 -book-file book.pgo
	rm -f ../bin/book.pgo ../bin/book.pgo.store
	$(PGO)	
	$(CC) $(CFLAGS) $(LTOFLAG) $(PGO_USE) all.c -s -o $(BIN)/$(EXE) $(LIBS)

pgo-rebuild:
	@echo "rebuilding edax with pgo..."
	$(CC) $(CFLAGS) $(LTOFLAG) $(PGO_USE) all.c -s -o $(BIN)/$(EXE) $(LIBS)

prof:
	@echo "rebuilding edax for profiling..."
	$(CC) $(CFLAGS) $(LTOFLAG) all.c -pg -o $(BIN)/$(EXE) $(LIBS) # -inline-level=0 -lprofiler

debug:
	make ARCH=$(ARCH) COMP=$(COMP) OS=$(OS) BUILD=debug prof

release:
	$(MAKE) pgo-build ARCH=x64 OS=linux COMP=gcc 
	$(MAKE) build ARCH=x64 OS=windows COMP=gcc CC='x86_64-w64-mingw32-gcc'
	$(MAKE) build ARCH=x86 OS=windows COMP=gcc CC='i686-w64-mingw32-gcc'
	$(MAKE) build ARCH=armv7 OS=android COMP=gcc CC='arm-linux-androideabi-gcc --sysroot=$SYSROOT'
	$(MAKE) clean
	$(MAKE) build ARCH=x64 OS=osx COMP=gcc-old CC=i686-apple-darwin10-gcc

android:
	ndk-build NDK_PROJECT_PATH=. NDK_APPLICATION_MK=./Application.mk # NDK_DEBUG=1

macuniversal:
	$(MAKE) build ARCH=x86 OS=osx COMP=clang
	$(MAKE) build ARCH=x64 OS=osx COMP=clang
	lipo -create -arch i686 ../bin/mEdax-x86 -arch x86_64 ../bin/mEdax-x64 -arch arm64 ../bin/mEdax-arm -output ../bin/mEdax
	rm -f ../bin/mEdax-x86 ../bin/mEdax-x64

clean:
	rm -f pgopti* *.dyn all.gc* *~ *.o generate_flip generate_count_flip *.prof*

noip:
	$(CC) -g $(CFLAGS) $(LTOFLAG) $(SRC) -o $(BIN)/$(EXE) $(LIBS)

code:
	$(CC) $(CFLAGS) $(LTOFLAG) generate_flip.c -o generate_flip
	$(CC) $(CFLAGS) $(LTOFLAG) generate_count_flip.c -o generate_count_flip
	generate_flip
	generate_count_flip

default:
	help

