#pragma once

#include <string>

extern const char* const GlslVersion;

int checkLinked(unsigned int program);
int checkLinked(unsigned int program, std::string& error);

int checkCompiled(unsigned int shader);
int checkCompiled(unsigned int shader, std::string& error);
