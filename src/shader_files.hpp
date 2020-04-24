#pragma once

#include "common.hpp"
#include "image.hpp"
#include "shader_program.hpp"

#include <vector>
#include <memory>

namespace shader_editor {

class ShaderFiles {
   private:
    std::vector<std::shared_ptr<Image>> imageFiles;
    int32_t numImageFileNames = 0;
    char** imageFileNames = nullptr;

    std::vector<std::shared_ptr<ShaderProgram>> shaderFiles;
    int32_t numShaderFileNames = 0;
    char** shaderFileNames = nullptr;

   public:
    void deleteShaderFileNamse();

    void deleteImageFileNames();

    void genShaderFileNames();

    void genImageFileNames();

    char** getImageFileNames();

    char** getShaderFileNames();

    int32_t getNumImageFileNames() const;

    int32_t getNumShaderFileNames() const;

    std::shared_ptr<Image> getImage(int32_t index);
    std::shared_ptr<ShaderProgram> getShaderFile(int32_t index);

    int32_t pushNewProgram(std::shared_ptr<ShaderProgram> newProgram);
    int32_t pushNewImage(std::shared_ptr<Image> image);

    void replaceNewProgram(int32_t uiShaderFileIndex,
                           std::shared_ptr<ShaderProgram> newProgram);

    void ShaderFiles::loadFiles(const std::string& assetPath);
};
}  // namespace shader_editor
