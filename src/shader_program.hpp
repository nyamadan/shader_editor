#pragma once

#include "common.hpp"
#include <map>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

class Shader {
   private:
    GLuint shader = 0;
    GLuint type = 0;
    std::string path = "(empty)";
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
    bool checkExpiredWithReset();

    void reset();
    bool compile(const std::string &path, GLuint type);
    bool compileWithSource(const std::string &path, const std::string &source,
                           GLuint type);
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

    void link();

   public:
    ShaderProgram() {}
    ~ShaderProgram() { reset(); }

    GLint uniform(const char *const name, UniformType type);

    GLuint getProgram() const { return program; }
    const Shader &getVertexShader() { return vertexShader; }
    const Shader &getFragmentShader() { return fragmentShader; }
    bool isOK() const { return ok; }
    bool checkExpired() const;
    const std::string &getError() const { return error; }

    void reset();
    bool checkExpiredWithReset();
    GLuint compile(const std::string &vsPath, const std::string &fsPath);
    GLuint compileWithSource(const std::string& vsPath,
                             const std::string& vsSource,
                             const std::string& fsPath,
                             const std::string& fsSource);
};
