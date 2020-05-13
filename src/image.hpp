#pragma once

#include <string>
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
    uint32_t textureId = 0;
    uint8_t *data = nullptr;

   public:
    Image();
    ~Image();
    const std::string &getPath() const;

    void setPath(const std::string &base, const std::string &name);
    void setPath(const std::string &base, const std::string &name,
                 const std::string &ext);
    bool load();
    bool isLoaded() const;

    uint32_t getTexture() const;
};

using PImage = std::shared_ptr<Image>;
