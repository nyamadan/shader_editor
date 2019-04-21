#include "common.hpp"
#include "file_utils.hpp"

#include <fstream>
#include <streambuf>
#include <algorithm>

#ifndef _MSC_VER
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
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

#ifdef _MSC_VER
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
#else
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

#endif

    return std::move(files);
}

std::string getExtention(const std::string &path) {
    int64_t iExt = path.find_last_of(".");

    if (iExt <= 0) {
        return std::move(std::string());
    }

    std::string ext = path.substr(iExt, path.size() - iExt);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return std::move(ext);
}

void appendPath(std::string &base, const std::string &path) {
#if defined(_MSC_VER) || defined(__MINGW32__)
    if (base.back() != '\\' && base.back() != '/') {
        base.append("\\");
    }
#else
    if (base.back() != '/') {
        base.append("/");
    }
#endif
    base.append(path);
}

bool readText(const std::string &path, std::string &memblock) {
    std::ifstream fs(path, std::ifstream::in);
    memblock.assign((std::istreambuf_iterator<char>(fs)),
                    std::istreambuf_iterator<char>());
    return true;
}

void writeText(const std::string &path, const char *const memblock,
               uint32_t size) {
    FILE *fp = fopen(path.c_str(), "wb");
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

	close(fd);

    return static_cast<int64_t>(st.st_mtime);
}
