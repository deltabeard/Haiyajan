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
Haiyajan *should* work on any platform supported by SDL2. The only
platforms that this project will be regularly tested on for major releases are:

- Linux (x86_64, ArchLinux, glibc)
- Linux (x86_64 Qemu, Alpine Linux, musl libc)
- Linux (ARMv6 Raspberry Pi 1 B+, Alpine Linux, musl libc)
- Linux (ARMv7 Raspberry Pi 3, Alpine Linux, musl libc)
- Linux (ARMv8 Raspberry Pi 4B, Alpine Linux, musl libc)
- Windows (x86_64, MinGW, glibc)

We welcome bug reports and contributions for all platforms.

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
The following dependencies are recommended for building Haiyajan. Other tools
may be used, but are unsupported by this project.
- SDL2 (Required)
- GNU Compiler Collection (GCC)
- GNU Make

Simply execute `make` in the project folder.

Execute `make help` in order to see various build options. Some options may be
automatically selected or unsupported for your build platform.

## Aims
- Simple to use.
- Small and fast.

## Objectives
In no particular order.

- Maximum depth of three in menu.
- Support for software and OpenGL cores.
- Automatic selection of libretro core given the input file.
- Compensation for ASCII-only font by reading input file header.
- Download libretro cores, program updates as required, with user control.
- Automatic assignment of gamepad mappings.
- Automatic detection of A/B placement for emulated system.
- Small number of GUI configuration parameters.
  - Colours, placement of menu and hotkeys.
- Automatic DPI scaling.
  - Font and UI size.
