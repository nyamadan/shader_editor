#pragma once

#include <vector>
#include <string>
#include <stb_image.h>
#include "file_utils.hpp"

class Image {
   private:
    std::string base;
    std::string name;
    std::string path;
    std::string ext;

    int32_t width = 0;
    int32_t height = 0;
    int32_t channels = 0;
    uint8_t *data = nullptr;

   public:
    Image() {}
    ~Image() {
        if (data != nullptr) {
            stbi_image_free(data);
            data = nullptr;
        }
    }
    const std::string &getName() const { return name; }

    void setPath(const std::string &base, const std::string &name) {
        std::string ext = getExtention(base);
        this->setPath(base, name, ext);
    }

    void setPath(const std::string &base, const std::string &name,
                 const std::string &ext) {
        this->base = base;
        this->name = name;
        this->ext = ext;

        this->path = this->base;
        appendPath(this->path, this->name);
    }

    void load() {
        this->data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    }
};
