#include "common.hpp"
#include "shader_utils.hpp"
#include "app_log.hpp"
#include "shader_program.hpp"
#include "file_utils.hpp"

#include <iostream>

#ifdef __EMSCRIPTEN__
const char *const GlslVersion = "#version 300 es\n";
#else
const char *const GlslVersion = "#version 330 core\n";
#endif

GLint checkLinked(GLuint program) {
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    return success;
}

GLint checkLinked(GLuint program, std::string &error) {
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success == GL_FALSE) {
        GLint max_len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_len);

        char *err_log = new char[max_len];
        glGetProgramInfoLog(program, max_len, &max_len, err_log);

        error = err_log;

        delete[] err_log;
    }

    return success;
}

GLint checkCompiled(GLuint shader) {
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    return success;
};

GLint checkCompiled(GLuint shader, std::string &error) {
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE) {
        GLint max_len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_len);

        char *err_log = new char[max_len];

        glGetShaderInfoLog(shader, max_len, &max_len, err_log);

        error = err_log;

        delete[] err_log;
    }

    return success;
};

void compileShaderFromFile(
    const std::shared_ptr<ShaderProgram> program, const std::string& vsPath,
    const std::string& fsPath) {
    const int64_t vsTime = getMTime(vsPath);
    const int64_t fsTime = getMTime(fsPath);
    std::string vsSource;
    std::string fsSource;
    readText(vsPath, vsSource);
    readText(fsPath, fsSource);
    program->compile(vsPath, fsPath, vsSource, fsSource, vsTime, fsTime);
}

void recompileFragmentShader(
    const std::shared_ptr<ShaderProgram> program,
    std::shared_ptr<ShaderProgram> newProgram, const std::string& fsSource) {
    const std::string& vsPath = program->getVertexShader().getPath();
    const std::string& fsPath = program->getFragmentShader().getPath();
    const int64_t vsTime = getMTime(vsPath);
    const int64_t fsTime = getMTime(fsPath);
    const std::string& vsSource = program->getVertexShader().getSource();
    newProgram->compile(vsPath, fsPath, vsSource, fsSource, vsTime, fsTime);
}

void recompileShaderFromFile(
    const std::shared_ptr<ShaderProgram> program,
    std::shared_ptr<ShaderProgram> newProgram) {
    const std::string& vsPath = program->getVertexShader().getPath();
    const std::string& fsPath = program->getFragmentShader().getPath();
    const int64_t vsTime = getMTime(vsPath);
    const int64_t fsTime = getMTime(fsPath);
    std::string vsSource;
    std::string fsSource;
    readText(vsPath, vsSource);
    readText(fsPath, fsSource);
    newProgram->compile(vsPath, fsPath, vsSource, fsSource, vsTime, fsTime);
}
