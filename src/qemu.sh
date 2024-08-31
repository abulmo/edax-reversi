<<<<<<< HEAD
<<<<<<< HEAD
if [ "$1" = arm32 ]
then
make build OS=linux ARCH=arm-neon COMP=gcc CC=arm-linux-gnueabi-gcc
cd ../bin
qemu-arm -L /usr/arm-linux-gnueabi ./lEdax-arm-neon -n 1 -l 60 -solve ../problem/fforum-20-39.obf
elif [ "$1" = sve ]
then
make build OS=linux ARCH=arm-sve COMP=gcc CC=aarch64-linux-gnu-gcc
cd ../bin
qemu-aarch64 -L /usr/aarch64-linux-gnu -cpu max,sve128=on ./lEdax-arm-sve -n 1 -l 60 -solve ../problem/fforum-20-39.obf
else
make build OS=linux ARCH=arm COMP=gcc CC=aarch64-linux-gnu-gcc
cd ../bin
qemu-aarch64 -L /usr/aarch64-linux-gnu ./lEdax-arm -n 1 -l 60 -solve ../problem/fforum-20-39.obf
fi
=======
=======
if [ "$1" = arm32 ]
then
make build OS=linux ARCH=arm-neon COMP=gcc CC=arm-linux-gnueabi-gcc
cd ../bin
qemu-arm -L /usr/arm-linux-gnueabi ./lEdax-arm-neon -n 1 -l 60 -solve ../problem/fforum-20-39.obf
else
<<<<<<< HEAD
>>>>>>> 23e04d1 (Backport endgame_sse optimizations into endgame.c)
make build OS=linux ARCH=ARM COMP=gcc CC=aarch64-linux-gnu-gcc
cd ../bin
qemu-aarch64 -L /usr/aarch64-linux-gnu ./lEdax-ARM -n 1 -l 60 -solve ../problem/fforum-20-39.obf
<<<<<<< HEAD
# qemu-arm -L /usr/arm-linux-gnueabi ./lEdax-ARMv7 -n 1 -l 60 -solve ../problem/fforum-20-39.obf
>>>>>>> 81dec96 (Kindergarten last flip for arm32; MSVC arm Windows build (not tested))
=======
=======
make build OS=linux ARCH=arm COMP=gcc CC=aarch64-linux-gnu-gcc
cd ../bin
qemu-aarch64 -L /usr/aarch64-linux-gnu ./lEdax-arm -n 1 -l 60 -solve ../problem/fforum-20-39.obf
>>>>>>> 520040b (Use DISPATCH_NEON, not hasNeon, for android arm32 build)
fi
>>>>>>> 23e04d1 (Backport endgame_sse optimizations into endgame.c)
cd ../src
# C:\Android\android-ndk-r21\toolchains\llvm\prebuilt\windows-x86_64\bin\clang.exe --target=aarch64-linux-gnu -O2 -S flip_neon_bitscan.c
