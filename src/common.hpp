#pragma once

#include <stdio.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define GLFW_INCLUDE_ES31
#include <GLFW/glfw3.h>
#else
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#endif
