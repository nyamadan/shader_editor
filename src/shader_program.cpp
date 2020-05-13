#include "common.hpp"
#include "app_log.hpp"
#include "shader_utils.hpp"
#include "file_utils.hpp"
#include "shader_program.hpp"
#include "shader_compiler.hpp"
#include "default_shader.hpp"

#include <regex>
#include <sstream>

#include "../glslang/glslang/MachineIndependent/Scan.h"
namespace {
#ifdef __EMSCRIPTEN__
const int32_t TargetShaderVersion = 100;
const bool IsGlslEs = true;
#else
const int32_t TargetShaderVersion = 420;
const bool IsGlslEs = false;
#endif
}  // namespace

namespace shader_editor {
bool Shader::scanVersion(const std::string &shaderSource, int &version,
                         EProfile &profile, bool &notFirstToken) {
    const char *const s[] = {shaderSource.c_str()};
    size_t l[] = {shaderSource.length()};
    glslang::TInputScanner userInput(1, s, l);
    return userInput.scanVersion(version, profile, notFirstToken);
}

bool Shader::parseSharderError(const std::string &line) {
    const std::regex reShader("^([^:]+):\\s+(.+):(\\d+):(.*)$");
    std::smatch m;
    std::regex_match(line, m, reShader);
    if (m.size() == 5) {
        const auto errorType = m[1];
        const auto fileName = m[2];
        const auto lineNumber = std::atoi(m[3].str().c_str());
        const std::string message = m[4].str();

        errors.push_back(
            CompileError(line, errorType, fileName, lineNumber, message));
        return false;
    }

    return true;
}

bool Shader::parseProgramError(const std::string &line) {
    const std::regex reShader("^([^:]+):\\s+(.+)$");
    std::smatch m;
    std::regex_match(line, m, reShader);
    if (m.size() == 3) {
        const auto errorType = m[1];
        const std::string message = m[2].str();

        errors.push_back(CompileError(line, errorType, "", -1, message));
        return false;
    }

    return true;
}

void Shader::parseGlslangErrors(const std::string &error) {
    std::istringstream ss(error);
    std::string line;

    errors.clear();

    while (std::getline(ss, line)) {
        parseSharderError(line) && parseProgramError(line);
    }
}

void Shader::setSourceTemplate(const std::string &sourceTemplate) {
    this->sourceTemplate = sourceTemplate;
}

void Shader::reset() {
    if (shader != 0) {
        glDeleteShader(shader);
    }

    errors.clear();
    type = 0;
    shader = 0;
    path = "";
    source = "";
    sourceTemplate = "";
    ok = false;

    mTime = 0;
}

void Shader::setCompileInfo(const std::string &path, GLuint type,
                            const std::string &source, int64_t mTime) {
    this->type = type;
    this->path = path;
    this->mTime = mTime;
    this->source = source;
}

bool Shader::compile(int32_t targetVersion, bool isGlslEs) {
    if (shader != 0) {
        glDeleteShader(shader);
        shader = 0;
    }

    if (!preCompile(TargetShaderVersion, IsGlslEs, compiledSource)) {
        return false;
    }

    shader = glCreateShader(type);
    const char *const pCompiledSource = compiledSource.c_str();

    glShaderSource(shader, 1, &pCompiledSource, NULL);
    glCompileShader(shader);

    std::string error;
    if (!checkCompiled(shader, error)) {
        AppLog::getInstance().error(error.c_str());
        shader = 0;
        return false;
    }

    errors.clear();

    ok = true;

    return true;
}

bool Shader::getCompilable()
{
    bool useTemplate = !sourceTemplate.empty();

    int version;
    EProfile profile;
    bool notFirstToken;
    scanVersion(useTemplate ? sourceTemplate : source, version, profile,
                notFirstToken);

    switch (profile) {
        case ENoProfile:
        case ECoreProfile:
        case ECompatibilityProfile: {
            return version >= 330;
        } break;
        case EEsProfile: {
            return version >= 310;
        } break;
        default: {
            return false;
        } break;
    }
}

bool Shader::preCompile(int32_t version, bool isGlslEs, std::string &compiledShaderSource) {
    bool isLinked = false;
    std::string error;

    if (getCompilable()) {
        shader_compiler::CompileResult compileResult;
        switch (type) {
            case GL_VERTEX_SHADER: {
                shader_compiler::compile(EShLangVertex, version,
                                         isGlslEs, path, source, sourceTemplate,
                                         compileResult);
            } break;

            case GL_FRAGMENT_SHADER: {
                shader_compiler::compile(EShLangFragment, version,
                                         isGlslEs, path, source, sourceTemplate,
                                         compileResult);
            } break;
        }

        isLinked = compileResult.isCompiled && compileResult.isLinked;
        error = compileResult.shaderLog + "\n" + compileResult.programLog;

        dependencies.clear();

        for (auto it = compileResult.dependencies.cbegin();
             it != compileResult.dependencies.cend(); it++) {
            const auto &path = *it;
            const auto shader = std::make_shared<Shader>();

            const int64_t mTime = getMTime(path);
            std::string source = "";
            readText(path, source);
            shader->setCompileInfo(path, GL_FRAGMENT_SHADER, source, mTime);
            dependencies.push_back(shader);
        }

        if (isLinked) {
            compiledShaderSource = compileResult.sourceCode;
        }
    } else {
        shader_compiler::ValidateResult validationResult;
        switch (type) {
            case GL_VERTEX_SHADER: {
                shader_compiler::validate(EShLangVertex, IsGlslEs, path, source,
                                          validationResult);
            } break;

            case GL_FRAGMENT_SHADER: {
                shader_compiler::validate(EShLangFragment, IsGlslEs, path,
                                          source, validationResult);
            } break;
        }

        isLinked = validationResult.isCompiled && validationResult.isLinked;
        error = validationResult.shaderLog + "\n" + validationResult.programLog;

        dependencies.clear();

        if (isLinked) {
            compiledShaderSource = source;
        }
    }

    if (!isLinked) {
        parseGlslangErrors(error);
        shader = 0;
        return false;
    }

    return true;
}

bool Shader::checkExpired() const {
    auto expired = false;
    expired |= getMTime(this->path) != this->mTime;

    for (auto it = dependencies.cbegin(); it != dependencies.cend(); it++) {
        expired |= (*it)->checkExpired();
    }

    return expired;
}

bool Shader::checkExpiredWithReset() {
    auto expired = false;

    int64_t mTime = getMTime(this->path.c_str());
    expired |= mTime != this->mTime;
    this->mTime = mTime;

    for (auto it = dependencies.begin(); it != dependencies.end(); it++) {
        expired |= (*it)->checkExpiredWithReset();
    }

    return expired;
}

void ShaderProgram::attribute(const std::string &name, GLint size, GLenum type,
                              GLboolean normalized, GLsizei stride,
                              const void *pointer) {
    if (!isOK()) {
        return;
    }

    int32_t location = glGetAttribLocation(program, name.c_str());
    attribute(name, location, size, type, normalized, stride, pointer);
}

void ShaderProgram::attribute(const std::string &name, GLint location,
                              GLint size, GLenum type, GLboolean normalized,
                              GLsizei stride, const void *pointer) {
    ShaderAttribute &attr = this->attributes[name];
    attr.name = name;
    attr.size = size;
    attr.type = type;
    attr.normalized = normalized;
    attr.stride = stride;
    attr.pointer = pointer;
    attr.location = location;
}

void ShaderProgram::applyAttribute(const std::string &name) {
    const ShaderAttribute &attr = attributes[name];

    if (attr.location < 0) {
        return;
    }

    glVertexAttribPointer(attr.location, attr.size, attr.type, attr.normalized,
                          attr.stride, attr.pointer);
}

void ShaderProgram::applyAttributes() {
    for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
        const ShaderAttribute &attr = iter->second;

        if (attr.location < 0) {
            continue;
        }

        glEnableVertexAttribArray(attr.location);
        glVertexAttribPointer(attr.location, attr.size, attr.type,
                              attr.normalized, attr.stride, attr.pointer);
    }
}

