#pragma once

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define GLFW_INCLUDE_ES3
#include <GLFW/glfw3.h>
#else
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#endif
