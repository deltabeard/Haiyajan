# vim: ts=4:sw=4:expandtab
CMAKE_MINIMUM_REQUIRED(VERSION 3.18...3.23)

## Check user set options.
IF(NOT CMAKE_BUILD_TYPE)
    MESSAGE(STATUS "CMAKE_BUILD_TYPE was not set by user; setting build type to Debug")
    SET(CMAKE_BUILD_TYPE "Debug")
ELSE()
    # List of valid build types
    SET(VALID_BUILD_TYPES Debug Release RelWithDebInfo MinSizeRel)
    LIST(FIND VALID_BUILD_TYPES ${CMAKE_BUILD_TYPE} IS_VALID_BUILD_TYPE)
    IF(IS_VALID_BUILD_TYPE EQUAL -1)
        MESSAGE(FATAL_ERROR "CMAKE_BUILD_TYPE was '${CMAKE_BUILD_TYPE}' but can only be set to one of ${VALID_BUILD_TYPES}")
    ENDIF()
ENDIF()

# Obtain the version of Haiyajan that is being built.
FIND_PACKAGE(Git)
IF(GIT_FOUND)
    EXECUTE_PROCESS(
            COMMAND ${GIT_EXECUTABLE} describe --tags --long --always --dirty=-dirty
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE PROC_RESULT
            OUTPUT_VARIABLE FULL_VERSION
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    EXECUTE_PROCESS(
            COMMAND ${GIT_EXECUTABLE} describe --abbrev=0 --tags
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE PROC_RESULT
            OUTPUT_VARIABLE LAST_TAG
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE)
ENDIF()
IF(NOT GIT_FOUND OR NOT PROC_RESULT EQUAL 0)
    # If git is not available on the build platform, use this as a fallback.
    SET(LAST_TAG 0.1.0)
    SET(FULL_VERSION ${LAST_TAG}-LOCAL)
ENDIF()
MESSAGE(STATUS "Configuring version ${FULL_VERSION}")

# Initialise project information.
PROJECT(haiyajan
        LANGUAGES C
        VERSION ${LAST_TAG}
        DESCRIPTION "Haiyajan"
        HOMEPAGE_URL "https://github.com/deltabeard/haiyajan")
SET(HAIYAJAN_LICENSE "AGPL-3.0, Copyright (c) 2020-2023 Mahyar Koshkouei")

# Add dependencies to project.
IF(NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug" AND APPLE)
    SET(EXE_TARGET_TYPE MACOSX_BUNDLE)
    MESSAGE(VERBOSE "Setting EXE type to MACOSX Bundle")
ELSEIF(NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug" AND MSVC)
    SET(EXE_TARGET_TYPE WIN32)
    MESSAGE(VERBOSE "Setting EXE type to WIN32")
ENDIF()
ADD_EXECUTABLE(${PROJECT_NAME} ${EXE_TARGET_TYPE})
TARGET_SOURCES(${PROJECT_NAME} PRIVATE src/font.c src/gl.c src/haiyajan.c
    src/input.c src/load.c src/menu.c src/play.c src/rec.c src/sig.c src/tai.c
    src/timer.c src/tinflate.c src/ui.c src/util.c)
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE inc)

# Set compile options based upon build type.
SET_PROPERTY(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    SET(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
    SET(CMAKE_C_FLAGS_MINSIZEREL "-flto -Os -g0")
    SET(CMAKE_C_FLAGS_RELWITHDEBINFO "-flto -O3 -g2")
    SET(CMAKE_C_FLAGS_RELEASE "-flto -O3 -g0")
ELSEIF((CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
        CMAKE_C_COMPILER_ID STREQUAL "Clang" OR
        CMAKE_C_COMPILER_ID STREQUAL "AppleClang"))
    # Enable warnings and use C99
    # "C_STANDARD 99" sets gnu99 which is unacceptable.
    TARGET_COMPILE_OPTIONS(${PROJECT_NAME} PRIVATE -Wall -Wextra -std=c99 -pedantic)
    SET(CMAKE_C_FLAGS_DEBUG "-Og -g3")
    SET(CMAKE_C_FLAGS_MINSIZEREL "-Os -g1")
    SET(CMAKE_C_FLAGS_RELWITHDEBINFO "-Ofast -g3")
    SET(CMAKE_C_FLAGS_RELEASE "-Ofast -g1")
ELSEIF(MSVC)
    SET(CMAKE_C_FLAGS_DEBUG "/Od /Zi /diagnostics:caret")
    # /Od Turn off all optimisations.
    # /Zi Create debugging information.

    SET(CMAKE_C_FLAGS_MINSIZEREL "/O1 /Os /Gw /GL /fp:fast /Ob1 /GT /MD /GS-")
    # /O1 Use maximum optimisation for size.
    # /Os Favour size over speed.

    SET(CMAKE_C_FLAGS_RELEASE "/O2 /Gw /GL /fp:fast /Ob2 /Oi /Ot /GT /MD /GS-")
    # /O2 Use maximum optimisation for speed.
    # /Gw Optimise global data.
    # /GL Whole program optimisation.
    # /fp:fast Optimise floating-point code for speed and space.
    # /Ob2 Inline function expansion.
    # /Oi Enable intrinsic functions.
    # /Ot Favour fast code.
    # /GT Optimise fibers.
    # /MD Use multi-threaded DLL runtime.
    # /GS- Disable security checking.
    # /arch:AVX2 Enable AVX2 instructions.

    IF(NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        TARGET_LINK_OPTIONS(${PROJECT_NAME} PRIVATE /LTCG /OPT:ICF)
    ENDIF()
ENDIF()

IF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    ADD_COMPILE_DEFINITIONS(SDL_ASSERT_LEVEL=3)
ELSE()
    ADD_COMPILE_DEFINITIONS(NDEBUG)
ENDIF()

# Discover libraries
SET(LIBRARY_DISCOVER_METHOD "CPM" CACHE STRING
        "Tool to use for discovering dependencies; options are: VCPKG, PKG_CONFIG, CPM")

IF(${CMAKE_SYSTEM_NAME} MATCHES "Emscripten")
    set(USE_FLAGS "-s USE_SDL=2")
    set(EMCC_FLAGS "-s ALLOW_MEMORY_GROWTH=1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS} ${EMCC_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS} ${EMCC_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${USE_FLAGS} \
        --shell-file ${PROJECT_SOURCE_DIR}/meta/emscripten_shell.html")
    set(CMAKE_EXECUTABLE_SUFFIX .html)
ELSEIF(${LIBRARY_DISCOVER_METHOD} STREQUAL "VCPKG")
    # If a VCPKG toolchain is specified, use it.
    FIND_PACKAGE(SDL2 CONFIG REQUIRED)
ELSEIF(${LIBRARY_DISCOVER_METHOD} STREQUAL "PKG_CONFIG")
    INCLUDE(FindPkgConfig)
    PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2)
ELSEIF(${LIBRARY_DISCOVER_METHOD} STREQUAL "CPM")
    SET(CPM_USE_NAMED_CACHE_DIRECTORIES)
    INCLUDE(meta/CPM.cmake)
    CPMADDPACKAGE(GITHUB_REPOSITORY libsdl-org/SDL
            NAME SDL2
            GIT_TAG release-2.26.3
            OPTIONS
            "SDL_SHARED_ENABLED_BY_DEFAULT OFF"
            "SDL_STATIC_ENABLED_BY_DEFAULT ON"
            "OPT_DEF_LIBC TRUE"
            "SDL_DISKAUDIO OFF"
            "CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}"
            "CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG}"
            "CMAKE_C_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_MINSIZEREL}"
            "CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO}"
            "CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE}")

    TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC ${SDL2_SOURCE_DIR}/include)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE SDL2::SDL2-static SDL2::SDL2main)
ELSE()
    MESSAGE(SEND_ERROR "LIBRARY_DISCOVER_METHOD '${LIBRARY_DISCOVER_METHOD}' is not valid")
ENDIF()

# Add required dependencies
TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARIES})