void ShaderProgram::uniform(const std::string &name, UniformType type) {
    if (!isOK()) {
        return;
    }

    int32_t location = glGetUniformLocation(program, name.c_str());
    uniform(name, location, type);
}

void ShaderProgram::uniform(const std::string &name, GLint location,
                            UniformType type) {
    ShaderUniform &u = uniforms[name];
    u.name = name;
    u.location = location;
    u.type = type;
}

void ShaderProgram::setUniformInteger(const std::string &name, int value) {
    ShaderUniform &u = uniforms[name];
    u.name = name;
    u.value.i = value;
}

void ShaderProgram::setUniformFloat(const std::string &name, float value) {
    ShaderUniform &u = uniforms[name];
    u.name = name;
    u.value.f = value;
}

void ShaderProgram::setUniformVector2(const std::string &name,
                                      const glm::vec2 &value) {
    ShaderUniform &u = uniforms[name];
    u.name = name;
    u.value.vec2 = value;
}

void ShaderProgram::setUniformVector3(const std::string &name,
                                      const glm::vec3 &value) {
    ShaderUniform &u = uniforms[name];
    u.name = name;
    u.value.vec3 = value;
}

void ShaderProgram::setUniformVector4(const std::string &name,
                                      const glm::vec4 &value) {
    ShaderUniform &u = uniforms[name];
    u.name = name;
    u.value.vec4 = value;
}

