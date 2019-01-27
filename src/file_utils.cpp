#include "common.hpp"

bool readText(const char *const path, char *&memblock, time_t *mTime) {
    FILE *fp;
    int fd;
    struct stat st;
    size_t size;

    fd = open(path, O_RDONLY | O_BINARY);

    fstat(fd, &st);
    size = st.st_size;

    if (mTime != nullptr) {
        *mTime = static_cast<time_t>(st.st_mtime);
    }

    fp = fdopen(fd, "rb");

    memblock = new char[size + 1];
    fread(memblock, sizeof(char), size, fp);
    memblock[size] = '\0';
    fclose(fp);

    return true;
}

time_t getMTime(const char *const path) {
    int fd;
    struct stat st;

    fd = open(path, O_RDONLY | O_BINARY);

    fstat(fd, &st);

    return static_cast<time_t>(st.st_mtime);
}