# Some FindSDL2 modules use slightly different variables, so we just use both.
TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC ${SDL2_INCLUDE_DIRS} ${SDL2_INCLUDE_DIR})

# Add definitions of project information.
ADD_COMPILE_DEFINITIONS(COMPANY=Deltabeard)
ADD_COMPILE_DEFINITIONS(DESCRIPTION=${PROJECT_DESCRIPTION})
ADD_COMPILE_DEFINITIONS(LICENSE=${HAIYAJAN_LICENSE})
ADD_COMPILE_DEFINITIONS(NAME=${PROJECT_NAME})
ADD_COMPILE_DEFINITIONS(FULL_VERSION=${FULL_VERSION})
ADD_COMPILE_DEFINITIONS(VER_MAJOR=${haiyajan_VERSION_MAJOR})
ADD_COMPILE_DEFINITIONS(VER_MINOR=${haiyajan_VERSION_MINOR})
ADD_COMPILE_DEFINITIONS(VER_PATCH=${haiyajan_VERSION_PATCH})
ADD_COMPILE_DEFINITIONS(VER_TWEAK=0)

# Collect source files and compress them
SET(SRC_ARCHIVE ${CMAKE_CURRENT_BINARY_DIR}/src.tar.xz)
FILE(GLOB PROJ_FILES LIST_DIRECTORIES false
        inc/*.h src/*.c LICENSE README.md CMakeLists.txt)
FILE(ARCHIVE_CREATE OUTPUT ${SRC_ARCHIVE}
        PATHS ${PROJ_FILES}
        FORMAT gnutar COMPRESSION XZ COMPRESSION_LEVEL 6)
IF(!MSVC)
    FILE(READ ${SRC_ARCHIVE} filedata HEX)
    STRING(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," filedata ${filedata})
    FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/src.c
            "/* This file is required for AGPLv3 compliance. */\n\n"
            "static const volatile unsigned char src[] = {${filedata}};\n"
            "static const unsigned long src_sz = sizeof(src);\n")
    TARGET_SOURCES(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src.c)
