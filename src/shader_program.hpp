#pragma once

#include "common.hpp"

class Shader {
   private:
    GLuint shader = 0;
    GLuint type = 0;
    std::string path = "";
    std::string source = "";
    std::string error = "";
    bool ok = false;

   public:
    Shader() {}
    ~Shader() { reset(); }
    const std::string &getSource() const { return source; }
    const std::string &getError() const { return error; }
    const std::string &getPath() const { return path; }
    GLuint getType() const { return type; }
    GLuint getShader() const { return shader; }

    void reset();

    bool compile(const char *const path, GLuint type);
};

class ShaderProgram {
   private:
    Shader vertexShader;
    Shader fragmentShader;
    GLuint program = 0;
    std::string error = "";
    bool ok = false;

   public:
    ShaderProgram() {}
    ~ShaderProgram() { reset(); }

    GLuint getProgram() const { return program; }
    const std::string &getError() const { return error; }

    void reset();

    GLuint compile(const char *const vsPath, const char *const fsPath);
};
