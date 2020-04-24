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
std::string getExtention(const std::string &path);

void compileShaderFromFile(const std::shared_ptr<ShaderProgram> program,
                           const std::string &vsPath,
                           const std::string &fsPath);

void recompileFragmentShader(const std::shared_ptr<ShaderProgram> program,
                             std::shared_ptr<ShaderProgram> newProgram,
                             const std::string &fsSource);

void recompileShaderFromFile(const std::shared_ptr<ShaderProgram> program,
                             std::shared_ptr<ShaderProgram> newProgram);
