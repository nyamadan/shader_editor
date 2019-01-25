#include "common.hpp"

bool readText(const char *const path, char *&memblock, time_t *mTime = nullptr);
time_t getMTime(const char *const path);
