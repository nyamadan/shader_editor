#include "common.hpp"

#include <string>
#include <vector>

bool readText(const std::string &path, std::string &memblock);
void writeText(const std::string &path, const char *const memblock,
               uint32_t size);
int64_t getMTime(const std::string &path);

std::vector<std::string> openDir(std::string path);
void appendPath(std::string &base, const std::string &path);
std::string getExtention(const std::string &path);
