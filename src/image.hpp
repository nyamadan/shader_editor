#pragma once

#include <vector>
#include <string>
#include "file_utils.hpp"

class Image {
   private:
    std::string path;
    std::string ext;

   public:
    const std::string &getPath() const { return path; }

    void setPath(const std::string &path) {
        std::string ext = getExtention(path);
        this->setPath(path, ext);
    }

    void setPath(const std::string &path, const std::string &ext) {
        this->path = path;
        this->ext = ext;
    }
};
