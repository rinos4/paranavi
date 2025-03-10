@echo off
set GCC_EXEC_PREFIX=c:/tmp/temp/gba/devkitadv/lib/gcc-lib/
set PATH=c:\tmp\temp\gba\devkitadv\bin;%PATH%

make %1 %2 %3