void ShaderProgram::setUniformMat3x3(const std::string &name,
                                     const glm::mat3x3 &value) {
    ShaderUniform &u = uniforms[name];
    u.name = name;
    u.value.mat3x3 = value;
}

void ShaderProgram::setUniformMat4x4(const std::string &name,
                                     const glm::mat4x4 &value) {
    ShaderUniform &u = uniforms[name];
    u.name = name;
    u.value.mat4x4 = value;
}

void ShaderProgram::setUniformValue(const std::string &name,
                                    const glm::vec2 &value) {
    this->setUniformVector2(name, value);
}

void ShaderProgram::setUniformValue(const std::string &name,
                                    const glm::vec3 &value) {
    this->setUniformVector3(name, value);
}

void ShaderProgram::setUniformValue(const std::string &name,
                                    const glm::vec4 &value) {
    this->setUniformVector4(name, value);
}

void ShaderProgram::setUniformValue(const std::string &name,
                                    const glm::mat3x3 &value) {
    this->setUniformMat3x3(name, value);
}

void ShaderProgram::setUniformValue(const std::string &name,
                                    const glm::mat4x4 &value) {
    this->setUniformMat4x4(name, value);
}

bool ShaderProgram::containsUniform(const std::string &name) const {
    return uniforms.count(name) != 0;
}

const ShaderUniform &ShaderProgram::getUniform(const std::string &name) const {
    const ShaderUniform &u = uniforms.at(name);
    return u;
}

void ShaderProgram::setUniformValue(const std::string &name,
                                    const ShaderUniformValue &value) {
    ShaderUniform &u = uniforms[name];
    u.name = name;
    u.value = value;
}

void ShaderProgram::setUniformValue(const std::string &name, float value) {
    this->setUniformFloat(name, value);
}

void ShaderProgram::setUniformValue(const std::string &name, int32_t value) {
    this->setUniformInteger(name, value);
}

void ShaderProgram::copyAttributesFrom(const ShaderProgram &program) {
    for (auto iter = program.attributes.begin();
         iter != program.attributes.end(); iter++) {
        const std::string &name = iter->first;
        const ShaderAttribute &attr = iter->second;

        this->attribute(name, attr.size, attr.type, attr.normalized,
                        attr.stride, attr.pointer);
    }
}

void ShaderProgram::copyUniformsFrom(const ShaderProgram &program) {
    for (auto iter = program.uniforms.begin(); iter != program.uniforms.end();
         iter++) {
        const std::string &name = iter->first;
        const ShaderUniform &u = iter->second;

        setUniformValue(name, u.value);
    }
}

void ShaderProgram::applyUniforms() {
    for (auto iter = uniforms.begin(); iter != uniforms.end(); iter++) {
        const ShaderUniform &u = iter->second;

        if (u.location < 0) {
            continue;
        }

        switch (u.type) {
            case UniformType::Float:
                glUniform1f(u.location, u.value.f);
                break;
            case UniformType::Integer:
            case UniformType::Sampler2D:
                glUniform1i(u.location, u.value.i);
                break;
            case UniformType::Vector1:
                break;
            case UniformType::Vector2:
                glUniform2fv(u.location, 1, glm::value_ptr(u.value.vec2));
                break;
            case UniformType::Vector3:
                glUniform3fv(u.location, 1, glm::value_ptr(u.value.vec3));
                break;
            case UniformType::Vector4:
                glUniform4fv(u.location, 1, glm::value_ptr(u.value.vec4));
                break;
            case UniformType::Mat3x3:
                glUniformMatrix3fv(u.location, 1, false, glm::value_ptr(u.value.mat3x3));
                break;
            case UniformType::Mat4x4:
                glUniformMatrix4fv(u.location, 1, false, glm::value_ptr(u.value.mat4x4));
                break;
        }
    }
}

