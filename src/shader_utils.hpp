#pragma once

#include <string>

extern const char* const GlslVersion;

GLint checkLinked(GLuint program);
GLint checkLinked(GLuint program, std::string& error);

GLint checkCompiled(GLuint shader);
GLint checkCompiled(GLuint shader, std::string& error);
