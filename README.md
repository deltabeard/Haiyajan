# Haiyajan
Haiyajan (هیجان) is a tiny and fast entertainment interface that is designed to
be simple to use. It uses plugins named "Libretro cores" to provide various
emulation and multimedia features. It is written in C99 language and depends
only on the SDL2 library and the Libretro cores you wish to use.

Haiyajan is a new project and is therefore not ready for regular use. The first
stable version will be released as version "1.0".

## Libretro
Haiyajan uses the Libretro API to provide various capabilities, such as game
emulation and multimedia. [RetroArch](https://github.com/libretro/RetroArch) is
the reference implementation of the Libretro API. The Libretro cores and their
respective content must be downloaded separately. You may find many Libretro
projects at the [official Libretro repository list](https://github.com/libretro/).

## Support
The only platforms that this project will be regularly tested on for
major releases are:

- Arch Linux (x86_64, glibc)
- Alpine Linux (aarch64, Raspberry Pi 2B/Qemu, musl libc)
- Windows 10 (x86_64, AMD Radeon 500/Kaby Lake R, MSVC 2015)

Haiyajan *should* work on any platform supported by SDL2 and has a C99
compiler. Haiyajan may therefore run on Linux, Unix, Windows
XP/Vista/7/8/10/RT, Mac OS X 10.5+/macOS, Haiku, iOS 5.1.1+, Android 2.3.3+,
Emscripten, Nintendo Switch (libnx).

Note:
- Haiyajan executables built for Windows will require the
[Visual C++ Redistributable for Visual Studio 2015](https://www.microsoft.com/en-us/download/details.aspx?id=48145).
- Since the official SDL2 API for the Nintendo Switch is released under an NDA,
it is not compatible with the AGPL license of Haiyajan. Consider using libnx.
- Libretro cores may have different system requirements.

We welcome bug reports and contributions for all platforms.

## License
Copyright (C) 2020 Mahyar Koshkouei, et al.

Haiyajan is free software released under the **GNU Affero General Public
License Version 3 (AGPL v3)**; see the LICENSE file for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.

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

## Building
The following dependencies are required for building Haiyajan. Other tools
may be used, but are unsupported by this project.
- SDL2
- C99 Compiler (GCC, Clang, MSVC 2015 cl)
- GNU Make

Simply execute GNU make in the Haiyajan project folder to build with
automatically detected options based on available libraries.

Execute `make help` in order to see various build options. Some options may be
automatically selected or unsupported for your build platform.

## Aims
- Simple to use.
- Small and fast.

## Objectives
In no particular order.

- [ ] Maximum depth of three in menu.
- [x] Support for software and OpenGL cores.
- [ ] Automatic selection of libretro core given the input file.
- [ ] Compensation for ASCII-only font by reading input file header.
- [ ] Download libretro cores, program updates as required, with user control.
- [x] Automatic assignment of gamepad mappings.
- [ ] Automatic detection of A/B placement for emulated system.
- [ ]Small number of GUI configuration parameters.
  - [ ] Colours, placement of menu and hotkeys.
- [ ] Automatic DPI scaling.
  - [ ] Font and UI size.