void ShaderProgram::reset() {
    if (program != 0) {
        glDeleteProgram(program);
    }
    vertexShader.reset();
    fragmentShader.reset();
    attributes.clear();
    uniforms.clear();
    compileTime = 0;
    program = 0;
    error = "";
    ok = false;
}

bool ShaderProgram::checkExpired() const {
    return vertexShader.checkExpired() || fragmentShader.checkExpired();
}

bool ShaderProgram::checkExpiredWithReset() {
    auto expiredVS = vertexShader.checkExpiredWithReset();
    auto expiredFS = fragmentShader.checkExpiredWithReset();
    return expiredVS || expiredFS;
}

void ShaderProgram::setCompileInfo(const std::string &vsPath,
                                   const std::string &fsPath,
                                   const std::string &vsSource,
                                   const std::string &fsSource, int64_t vsMTime,
                                   int64_t fsMTime) {
    vertexShader.setCompileInfo(vsPath, GL_VERTEX_SHADER, vsSource, vsMTime);
    fragmentShader.setCompileInfo(fsPath, GL_FRAGMENT_SHADER, fsSource,
                                  fsMTime);
}

GLuint ShaderProgram::compile() {
    if (program != 0) {
        glDeleteProgram(program);
        program = 0;
    }

    AppLog::getInstance().info("(%s, %s): Shader compilation started\n",
                               vertexShader.getPath().c_str(),
                               fragmentShader.getPath().c_str());

    double t0 = glfwGetTime();

    if (!vertexShader.compile(TargetShaderVersion, IsGlslEs)) {
        const auto &errors = vertexShader.getErrors();
        for (auto iter = errors.begin(); errors.end() != iter; iter++) {
            AppLog::getInstance().error("%s\n", iter->getOriginal().c_str());
        }

        AppLog::getInstance().error("(%s): Shader compilation failed\n",
                                    vertexShader.getPath().c_str());
        return 0;
    }

    if (!fragmentShader.compile(TargetShaderVersion, IsGlslEs)) {
        const auto &errors = fragmentShader.getErrors();
        for (auto iter = errors.begin(); errors.end() != iter; iter++) {
            AppLog::getInstance().error("%s\n", iter->getOriginal().c_str());
        }

        AppLog::getInstance().error("(%s): Shader compilation failed\n",
                                    fragmentShader.getPath().c_str());
        return 0;
    }

    link();

    if (!checkLinked(program, error)) {
        AppLog::getInstance().error("Program linking failed:%s\n",
                                    error.c_str());
        AppLog::getInstance().error("(%s): Shader compilation failed\n",
                                    fragmentShader.getPath().c_str());
        return 0;
    }

    ok = true;

    loadAttributes();
    loadUniforms();

    compileTime = glfwGetTime() - t0;

    AppLog::getInstance().info("(%s, %s): Program linking ok (%.2fs)\n",
                               vertexShader.getPath().c_str(),
                               fragmentShader.getPath().c_str(), compileTime);

    return program;
}

GLuint ShaderProgram::compile(const std::string &vsPath,
                              const std::string &fsPath,
                              const std::string &vsSource,
                              const std::string &fsSource, int64_t vsMTime,
                              int64_t fsMTime) {
    this->setCompileInfo(vsPath, fsPath, vsSource, fsSource, vsMTime, fsMTime);
    return this->compile();
}

