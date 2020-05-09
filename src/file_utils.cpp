#include "common.hpp"
#include "file_utils.hpp"
#include "app_log.hpp"

#include <fstream>
#include <streambuf>
#include <algorithm>
#include <memory>

#if defined(_MSC_VER) | defined(__MINGW32__)
#include <io.h>
#define open _open
#define fdopen _fdopen
#else
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0x0000
#endif

std::vector<std::string> openDir(std::string path) {
    std::vector<std::string> files;

#if defined(_MSC_VER) || defined(__MINGW32__)
    if (path.back() != '\\' && path.back() != '/') {
        path.append("\\");
    }
#else
    if (path.back() != '/') {
        path.append("/");
    }
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
    HANDLE hFind;
    WIN32_FIND_DATAA win32fd;

    const std::string fileName = path + "*.*";
    hFind = FindFirstFileA(fileName.c_str(), &win32fd);

    if (hFind == INVALID_HANDLE_VALUE) {
        AppLog::getInstance().error("failed to openDir.\n");
        return std::move(files);
    }

    do {
        if ((win32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            AppLog::getInstance().debug("file: %s\n", win32fd.cFileName);
            files.push_back(win32fd.cFileName);
        }

    } while (FindNextFileA(hFind, &win32fd));
#else
    const char *const dir = path.c_str();
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    if ((dp = opendir(dir)) == NULL) {
        AppLog::getInstance().error("failed to opendir.\n");
        return files;
    }

    while ((entry = readdir(dp)) != NULL) {
        stat(entry->d_name, &statbuf);
#ifndef __EMSCRIPTEN__
        if (S_ISREG(statbuf.st_mode)) {
#endif
            AppLog::getInstance().debug("readdir: %s\n", entry->d_name);
            files.push_back(entry->d_name);
#ifndef __EMSCRIPTEN__
        }
#endif
    }
    closedir(dp);
#endif
    return files;
}

bool readText(const std::string &path, std::string &memblock) {
    int32_t fd = open(path.c_str(), O_RDONLY | O_BINARY);

    if (fd < 0) {
        return false;
    }

    struct stat st;
    fstat(fd, &st);

    FILE *fp = fdopen(fd, "rb");
    auto buf = std::make_unique<char []>(st.st_size);

    fread(buf.get(), 1, st.st_size, fp);
    memblock.assign(buf.get(), st.st_size);
    fclose(fp);

    return true;
}

void writeText(const std::string &path, const char *const memblock,
               uint32_t size) {
    FILE *fp = fopen(path.c_str(), "wb");
    fwrite(memblock, sizeof(char), size, fp);
    fclose(fp);
}

int64_t getMTime(const std::string &path) {
#ifdef __EMSCRIPTEN__
    return -1;
#else
    int32_t fd = open(path.c_str(), O_RDONLY | O_BINARY);

    if (fd < 0) {
        return -1;
    }

    struct stat st;
    fstat(fd, &st);

    close(fd);

    return static_cast<int64_t>(st.st_mtime);
#endif
}
