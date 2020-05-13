#pragma once

#include "common.hpp"
#include <map>
#include <string>
#include <memory>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "../glslang/glslang/Include/ShHandle.h"

namespace shader_editor {

class CompileError {
   private:
    std::string original;
    std::string type;
    std::string fileName;
    int32_t lineNumber;
    std::string message;
    CompileError() {}

   public:
    CompileError(std::string _original, std::string _type,
                 std::string _fileName, int32_t _lineNumber,
                 std::string _message)
        : original(_original),
          type(_type),
          fileName(_fileName),
          lineNumber(_lineNumber),
          message(_message) {}
    const std::string &getOriginal() const { return original; }
    const std::string &getType() const { return type; }
    int32_t getLineNumber() const { return lineNumber; }
    const std::string &getMessage() const { return message; }
    const std::string &getFileName() const { return fileName; }
};

class Shader {
   private:
    GLuint shader = 0;
    GLuint type = 0;
    std::string path = "";
    std::string source = "";
    std::string sourceTemplate = "";
    std::string compiledSource = "";
    std::vector<std::shared_ptr<Shader>> dependencies;
    std::vector<CompileError> errors;
    bool ok = false;
    int64_t mTime = 0;

    bool preCompile(int32_t version, bool isGlslEs, std::string &combinedSource);
    void parseGlslangErrors(const std::string &error);
    bool scanVersion(const std::string &source, int &version, EProfile &profile,
                     bool &notFirstToken);
    bool parseSharderError(const std::string &line);
    bool parseProgramError(const std::string &line);

   public:
    Shader() {}
    ~Shader() { reset(); }
    const std::string &getSource() const { return source; }
    const std::vector<CompileError> &getErrors() const { return errors; }
    const std::string &getPath() const { return path; }
    const std::string &getSourceTemplate() const { return sourceTemplate; }
    bool isOK() const { return ok; }
    bool getCompilable();
    GLuint getType() const { return type; }
    GLuint getShader() const { return shader; }
    void setSourceTemplate(const std::string &sourceTemplate);
    void reset();
    bool checkExpired() const;
    bool checkExpiredWithReset();

    void setCompileInfo(const std::string &path, GLuint type,
                        const std::string &source, int64_t mTime);
    bool compile(int32_t targetVersion, bool isGlslEs);
};

enum UniformType {
    Float,
    Integer,
    Vector1,
    Vector2,
    Vector3,
    Vector4,
    Mat3x3,
    Mat4x4,
    Sampler2D,
};

union ShaderUniformValue {
    int32_t i;
    float f;
    glm::vec1 vec1;
    glm::vec2 vec2;
    glm::vec3 vec3;
    glm::vec4 vec4;
    glm::mat3x3 mat3x3;
    glm::mat4x4 mat4x4;
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
    void uniform(const std::string &name, UniformType type);
    void uniform(const std::string &name, GLint location, UniformType type);
    void link();
    void setUniformInteger(const std::string &name, int value);
    void setUniformFloat(const std::string &name, float value);
    void setUniformVector2(const std::string &name, const glm::vec2 &value);
    void setUniformVector3(const std::string &name, const glm::vec3 &value);
    void setUniformVector4(const std::string &name, const glm::vec4 &value);
    void setUniformMat3x3(const std::string &name, const glm::mat3x3 &value);
    void setUniformMat4x4(const std::string &name, const glm::mat4x4 &value);

    void loadUniforms();
    void loadAttributes();

   public:
    ShaderProgram() {}
    ~ShaderProgram() { reset(); }

    void applyAttribute(const std::string &name);
    void applyAttributes();

    bool containsUniform(const std::string &name) const;
    const ShaderUniform &getUniform(const std::string &name) const;
    void setUniformValue(const std::string &name,
                         const ShaderUniformValue &value);
    void setUniformValue(const std::string &name, const glm::vec2 &value);
    void setUniformValue(const std::string &name, const glm::vec3 &value);
    void setUniformValue(const std::string &name, const glm::vec4 &value);
    void setUniformValue(const std::string &name, const glm::mat3x3 &value);
    void setUniformValue(const std::string &name, const glm::mat4x4 &value);
    void setUniformValue(const std::string &name, float value);
    void setUniformValue(const std::string &name, int32_t value);

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
    void setVertexShaderSourceTemplate(const std::string sourceTemplate) {
        this->vertexShader.setSourceTemplate(sourceTemplate);
    }
    void setFragmentShaderSourceTemplate(const std::string sourceTemplate) {
        this->fragmentShader.setSourceTemplate(sourceTemplate);
    }

    void setCompileInfo(const std::string &vsPath, const std::string &fsPath,
                        const std::string &vsSource,
                        const std::string &fsSource, int64_t vsMTime,
                        int64_t fsMTime);
    GLuint compile();
    GLuint compile(const std::string &vsPath, const std::string &fsPath,
                   const std::string &vsSource, const std::string &fsSource,
                   int64_t vsMTime, int64_t fsMTime);
};
}  // namespace shader_editor
