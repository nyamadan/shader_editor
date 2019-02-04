#pragma once

#include <stdio.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
#include <io.h>
#define open _open
#define fdopen _fdopen
#endif

#ifndef O_BINARY
#define O_BINARY 0x0000
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define GLFW_INCLUDE_ES3
#include <GLFW/glfw3.h>
#else
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#endif
