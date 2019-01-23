#include "common.hpp"
#include "app_log.hpp"
#include "shader_utils.hpp"
#include "file_utils.hpp"
#include "shader_program.hpp"

void Shader::reset() {
    if (shader != 0) {
        glDeleteShader(shader);
    }
    type = 0;
    shader = 0;
    path = "";
    source = "";
    error = "";
    ok = false;
}

bool Shader::compile(const char *const path, GLuint type) {
    reset();

    this->path = std::string(path);
    this->type = type;

    char *source = nullptr;
    readText(source, path);

    this->source = std::string(source);

    shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);

    glCompileShader(shader);

    delete[] source;

    if (!checkCompiled(shader, &error)) {
        glDeleteShader(shader);
        shader = 0;
        return false;
    }

    ok = true;

    return true;
}

void ShaderProgram::reset() {
    vertexShader.reset();
    fragmentShader.reset();
    program = 0;
    error = "";
    ok = false;
}

GLuint ShaderProgram::compile(const char *const vsPath,
                              const char *const fsPath) {
    reset();

    if (!vertexShader.compile(vsPath, GL_VERTEX_SHADER)) {
        GetAppLog().AddLog("(%s) %s", vertexShader.getPath().c_str(),
                           vertexShader.getError().c_str());
        return 0;
    }

    if (!fragmentShader.compile(fsPath, GL_FRAGMENT_SHADER)) {
        GetAppLog().AddLog("(%s) %s", fragmentShader.getPath().c_str(),
                           fragmentShader.getError().c_str());
        return 0;
    }

    program = glCreateProgram();
    glAttachShader(program, vertexShader.getShader());
    glAttachShader(program, fragmentShader.getShader());
    glLinkProgram(program);

    if (!checkLinked(program, &error)) {
        GetAppLog().AddLog("%s", error.c_str());
        return 0;
    }

    ok = true;

    return program;
}
