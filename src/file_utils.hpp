#include "common.hpp"

#include "shader_program.hpp"

#include <string>
#include <vector>
#include <memory>

bool readText(const std::string &path, std::string &memblock);
void writeText(const std::string &path, const char *const memblock,
               uint32_t size);
int64_t getMTime(const std::string &path);

std::vector<std::string> openDir(std::string path);

void compileShaderFromFile(const shader_editor::PShaderProgram program,
                           const std::string &vsPath,
                           const std::string &fsPath);

void recompileFragmentShader(const shader_editor::PShaderProgram program,
                             shader_editor::PShaderProgram newProgram,
                             const std::string &fsSource);

void recompileShaderFromFile(const shader_editor::PShaderProgram program,
                             shader_editor::PShaderProgram newProgram);
