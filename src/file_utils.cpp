#include "common.hpp"
#include <string>

bool readText(const std::string &path, char *&memblock) {
    FILE *fp;
    int fd;
    struct stat st;
    size_t size;

    fd = open(path.c_str(), O_RDONLY | O_BINARY);

    fstat(fd, &st);
    size = st.st_size;

    fp = fdopen(fd, "rb");

    memblock = new char[size + 1];
    fread(memblock, sizeof(char), size, fp);
    memblock[size] = '\0';
    fclose(fp);

    return true;
}

bool readText(const std::string &path, char *&memblock, time_t &mTime) {
    FILE *fp;
    int fd;
    struct stat st;
    size_t size;

    fd = open(path.c_str(), O_RDONLY | O_BINARY);

    fstat(fd, &st);
    size = st.st_size;

    mTime = static_cast<time_t>(st.st_mtime);

    fp = fdopen(fd, "rb");

    memblock = new char[size + 1];
    fread(memblock, sizeof(char), size, fp);
    memblock[size] = '\0';
    fclose(fp);

    return true;
}

void writeText(const std::string &path, const char * const memblock, size_t size) {
    FILE *fp;
    int fd;

    fd = open(path.c_str(), O_WRONLY | O_BINARY);
    fp = fdopen(fd, "wb");
    fwrite(memblock, sizeof(char), size, fp);
    fclose(fp);
}

time_t getMTime(const std::string &path) {
    int fd;
    struct stat st;

    fd = open(path.c_str(), O_RDONLY | O_BINARY);

    fstat(fd, &st);

    return static_cast<time_t>(st.st_mtime);
}
