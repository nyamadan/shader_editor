#include "common.hpp"
#include "file_utils.hpp"

#include <fstream>
#include <streambuf>
#include <vector>

#ifndef WIN32
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

std::vector<std::string> openDir(const std::string &path) {
#ifdef WIN32
    std::vector<std::string> files;

    HANDLE hFind;
    WIN32_FIND_DATA win32fd;

    const std::string fileName = path + "*.*";
    hFind = FindFirstFileA(fileName.c_str(), &win32fd);

    if (hFind == INVALID_HANDLE_VALUE) {
        return std::move(files);
    }

    do {
        if (win32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        } else {
            files.push_back(win32fd.cFileName);
        }

    } while (FindNextFileA(hFind, &win32fd));

    return std::move(files);
#else
    std::vector<std::string> files;

    const char *const dir = path.c_str();
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    if ((dp = opendir(dir)) == NULL) {
        return std::move(files);
    }

    while ((entry = readdir(dp)) != NULL) {
        stat(entry->d_name, &statbuf);
        if (S_ISDIR(statbuf.st_mode)) {
        } else {
            files.push_back(entry->d_name);
        }
    }
    closedir(dp);

    return std::move(files);
#endif
}

bool readText(const std::string &path, std::string &memblock) {
    std::ifstream fs(path, std::ifstream::in);
    memblock.assign((std::istreambuf_iterator<char>(fs)),
                    std::istreambuf_iterator<char>());
    return true;
}

void writeText(const std::string &path, const char *const memblock,
               uint32_t size) {
    FILE *fp;
    int32_t fd;

    fd = open(path.c_str(), O_WRONLY | O_BINARY);
    fp = fdopen(fd, "wb");
    fwrite(memblock, sizeof(char), size, fp);
    fclose(fp);
}

int64_t getMTime(const std::string &path) {
    int32_t fd = open(path.c_str(), O_RDONLY | O_BINARY);

    if (fd < 0) {
        return -1;
    }

    struct stat st;
    fstat(fd, &st);

    return static_cast<int64_t>(st.st_mtime);
}
