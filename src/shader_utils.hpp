#pragma once

#include <string>

extern const char* const GlslVersion;

int checkLinked(unsigned int program, std::string *error = nullptr);
int checkCompiled(unsigned int shader, std::string *error = nullptr);
