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
    uint32_t textureId = 0;
    uint8_t *data = nullptr;

   public:
    Image() {}
    ~Image() {
        if (data != nullptr) {
            stbi_image_free(data);
            data = nullptr;
        }

        if (textureId != 0) {
            glDeleteTextures(1, &textureId);
            textureId = 0;
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

    bool load() {
        stbi_set_flip_vertically_on_load(1);
        uint8_t *data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        if (data == nullptr) {
            return false;
        }

        this->data = data;

        if (textureId == 0) {
            glCreateTextures(GL_TEXTURE_2D, 1, &this->textureId);
        }

        glBindTexture(GL_TEXTURE_2D, textureId);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        switch (channels) {
            case 3:
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                             GL_UNSIGNED_BYTE, data);
                break;
            case 4:
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                             GL_RGBA, GL_UNSIGNED_BYTE, data);
                break;
            default:
                AppLog::getInstance().addLog("Texture Channel Error: %d\n", channels);
                break;
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

        return true;
    }

    bool isLoaded() const { return this->data != nullptr; }

    uint32_t getTexture() const { return this->textureId; }
};
