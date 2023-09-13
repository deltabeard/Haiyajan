/**
 * Header file that should be included in all Haiyajan source files.
 */

#pragma once

#ifndef HAIYAJAN_ALL_HEADER
#define HAIYAJAN_ALL_HEADER

#ifdef __GNUC__

/** Bad string functions. **/
/* Use memccpy() instead of any of these. */
#pragma GCC poison strcpy strncpy strlcpy SDL_strlcpy

#endif

#endif
