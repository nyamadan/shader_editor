#include "common.hpp"

bool readText(char *&memblock, const char *const path) {
    FILE *fp;
    int fd;
    struct stat st;
    size_t size;

    fd = open(path, O_RDONLY | O_BINARY);

    fp = fdopen(fd, "rb");
    fstat(fd, &st);
    size = st.st_size;

    memblock = new char[size + 1];
    fread(memblock, sizeof(char), size, fp);
    memblock[size] = '\0';
    fclose(fp);

    return true;
}