ENDIF()

# Platform specific options
IF(MSVC)
    TARGET_SOURCES(${PROJECT_NAME} PRIVATE meta/winres.rc)
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE
            Winmm.lib Version.lib Setupapi.lib Imm32.lib)
    SET_PROPERTY(TARGET ${PROJECT_NAME} PROPERTY VS_DPI_AWARE "PerMonitor")
    TARGET_COMPILE_OPTIONS(${PROJECT_NAME} PRIVATE /W4)
ELSEIF(NINTENDO_SWITCH)
    # Fix linking with C++ libraries
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} PRIVATE stdc++)

    NX_GENERATE_NACP(${PROJECT_NAME}.nacp
            NAME "${PROJECT_NAME}"
            AUTHOR "Deltabeard"
            VERSION "${FULL_VERSION}")

    NX_CREATE_NRO(${PROJECT_NAME}
            NACP ${PROJECT_NAME}.nacp
            ICON "${PROJECT_SOURCE_DIR}/meta/haiya_dragon_256.png")
ENDIF()

# Package options
IF(APPLE)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
            MACOSX_BUNDLE_BUNDLE_VERSION ${LAST_TAG}
            MACOSX_BUNDLE_COPYRIGHT ${HAIYAJAN_LICENSE}
            MACOSX_BUNDLE_INFO_STRING ${PROJECT_DESCRIPTION}
            MACOSX_BUNDLE_SHORT_VERSION_STRING ${FULL_VERSION}
            MACOSX_BUNDLE_LONG_VERSION_STRING ${FULL_VERSION})
    INSTALL(TARGETS haiyajan BUNDLE DESTINATION
            ${CMAKE_INSTALL_BINDIR}/${PROJECT_NAME})
ELSE()
    INSTALL(TARGETS haiyajan RUNTIME)
ENDIF()

SET(CPACK_PACKAGE_VENDOR Deltabeard)
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
#SET(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}/meta/icon_hi.ico)
SET(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE)
INCLUDE(CPack)

# The following macro was taken from the SDL2 project.
MACRO(MESSAGE_BOOL_OPTION _NAME _VALUE)
    IF(${_VALUE})
        MESSAGE(STATUS "  ${_NAME}: enabled")
    ELSE()
        MESSAGE(STATUS "  ${_NAME}: disabled")
    ENDIF()
ENDMACRO()

MESSAGE(STATUS "Haiyajan will build with the following options:")
MESSAGE(STATUS "  CC:      ${CMAKE_C_COMPILER} '${CMAKE_C_COMPILER_ID}'")
MESSAGE(STATUS "  CFLAGS:  ${CMAKE_C_FLAGS}")
MESSAGE(STATUS "  LDFLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
