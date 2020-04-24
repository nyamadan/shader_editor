#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <filesystem>

#include "image.hpp"
#include "app_log.hpp"

namespace fs = std::filesystem;

Image::Image() {}

Image::~Image() {
    if (data != nullptr) {
        stbi_image_free(data);
        data = nullptr;
    }

    if (textureId != 0) {
        glDeleteTextures(1, &textureId);
        textureId = 0;
    }
}

const std::string &Image::getPath() const { return path; }

void Image::setPath(const std::string &base, const std::string &name) {
    std::string ext = getExtention(base);
    this->setPath(base, name, ext);
}

void Image::setPath(const std::string &base, const std::string &name,
                    const std::string &ext) {
    this->base = base;
    this->name = name;
    this->ext = ext;

    this->path = fs::path(this->base).append(this->name).string();
}

bool Image::load() {
    stbi_set_flip_vertically_on_load(1);
    uint8_t *data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (data == nullptr) {
        return false;
    }

    this->data = data;

    if (textureId == 0) {
        glGenTextures(1, &this->textureId);
    }

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    switch (channels) {
        case 3:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
                         GL_UNSIGNED_BYTE, data);
            break;
        case 4:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, data);
            break;
        default:
            AppLog::getInstance().addLog("Texture Channel Error: %d\n",
                                         channels);
            break;
    }

    return true;
}

bool Image::isLoaded() const { return this->data != nullptr; }

uint32_t Image::getTexture() const { return this->textureId; }
