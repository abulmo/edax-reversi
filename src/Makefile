#
# makefile to Compile Edax
#
# Copyright 1998 - 2012
# Richard Delorme
# Version 4.3
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
	CFLAGS = -std=c99 -pedantic -W -Wall -Wextra -pipe -D_GNU_SOURCE=1
	PGO_GEN = -fprofile-generate
	PGO_USE = -fprofile-correction -fprofile-use
	
	ifeq ($(BUILD),optimize)
		# modify for libedax by sensuikan1973.20121/1/21
		CFLAGS += -Ofast -flto -DNDEBUG
	else
		CFLAGS += -O0 -g -DDEBUG
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -m64 -march=native -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x32-modern)
		CFLAGS += -mx32 -march=native -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -m64 -mtune=generic -DUSE_GAS_X64 
	endif
	ifeq ($(ARCH),x32)
		CFLAGS += -mx32 -march=native -DUSE_GAS_X64
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -m32 -mtune=generic -DUSE_GAS_X86
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer
		endif
	endif
	ifeq ($(ARCH),ARM)
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer -DUSE_GCC_ARM
		endif
	endif
	ifeq ($(ARCH),ARMv7)
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer -march=armv7-a -mfpu=neon -DUSE_GCC_ARM
		endif
	endif

	ifeq ($(OS),osx)
		CFLAGS += -mmacosx-version-min=10.4
	endif
	ifeq ($(OS),windows)
		CFLAGS += -D__USE_MINGW_ANSI_STDIO -DWINVER=0x501
		ifeq ($(ARCH),x86)
			CFLAGS += -DUSE_PTHREAD
		endif
	endif

endif

ifeq ($(COMP),gcc-old)
	CFLAGS = -std=c99 -pedantic -W -Wall -Wextra -pipe -D_GNU_SOURCE=1
	
	ifeq ($(BUILD),optimize)
		# modify for libedax by sensuikan1973.20121/1/21
		CFLAGS += -O3 -DNDEBUG
	else
		CFLAGS += -O0 -g -DDEBUG
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -m64 -march=native -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -m64 -mtune=generic -DUSE_GAS_X64 
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -m64 -mtune=generic -DUSE_GAS_X86
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer
		endif
	endif
	ifeq ($(ARCH),ARM)
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer -DUSE_GCC_ARM
		endif
	endif
	ifeq ($(ARCH),ARMv7)
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer -march=armv7-a -mfpu=neon -DUSE_GCC_ARM
		endif
	endif

	ifeq ($(OS),osx)
		CFLAGS += -mmacosx-version-min=10.4
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
	CFLAGS = -x c++ -std=c++11 -pedantic -W -Wall -Wextra -pipe -D_GNU_SOURCE=1
	PGO_GEN = -fprofile-generate
	PGO_USE = -fprofile-correction -fprofile-use

	ifeq ($(BUILD),optimize)
		# modify for libedax by sensuikan1973.20121/1/21
		CFLAGS += -Ofast -flto -DNDEBUG
	else
		CFLAGS += -O0 -g -DDEBUG
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -m64 -march=native -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -m64 -mtune=generic -DUSE_GAS_X64 
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -m32 -mtune=generic -DUSE_GAS_X86
		ifeq ($(BUILD),optimize)
			CFLAGS += -fomit-frame-pointer
		endif
	endif

	ifeq ($(OS),osx)
		CFLAGS += -mmacosx-version-min=10.4 -mdynamic-no-pic
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
	CFLAGS = -std=c99 -Wall -Wcheck -wd2259 -wd913 -D_GNU_SOURCE=1
	PGO_GEN = -prof_gen
	PGO_USE = -prof_use -wd11505

	ifeq ($(BUILD),optimize)
		CFLAGS += -Ofast -DNDEBUG -rcd -DRCD=0.0 -ansi-alias
	else
		CFLAGS += -O0 -g -DDEBUG -rcd -DRCD=0.0
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -m64 -xHOST -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -m64 -xHost -DUSE_GAS_X64 
	endif
	ifeq ($(ARCH),x32-modern)
		CFLAGS += -m64 -xHost -ipo -auto-ilp32 -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -m32 -DUSE_GAS_X86
	endif
endif

