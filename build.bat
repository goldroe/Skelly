@echo off

set mode=%1
if [%1]==[] (
  set mode=debug
)

IF NOT EXIST build MKDIR build
SET warning_flags=/WX /W4 /wd4100 /wd4101 /wd4189 /wd4505 
SET includes=/Iext\ /Iext\assimp\include\
SET compiler_flags=%warning_flags% %includes% /nologo /FC /Fdbuild\ /Fobuild\ /FeSkelly.exe /EHsc

if %mode%==release (
  SET compiler_flags=/O2 /D %compiler_flags%
) else if %mode%==debug (
  SET compiler_flags=/Zi /Od %compiler_flags%
) else (
  echo Unkown build mode
  exit /B 2
)

SET libs=assimp-vc143-mt.lib user32.lib kernel32.lib winmm.lib
SET linker_flags=/LIBPATH:ext\assimp\lib\x64\ %libs% /INCREMENTAL:NO /OPT:REF

CL src\skelly.cpp %compiler_flags% /link %linker_flags%
