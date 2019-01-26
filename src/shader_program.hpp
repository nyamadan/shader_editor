#pragma once

#include "common.hpp"

#include <map>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

class Shader {
   private:
    GLuint shader = 0;
    GLuint type = 0;
    std::string path = "";
    std::string source = "";
    std::string error = "";
    bool ok = false;
    time_t mTime = 0;

   public:
    Shader() {}
    ~Shader() { reset(); }
    const std::string &getSource() const { return source; }
    const std::string &getError() const { return error; }
    const std::string &getPath() const { return path; }
    GLuint getType() const { return type; }
    GLuint getShader() const { return shader; }
    bool checkExpired() const;

    void reset();
    bool compile(const char *const path, GLuint type);
};

enum UniformType {
    Float,
    Vector1,
    Vector2,
    Vector3,
    Vector4,
    Sampler2D,
};

union ShaderUniformValue {
    glm::vec1 vec1;
    glm::vec2 vec2;
    glm::vec3 vec3;
    glm::vec4 vec4;
    int sampler2d;
};

struct ShaderUniform {
    UniformType type;
    std::string name;
    GLint location;

    ShaderUniformValue value;
};

class ShaderProgram {
   private:
    Shader vertexShader;
    Shader fragmentShader;
    GLuint program = 0;
    std::string error = "";
    std::map<std::string, ShaderUniform> uniforms;
    bool ok = false;

   public:
    ShaderProgram() {}
    ~ShaderProgram() { reset(); }

    GLint uniform(const char *const name, UniformType type);

    GLuint getProgram() const { return program; }
    bool checkExpired() const {
        return vertexShader.checkExpired() || fragmentShader.checkExpired();
    };
    const std::string &getError() const { return error; }

    void reset();

    GLuint compile(const char *const vsPath, const char *const fsPath);
};
