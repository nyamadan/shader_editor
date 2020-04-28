#include "common.hpp"
#include "shader_utils.hpp"
#include "app_log.hpp"
#include "shader_program.hpp"
#include "file_utils.hpp"
#include "default_shader.hpp"

#include <iostream>

#ifdef __EMSCRIPTEN__
const char* const GlslVersion = "#version 300 es\n";
#else
const char* const GlslVersion = "#version 330 core\n";
#endif

GLint checkLinked(GLuint program) {
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    return success;
}

GLint checkLinked(GLuint program, std::string& error) {
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success == GL_FALSE) {
        GLint max_len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_len);

        char* err_log = new char[max_len];
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

GLint checkCompiled(GLuint shader, std::string& error) {
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE) {
        GLint max_len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_len);

        char* err_log = new char[max_len];

        glGetShaderInfoLog(shader, max_len, &max_len, err_log);

        error = err_log;

        delete[] err_log;
    }

    return success;
};

void compileShaderFromFile(const std::shared_ptr<shader_editor::ShaderProgram> program,
                           const std::string& vsPath,
                           const std::string& fsPath) {
    int64_t vsTime = -1;
    int64_t fsTime = -1;
    std::string vsSource;
    std::string fsSource;

    if (vsPath == "<default-vertex-shader>") {
        vsSource = shader_editor::DefaultVertexShaderSource;
    } else {
        vsTime = getMTime(vsPath);
        readText(vsPath, vsSource);
    }

    if (fsPath == "<default-fragment-shader>") {
        fsSource = shader_editor::DefaultFragmentShaderSource;
    } else {
        fsTime = getMTime(fsPath);
        readText(fsPath, fsSource);
    }

    program->compile(vsPath, fsPath, vsSource, fsSource, vsTime, fsTime);
}

void recompileFragmentShader(const std::shared_ptr<shader_editor::ShaderProgram> program,
                             std::shared_ptr<shader_editor::ShaderProgram> newProgram,
                             const std::string& fsSource) {
    const std::string& vsPath = program->getVertexShader().getPath();
    const std::string& fsPath = program->getFragmentShader().getPath();
    int64_t vsTime = -1;
    int64_t fsTime = -1;
    const std::string& vsSource = program->getVertexShader().getSource();

    if (vsPath != "<default-vertex-shader>") {
        vsTime = getMTime(vsPath);
    }

    if (fsPath != "<default-fragment-shader>") {
        fsTime = getMTime(fsPath);
    }
    newProgram->compile(vsPath, fsPath, vsSource, fsSource, vsTime, fsTime);
}

void recompileShaderFromFile(const std::shared_ptr<shader_editor::ShaderProgram> program,
                             std::shared_ptr<shader_editor::ShaderProgram> newProgram) {
    const std::string& vsPath = program->getVertexShader().getPath();
    const std::string& fsPath = program->getFragmentShader().getPath();
    int64_t vsTime = -1;
    int64_t fsTime = -1;
    std::string vsSource;
    std::string fsSource;

    if (vsPath == "<default-vertex-shader>") {
        vsSource = shader_editor::DefaultVertexShaderSource;
    } else {
        vsTime = getMTime(vsPath);
        readText(vsPath, vsSource);
    }

    if (fsPath == "<default-fragment-shader>") {
        fsSource = shader_editor::DefaultFragmentShaderSource;
    } else {
        fsTime = getMTime(fsPath);
        readText(fsPath, fsSource);
    }

    newProgram->compile(vsPath, fsPath, vsSource, fsSource, vsTime, fsTime);
}
