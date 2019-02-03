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
    std::string preSource = "";
    bool ok = false;
    time_t mTime = 0;

   public:
    Shader() {}
    ~Shader() { reset(); }
    const std::string &getSource() const { return source; }
    const std::string &getError() const { return error; }
    const std::string &getPath() const { return path; }
    const std::string &getPreSource() const { return preSource; }
    bool isOK() const { return ok; }
    GLuint getType() const { return type; }
    GLuint getShader() const { return shader; }
    void setPreSource(const std::string &preSource);
    void reset();
    bool checkExpired() const;
    bool checkExpiredWithReset();

    bool compile(const std::string &path, GLuint type);
    bool compile(const std::string &path, GLuint type,
                 const std::string &source);
};

enum UniformType {
    Float,
    Integer,
    Vector1,
    Vector2,
    Vector3,
    Vector4,
    Sampler2D,
};

union ShaderUniformValue {
    int i;
    float f;
    glm::vec1 vec1;
    glm::vec2 vec2;
    glm::vec3 vec3;
    glm::vec4 vec4;
};

struct ShaderUniform {
    UniformType type;
    std::string name;
    GLint location;
    ShaderUniformValue value;

    ShaderUniform();
    const std::string toString() const;
};

struct ShaderAttribute {
    std::string name;
    GLint size;
    GLint location;
    GLenum type;
    GLboolean normalized;
    GLsizei stride;
    const void *pointer;
};

class ShaderProgram {
   private:
    Shader vertexShader;
    Shader fragmentShader;
    GLuint program = 0;
    double compileTime = 0;
    std::string error = "";

    std::map<const std::string, ShaderUniform> uniforms;
    std::map<const std::string, ShaderAttribute> attributes;

    bool ok = false;

    void attribute(const std::string &name, GLint location, GLint size,
                   GLenum type, GLboolean normalized, GLsizei stride,
                   const void *pointer);

    void attribute(const std::string &name, GLint size, GLenum type,
                   GLboolean normalized, GLsizei stride, const void *pointer);
    GLint uniform(const std::string &name, UniformType type);
    GLint uniform(const std::string &name, GLint location, UniformType type);
    void link();
    void setUniformInteger(const std::string &name, int value);
    void setUniformFloat(const std::string &name, float value);
    void setUniformVector2(const std::string &name, const glm::vec2 &value);
    void setUniformVector3(const std::string &name, const glm::vec3 &value);
    void setUniformVector4(const std::string &name, const glm::vec4 &value);

    void loadUniforms();
    void loadAttributes();

   public:
    ShaderProgram() {}
    ~ShaderProgram() { reset(); }

    void applyAttribute(const std::string &name);
    void applyAttributes();

    void setUniformValue(const std::string &name,
                         const ShaderUniformValue &value);
    void setUniformValue(const std::string &name, const glm::vec2 &value);
    void setUniformValue(const std::string &name, const glm::vec3 &value);
    void setUniformValue(const std::string &name, const glm::vec4 &value);
    void setUniformValue(const std::string &name, float value);
    void setUniformValue(const std::string &name, int value);

    void copyUniformsFrom(const ShaderProgram &program);
    void copyAttributesFrom(const ShaderProgram &program);
    void applyUniforms();

    std::map<const std::string, ShaderUniform> &getUniforms() {
        return uniforms;
    }

    void reset();
    GLuint getProgram() const { return program; }
    const Shader &getVertexShader() const { return vertexShader; }
    const Shader &getFragmentShader() const { return fragmentShader; }
    bool isOK() const { return ok; }
    bool checkExpired() const;
    const std::string &getError() const { return error; }

    bool checkExpiredWithReset();
    void setVertexShaderPreSource(const std::string preSource) {
        this->vertexShader.setPreSource(preSource);
    }
    void setFragmentShaderPreSource(const std::string preSource) {
        this->fragmentShader.setPreSource(preSource);
    }
    GLuint compile(const std::string &vsPath, const std::string &fsPath);
    GLuint compile(const std::string &vsPath, const std::string &fsPath,
                   const std::string &vsSource, const std::string &fsSource);
};
