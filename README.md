<img align="right" width="128" height="128" alt="The Haiyajan Icon: Haiya Dragon" src="meta/haiya_dragon_bg_rounded.svg">

# Haiyajan

Haiyajan (هیجان) is an entertainment interface that is designed to
be small, fast, and simple to use. It uses plugins named "Libretro cores" to
provide various emulation and multimedia features. It is written in C99
language and depends only on the SDL2 library and the Libretro cores you wish
to use.

Haiyajan is a new project and is therefore not ready for regular use. The first
stable version will be released as version "1.0".

Join the community on Matrix at [#haiyajan:deltabeard.com](https://matrix.to/#/#haiyajan:deltabeard.com)
or on Reddit at [/r/Haiyajan/](https://www.reddit.com/r/Haiyajan/).

## Libretro

Haiyajan uses the Libretro API to provide various capabilities, such as game
emulation and multimedia. [RetroArch](https://github.com/libretro/RetroArch) is
the reference implementation of the Libretro API. The Libretro cores and their
respective content must be downloaded separately. You may find many Libretro
projects at the [official Libretro repository list](https://github.com/libretro/).

## Download

The latest release is version **0.1**.

See the [Releases page](https://github.com/deltabeard/Haiyajan/releases/) for downloads.

## License

Copyright (C) 2020 Mahyar Koshkouei, et al.

Haiyajan is free software released under the **GNU Affero General Public
License Version 3 (AGPL v3)**; see the LICENSE file for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.

The Haiyajan icon was created by Cher at https://ko-fi.com/staticevent.

# Development

## Versioning

This project will use a MAJOR.MINOR revision system. The first release build
will be version 1.0. Until then, all releases will be considered unstable.
The MAJOR version will then be incremented only when a significant change is
made to the program. This may include significant changes to the user interface,
use of a new version of Libretro API, removal of support for a platform, etc.
All other changes will only increment the MINOR version.

Compared to Semantic Versioning 2.0, the PATCH version is removed in this
project for brevity.

## Support

The platforms that this project will be regularly tested on for
major releases are currently:

| OS           | Kernel                    | Architecture         | Test Platform   | C Library          | Additionally Tests Support For:                |
|--------------|---------------------------|----------------------|-----------------|--------------------|------------------------------------------------|
| Alpine Linux | Linux Latest Stable       | x86-64               | Various         | musl libc          | Generic Modern Linux System with OpenGL 3.3+   |
| Alpine Linux | Linux Latest Stable       | ARM1176JZF-S         | Raspberry Pi 1A | musl libc          | Any Low Power Linux System with OpenGLES 2     |
| Arch Linux   | Linux Latest Stable       | ARM Cortex-A72       | Raspberry Pi 4B | GNU libc           | Any High Power Linux System with OpenGLES 3    |
| ReactOS      | Windows NT 5.1 Compatible | x86-32               | VM              | MSVC 2019, v141_xp | Wine, Windows XP/7/8.1/10                      |
| Windows 10   | Windows NT 10.0           | x86-64               | Various         | MSVC 2019, v142    | Generic Modern Windows System with OpenGL 3.3+ |
| Horizon OS   | Custom BSD                | ARM Cortex-A57 & A53 | Yuzu            | GNU libc           | Nintendo Switch via libNX with OpenGL 3.3+     |


Haiyajan *should* work on any platform supported by SDL2 and has a C99
compiler. Haiyajan may therefore run on Linux, Unix, Windows
XP/2003/Vista/7/8/10/RT, Mac OS X 10.5+/macOS, Haiku, iOS 5.1.1+, Android 2.3.3+,
Emscripten, Nintendo Switch (libnx).

We welcome bug reports and contributions for all platforms.

Note:
- Haiyajan executables built for Windows will be compiled with MSVC 2019, and
may require the
[Visual C++ 2015 Redistributable](https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads).
- Since the official SDL2 API for the Nintendo Switch is released under an NDA,
it is not compatible with the AGPL license of Haiyajan. Consider using libnx.
- Libretro cores may have different system requirements.

## Building

The following dependencies are required for building Haiyajan. Other tools
may be used, but are unsupported by this project.
- SDL2 2.0.12
- C99 Compiler (GCC, Clang, MSVC 2019)
- GNU Make 4.3

When compiling for Windows NT, follow the instructions under the Windows NT
header below. Otherwise, simply execute GNU make in the Haiyajan project folder
to build with automatically detected options based on available libraries.

Execute `make help` in order to see various build options. Some options may be
automatically selected or unsupported for your build platform.

### Windows NT

When compiling for Windows NT platforms (such as ReactOS, Windows XP, Windows 10, etc.)
please use the "Native Tools Command Prompt for VS 2019". Steps for compiling
Haiyajan for Windows NT are basically:

1. On a Windows 10 platform with Visual Studio 2019 installed, download and
   compile [the latest GNU make](http://ftpmirror.gnu.org/make/) by executing
   "build_w32.bat" within the *Native Tools Command Prompt for VS 2019*. Add
   the compiled gnumake.exe binary to PATH.
2. Download SDL2 and compile it with cmake. Specify the toolset v141_xp when
   targeting x86 (32-bit) version of Windows.
3. Within *Native Tools Command Prompt for VS 2019*, execute GNU make with the
   environment variables SDL_LIB and SDL_INC set to the location of the
   compiled SDL2 libraries and the location of the SDL2 headers respectively.

Execute `make help` for help with options.

## Objectives

In no particular order.

- [ ] Maximum depth of three in menu.
- [x] Support for software and OpenGL cores.
- [ ] Automatic selection of libretro core given the input file.
- [ ] Compensation for ASCII-only font by reading input file header.
- [ ] Download libretro cores, program updates as required, with user control.
- [x] Automatic assignment of gamepad mappings.
- [ ] Automatic detection of A/B placement for emulated system.
- [ ] Small number of GUI configuration parameters.
  - [ ] Colours, placement of menu and hotkeys.
- [ ] Automatic DPI scaling.
  - [ ] Font and UI size.
