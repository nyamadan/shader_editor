#pragma once

#include "common.hpp"
#include "image.hpp"
#include "shader_program.hpp"

#include <vector>
#include <memory>

namespace shader_editor {

class ShaderFiles {
   private:
    std::vector<PImage> imageFiles;
    int32_t numImageFileNames = 0;
    char** imageFileNames = nullptr;

    std::vector<PShaderProgram> shaderFiles;
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

    PImage getImage(int32_t index);
    PShaderProgram getShaderFile(int32_t index);

    int32_t pushNewProgram(PShaderProgram newProgram);
    int32_t pushNewImage(PImage image);

    void replaceNewProgram(int32_t uiShaderFileIndex,
                           PShaderProgram newProgram);

    void loadFiles(const std::string& assetPath);
};
}  // namespace shader_editor
