#include "shader_files.hpp"
#include "default_shader.hpp"
#include "app_log.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace shader_editor {
void ShaderFiles::deleteShaderFileNamse() {
    if (shaderFileNames != nullptr) {
        for (int64_t i = 0; i < numShaderFileNames; i++) {
            delete[] shaderFileNames[i];
        }
        delete[] shaderFileNames;
    }
    shaderFileNames = nullptr;
}

void ShaderFiles::deleteImageFileNames() {
    if (imageFileNames != nullptr) {
        for (int64_t i = 0; i < numImageFileNames; i++) {
            delete[] imageFileNames[i];
        }
        delete[] imageFileNames;
    }
    imageFileNames = nullptr;
}

void ShaderFiles::genShaderFileNames() {
    deleteShaderFileNamse();

    numShaderFileNames = static_cast<int32_t>(shaderFiles.size());
    shaderFileNames = new char*[numShaderFileNames];

    for (int64_t i = 0; i < numShaderFileNames; i++) {
        std::string fileStr = shaderFiles[i]->getFragmentShader().getPath();
        const int64_t fileNameLength = fileStr.size();
        char* fileName = new char[fileNameLength + 1];
        fileName[fileNameLength] = '\0';
        fileStr.copy(fileName, fileNameLength, 0);
        shaderFileNames[i] = fileName;
    }
}

void ShaderFiles::genImageFileNames() {
    deleteImageFileNames();

    numImageFileNames = static_cast<int32_t>(imageFiles.size());
    imageFileNames = new char*[numImageFileNames];
    for (int64_t i = 0; i < numImageFileNames; i++) {
        std::shared_ptr<Image> image = imageFiles[i];
        const std::string& path = image->getPath();
        const int64_t fileNameLength = path.size();
        char* fileName = new char[fileNameLength + 1];
        fileName[fileNameLength] = '\0';
        image->getPath().copy(fileName, fileNameLength, 0);
        imageFileNames[i] = fileName;
    }
}

char ** ShaderFiles::getImageFileNames() { return imageFileNames; }

char ** ShaderFiles::getShaderFileNames() { return shaderFileNames; }

int32_t ShaderFiles::getNumImageFileNames() const { return numImageFileNames; }

int32_t ShaderFiles::getNumShaderFileNames() const { return numShaderFileNames; }

std::shared_ptr<Image> ShaderFiles::getImage(int32_t index) { return imageFiles[index]; }

std::shared_ptr<ShaderProgram> ShaderFiles::getShaderFile(int32_t index) {
    return shaderFiles[index];
}

int32_t ShaderFiles::pushNewProgram(std::shared_ptr<ShaderProgram> newProgram) {
    shaderFiles.push_back(newProgram);
    genShaderFileNames();
    return numShaderFileNames - 1;
}

int32_t ShaderFiles::pushNewImage(std::shared_ptr<Image> newImage) {
    imageFiles.push_back(newImage);
    genImageFileNames();
    return numImageFileNames - 1;
}

void ShaderFiles::replaceNewProgram(int32_t uiShaderFileIndex,
                       std::shared_ptr<ShaderProgram> newProgram) {
    shaderFiles[uiShaderFileIndex] = newProgram;
}

void ShaderFiles::loadFiles(const std::string& path) {
    AppLog::getInstance().addLog("Assets:\n");

    std::vector<std::string> files = openDir(path);
    for (auto iter = files.begin(); iter != files.end(); iter++) {
        const std::string& file = *iter;
        std::string ext = fs::path(file).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        AppLog::getInstance().addLog("%s(%s)\n", iter->c_str(), ext.c_str());

        if (ext == ".jpg" || ext == ".jpeg" || ext == ".png") {
            std::shared_ptr<Image> image = std::make_shared<Image>();
            image->setPath(path, *iter, ext);
            pushNewImage(image);
        }

        if (ext == ".glsl" || ext == ".frag") {
            std::shared_ptr<ShaderProgram> newProgram =
                std::make_shared<ShaderProgram>();

            std::string fragmentShaderPath = fs::path(path).append(file).string();

            const int64_t fsTime = getMTime(fragmentShaderPath);
            std::string fsSource;
            readText(fragmentShaderPath, fsSource);
            newProgram->setCompileInfo("<default-vertex-shader>", fragmentShaderPath, DefaultVertexShaderSource, fsSource, 0, fsTime);
            pushNewProgram(newProgram);
        }
    }
}
}