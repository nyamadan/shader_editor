#include "common.hpp"

bool readText(const std::string &path, char *&memblock);
bool readText(const std::string &path, char *&memblock, time_t &mTime);
void writeText(const std::string &path, const char * const memblock, size_t size);
time_t getMTime(const std::string &path);
