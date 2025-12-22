echo off
clang -std=c17 -pedantic -W -Wall -Wno-invalid-source-encoding -O3 -flto -ffast-math -fomit-frame-pointer -fuse-ld=lld^
 -DNDEBUG -D_GNU_SOURCE=1 -Wno-builtin-macro-redefined -U__STDC_NO_THREADS__ -D_CRT_SECURE_NO_DEPRECATE^
 -march=%1 -fprofile-instr-generate  all.c -o ..\bin/wEdax-%1.exe -lWS2_32
..\bin\wEdax-%1 -bench 20 -n 1 -eval-file ..\bin\data\eval.dat
..\bin\wEdax-%1 -bench 20 -n 8 -eval-file ..\bin\data\eval.dat
llvm-profdata merge -output=edax.profdata *.profraw
clang -std=c17 -pedantic -W -Wall -Wno-invalid-source-encoding -O3 -flto -ffast-math -fomit-frame-pointer -fuse-ld=lld^
 -DNDEBUG -D_GNU_SOURCE=1 -Wno-builtin-macro-redefined -U__STDC_NO_THREADS__ -D_CRT_SECURE_NO_DEPRECATE^
 -march=%1 -fprofile-instr-use=edax.profdata  all.c -o ..\bin/wEdax-%1.exe -lWS2_32
del *.prof*
