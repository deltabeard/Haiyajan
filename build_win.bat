@echo off
rem This file is for use in Windows 10 command prompt only.

set sdlincdir=sdlinc
set sdllibdir=sdllib

if NOT DEFINED VSCMD_VER (
	echo This must be executed within Native Tools Command Prompt.
	exit /B
)

rem Check that GNU make is available.
where gnumake
if errorlevel 1 (
	echo gnumake was not found. Please install GNU make as gnumake.exe in PATH.
	exit /B
)

rem Extract SDL2 headers and pre-compiled libraries
mkdir %sdlincdir%
mkdir %sdllibdir%
expand tools\sdl2_2-0-12_inc.cab -F:* %sdlincdir%
expand tools\sdl2_2-0-12_%VSCMD_ARG_TGT_ARCH%.cab -F:* %sdllibdir%
if errorlevel 1 (
	echo Could not expand tools\sdl2_2-0-12_%VSCMD_ARG_TGT_ARCH%.cab 
	exit /B
)

set CC=cl
set CFLAGS=/nologo /GL /O2 /Ob2 /fp:fast /Ot /GF /GT /Oi /MT /Iinc /I'%sdlincdir%' /D_UNICODE /DUNICODE

if "%VSCMD_ARG_TGT_ARCH%"=="x64" (
	set CFLAGS=%CFLAGS% /Fdvc142.pdb
	set NT_REV=6.01
	set ICON_FILE=icon_hi.ico
) else (
	rem 32-bit Windows NT builds require SSE instructions, supported from Pentium III CPUs.
	rem These builds offer support for ReactOS and Windows XP.
	set CFLAGS=%CFLAGS% /arch:SSE /Fdvc141.pdb
	set NT_REV=5.01
	set ICON_FILE=icon_lo.ico
)

set LDFLAGS=/link /OUT:"Haiyajan-%VSCMD_ARG_TGT_ARCH%.exe" /MANIFEST /LTCG /NXCOMPAT /PDB:"Haiyajan-%VSCMD_ARG_TGT_ARCH%.pdb" /DYNAMICBASE "SDL2main.lib" "SDL2-static.lib" "libx264.lib" "libwavpack.lib" "winmm.lib" "msimg32.lib" "version.lib" "imm32.lib" "setupapi.lib" "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" /MACHINE:%VSCMD_ARG_TGT_ARCH% /INCREMENTAL:NO /SUBSYSTEM:WINDOWS",%NT_REV%" /LIBPATH:"%sdllibdir%"

@echo on
gnumake -B CC="%CC%" LDFLAGS="%LDFLAGS%" CFLAGS="%CFLAGS%" DEBUG=0 %1
