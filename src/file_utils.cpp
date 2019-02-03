#include "common.hpp"
#include "file_utils.hpp"

#include <fstream>
#include <streambuf>

bool readText(const std::string &path, std::string &memblock) {
    std::ifstream fs(path, std::ifstream::in);
    memblock.assign((std::istreambuf_iterator<char>(fs)),
                    std::istreambuf_iterator<char>());
    return true;
}

void writeText(const std::string &path, const char *const memblock,
               size_t size) {
    FILE *fp;
    int fd;

    fd = open(path.c_str(), O_WRONLY | O_BINARY);
    fp = fdopen(fd, "wb");
    fwrite(memblock, sizeof(char), size, fp);
    fclose(fp);
}

time_t getMTime(const std::string &path) {
    auto fd = open(path.c_str(), O_RDONLY | O_BINARY);

    struct stat st;
    fstat(fd, &st);

    return static_cast<time_t>(st.st_mtime);
}
