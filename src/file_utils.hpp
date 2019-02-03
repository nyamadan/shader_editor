#include "common.hpp"

#include <string>

bool readText(const std::string &path, std::string &memblock);
void writeText(const std::string &path, const char *const memblock,
               size_t size);
time_t getMTime(const std::string &path);