#pcc
ifeq ($(COMP),pcc)
	CFLAGS = - -D_GNU_SOURCE=1

	ifeq ($(BUILD),optimize)
		CFLAGS += -O4 -DNDEBUG
	else
		CFLAGS += -O0 -g -DDEBUG
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -DUSE_GAS_X64 -DPOPCOUNT
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
	CFLAGS = -std=c99 -pedantic -W -Wall -D_GNU_SOURCE=1 -Wno-invalid-source-encoding
	PGO_GEN = -fprofile-instr-generate
	PGO_USE = -fprofile-instr-use=edax.profdata
	PGO = llvm-profdata merge -output=edax.profdata $(BIN)/*.profraw

	ifeq ($(BUILD),optimize)
		CFLAGS += -O3 -flto -ffast-math -fomit-frame-pointer -DNDEBUG -fuse-ld=gold
	else
		CFLAGS += -O0 -g -DDEBUG
	endif

	ifeq ($(ARCH),x64-modern)
		CFLAGS += -m64 -march=native -DUSE_GAS_X64 -DPOPCOUNT
	endif
	ifeq ($(ARCH),x64)
		CFLAGS += -m64 -DUSE_GAS_X64 
	endif
	ifeq ($(ARCH),x86)
		CFLAGS += -m32 -DUSE_GAS_X86
	endif

	ifeq ($(OS),osx)
		CFLAGS += -mmacosx-version-min=10.4 -mdynamic-no-pic
	endif
endif

#EXE & LIBS
LIBFLAGS = -DLIB_BUILD -fPIC -shared
ifeq ($(OS),linux)
	EXE = lEdax-$(ARCH)
	# add for libedax by lavox.2018/1/16
	LIB = libedax.so
 	LIBS += -lrt -lpthread
endif
ifeq ($(OS),android)
	EXE = aEdax
endif
ifeq ($(OS),windows)
	EXE = wEdax-$(ARCH).exe
	# add for libedax by lavox.2018/1/16
	LIB = libedax-$(ARCH).dll
	LIBFLAGS += -DDLL_BUILD
	LIBS += -lws2_32
	ifeq ($(ARCH),x86)
		LIBS += -lpthread
	endif
	# add for libedax by lavox.2018/1/16
endif
ifeq ($(OS),osx)
	EXE = mEdax
	# add for libedax by lavox.2018/1/16
	LIB = libedax.dylib
	LIBS += -lpthread
endif

ifeq ($(ARCH),x64)
	CFLAGS += -DHAS_CPU_64
endif
ifeq ($(ARCH),x64-modern)
	CFLAGS += -DHAS_CPU_64
endif
ifeq ($(ARCH),x32)
	CFLAGS += -DHAS_CPU_64
endif
ifeq ($(ARCH),x32-modern)
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
	@echo "   libbuiild  Build library"
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
	@echo " ARM          ARM v5 & up"
	@echo " ARMv7        ARM v7-a"
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
	$(CC) $(CFLAGS) all.c -o $(BIN)/$(EXE) $(LIBS)

# add for libedax by lavox. 2018/1/16 
libbuild:
	$(CC) $(CFLAGS) $(LIBFLAGS) all.c -o $(BIN)/$(LIB) $(LIBS)

pgo-build:
	@echo "building edax with pgo..."
	$(MAKE) clean
	$(CC) $(CFLAGS) $(PGO_GEN) all.c -o $(BIN)/$(EXE) $(LIBS)
	cd $(BIN); echo -e 'count games 10\ncount positions 9\n' | ./$(EXE) 
	cd $(BIN); ./$(EXE) -l 60 -solve problem/fforum-20-39.obf
	cd $(BIN); ./$(EXE) -l 18 -auto-store on -auto-start on -repeat 2 -auto-quit on -mode 2 -book-file book.pgo
	rm -f ../bin/book.pgo ../bin/book.pgo.store
	$(PGO)	
	$(CC) $(CFLAGS) $(PGO_USE) all.c -s -o $(BIN)/$(EXE) $(LIBS)

pgo-rebuild:
	@echo "rebuilding edax with pgo..."
	$(CC) $(CFLAGS) $(PGO_USE) all.c -s -o $(BIN)/$(EXE) $(LIBS)

prof:
	@echo "rebuilding edax for profiling..."
	$(CC) $(CFLAGS) all.c -g -inline-level=0 -o $(BIN)/$(EXE) $(LIBS) -lprofiler

debug:
	make ARCH=$(ARCH) COMP=$(COMP) OS=$(OS) BUILD=debug build

release:
	$(MAKE) pgo-build ARCH=x64 OS=linux COMP=gcc 
	$(MAKE) build ARCH=x64 OS=windows COMP=gcc CC='x86_64-w64-mingw32-gcc'
	$(MAKE) build ARCH=x86 OS=windows COMP=gcc CC='i686-w64-mingw32-gcc'
	$(MAKE) build ARCH=ARMv7 OS=android COMP=gcc-old CC='arm-linux-androideabi-gcc'
	$(MAKE) clean
	$(MAKE) build ARCH=x64 OS=osx COMP=gcc-old CC=i686-apple-darwin10-gcc

android:
	$(MAKE) build ARCH=ARMv7 OS=android COMP=gcc-old CC='arm-linux-androideabi-gcc'

clean:
	rm -f pgopti* *.dyn all.gc* *~ *.o generate_flip generate_count_flip *.prof*

noip:
	$(CC) -g $(CFLAGS) $(SRC) -o $(BIN)/$(EXE) $(LIBS)

code:
	$(CC) $(CFLAGS) generate_flip.c -o generate_flip
	$(CC) $(CFLAGS) generate_count_flip.c -o generate_count_flip
	generate_flip
	generate_count_flip

default:
	help