void ShaderProgram::loadUniforms() {
    GLint count;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
    AppLog::getInstance().debug("Active Uniforms: %d\n", count);
    for (GLint i = 0; i < count; i++) {
        const GLsizei bufSize = 128;
        GLchar name[bufSize];
        GLsizei length;
        GLint size;
        GLenum type;

        glGetActiveUniform(program, (GLuint)i, bufSize, &length, &size, &type,
                           name);

        switch (type) {
            case GL_FLOAT:
                uniform(name, UniformType::Float);
                setUniformValue(name, 0.0f);
                AppLog::getInstance().debug(
                    "Uniform #%d Type: Float, Name: %s\n", i, name);
                break;
            case GL_FLOAT_VEC2:
                uniform(name, UniformType::Vector2);
                setUniformValue(name, glm::vec2(0.0f, 0.0f));
                AppLog::getInstance().debug(
                    "Uniform #%d Type: Vector2, Name: %s\n", i, name);
                break;
            case GL_FLOAT_VEC3:
                uniform(name, UniformType::Vector3);
                setUniformValue(name, glm::vec3(0.0f, 0.0f, 0.0f));
                AppLog::getInstance().debug(
                    "Uniform #%d Type: Vector3, Name: %s\n", i, name);
                break;
            case GL_FLOAT_VEC4:
                uniform(name, UniformType::Vector4);
                setUniformValue(name, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
                AppLog::getInstance().debug(
                    "Uniform #%d Type: Vector4, Name: %s\n", i, name);
                break;
            case GL_FLOAT_MAT3:
                uniform(name, UniformType::Mat3x3);
                setUniformValue(name, glm::mat3x3());
                AppLog::getInstance().debug(
                    "Uniform #%d Type: Mat3x3, Name: %s\n", i, name);
                break;
            case GL_FLOAT_MAT4:
                uniform(name, UniformType::Mat4x4);
                setUniformValue(name, glm::mat4x4());
                AppLog::getInstance().debug(
                    "Uniform #%d Type: Mat4x4, Name: %s\n", i, name);
                break;
            case GL_INT:
                uniform(name, UniformType::Integer);
                setUniformValue(name, 0);
                AppLog::getInstance().debug(
                    "Uniform #%d Type: Integer, Name: %s\n", i, name);
                break;
            case GL_SAMPLER_2D:
                uniform(name, UniformType::Sampler2D);
                setUniformValue(name, 0);
                AppLog::getInstance().debug(
                    "Uniform #%d Type: Sampler2D, Name: %s\n", i, name);
                break;
            default:
                AppLog::getInstance().debug(
                    "[Warning] Unsupported uniform #%d Type: 0x%04X Name: %s\n",
                    i, type, name);
                break;
        }
    }
}

void ShaderProgram::loadAttributes() {
    GLint count;

    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &count);
    AppLog::getInstance().debug("Active Attributes: %d\n", count);

    for (int i = 0; i < count; i++) {
        const GLsizei bufSize = 128;
        GLchar name[bufSize + 1] = {0};
        GLsizei length;
        GLint size;
        GLenum type;

        glGetActiveAttrib(program, (GLuint)i, bufSize, &length, &size, &type,
                          name);

        switch (type) {
            case GL_FLOAT_VEC2:
                attribute(name, i, 2, GL_FLOAT, false, 0, 0);
                break;
            case GL_FLOAT_VEC3:
                attribute(name, i, 3, GL_FLOAT, false, 0, 0);
                break;
            case GL_FLOAT_VEC4:
                attribute(name, i, 4, GL_FLOAT, false, 0, 0);
                break;
        }

        AppLog::getInstance().debug("Attribute #%d Type: %04X Name: %s\n", i,
                                    type, name);
    }
}

void ShaderProgram::link() {
    program = glCreateProgram();
    glAttachShader(program, vertexShader.getShader());
    glAttachShader(program, fragmentShader.getShader());
    glLinkProgram(program);
}

ShaderUniform::ShaderUniform() {
    this->location = -1;
    this->name = std::string();
    this->type = UniformType::Integer;
    memset(&value, 0, sizeof(value));
}

const std::string ShaderUniform::toString() const {
    std::stringstream ss;

    switch (type) {
        case UniformType::Float:
            ss << value.f;
            break;
        case UniformType::Integer:
        case UniformType::Sampler2D:
            ss << value.i;
            break;
        case UniformType::Vector2:
            ss << value.vec2.x << ", " << value.vec2.y;
            break;
        case UniformType::Vector3:
            ss << value.vec3.x << ", " << value.vec3.y << ", " << value.vec3.z;
            break;
        case UniformType::Vector4:
            ss << value.vec4.x << ", " << value.vec4.y << ", " << value.vec4.z
               << ", " << value.vec4.w;
            break;
        default:
            ss << value.i;
            break;
    }

    return ss.str();
}
}  // namespace shader_editor
