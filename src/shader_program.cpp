#include "common.hpp"
#include "app_log.hpp"
#include "shader_utils.hpp"
#include "file_utils.hpp"
#include "shader_program.hpp"

#include <regex>
#include <sstream>

void Shader::parseErrors(const std::string &error) {
    const auto preSourceLines =
        static_cast<int>(std::count(preSource.begin(), preSource.end(), '\n'));
#ifdef __EMSCRIPTEN__
    const std::regex re("^ERROR: \\d+:(\\d+): (.*)$");
#else
    const std::regex re("^\\d\\((\\d+)\\) : (.*)$");
#endif
    std::istringstream ss(error);
    std::string line;
    while (std::getline(ss, line)) {
        std::smatch m;
        std::regex_match(line, m, re);
        if (m.size() == 3) {
            const auto lineNumber =
                std::atoi(m[1].str().c_str()) - preSourceLines;
            const auto message = m[2].str();

            errors.insert(std::make_pair(lineNumber, message));
        }
    }
}

void Shader::setPreSource(const std::string &preSource) {
    this->preSource = preSource;
}

void Shader::reset() {
    if (shader != 0) {
        glDeleteShader(shader);
    }

    errors.clear();
    type = 0;
    shader = 0;
    path = "(empty)";
    source = "";
    preSource = "";
    ok = false;

    mTime = 0;
}

bool Shader::compile(const std::string &path, GLuint type,
                     const std::string &source, time_t mTime) {
    this->type = type;
    this->path = path;
    this->mTime = mTime;
    this->source = source;

    std::string combinedSource;

    combinedSource.append(preSource);
    combinedSource.append(source);

    shader = glCreateShader(type);
    const auto pCombinedSource = combinedSource.c_str();
    glShaderSource(shader, 1, &pCombinedSource, NULL);
    glCompileShader(shader);

    std::string error;

    if (!checkCompiled(shader, error)) {
        parseErrors(error);
        shader = 0;
        return false;
    }

    errors.clear();

    ok = true;

    return true;
}

bool Shader::checkExpired() const {
    auto mTime = getMTime(this->path);
    return mTime != this->mTime;
}

bool Shader::checkExpiredWithReset() {
    auto mTime = getMTime(this->path.c_str());
    auto expired = mTime != this->mTime;
    this->mTime = mTime;
    return expired;
}

void ShaderProgram::attribute(const std::string &name, GLint size, GLenum type,
                              GLboolean normalized, GLsizei stride,
                              const void *pointer) {
    auto location = glGetAttribLocation(program, name.c_str());
    attribute(name, location, size, type, normalized, stride, pointer);
}

void ShaderProgram::attribute(const std::string &name, GLint location,
                              GLint size, GLenum type, GLboolean normalized,
                              GLsizei stride, const void *pointer) {
    auto &attr = this->attributes[name];
    attr.name = name;
    attr.size = size;
    attr.type = type;
    attr.normalized = normalized;
    attr.stride = stride;
    attr.pointer = pointer;
    attr.location = location;
}

void ShaderProgram::applyAttribute(const std::string &name) {
    const auto &attr = attributes[name];

    if (attr.location < 0) {
        return;
    }

    glVertexAttribPointer(attr.location, attr.size, attr.type, attr.normalized,
                          attr.stride, attr.pointer);
}

void ShaderProgram::applyAttributes() {
    for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
        const auto &name = iter->first;
        const auto &attr = iter->second;

        if (attr.location < 0) {
            continue;
        }

        glVertexAttribPointer(attr.location, attr.size, attr.type,
                              attr.normalized, attr.stride, attr.pointer);
    }
}

GLint ShaderProgram::uniform(const std::string &name, UniformType type) {
    auto location = glGetUniformLocation(program, name.c_str());
    return uniform(name, location, type);
}

GLint ShaderProgram::uniform(const std::string &name, GLint location,
                             UniformType type) {
    auto &u = uniforms[name];
    u.name = name;
    u.location = location;
    u.type = type;
    return location;
}

void ShaderProgram::setUniformInteger(const std::string &name, int value) {
    auto &u = uniforms[name];
    u.name = name;
    u.value.i = value;
}

void ShaderProgram::setUniformFloat(const std::string &name, float value) {
    auto &u = uniforms[name];
    u.name = name;
    u.value.f = value;
}

void ShaderProgram::setUniformVector2(const std::string &name,
                                      const glm::vec2 &value) {
    auto &u = uniforms[name];
    u.name = name;
    u.value.vec2 = value;
}

void ShaderProgram::setUniformVector3(const std::string &name,
                                      const glm::vec3 &value) {
    auto &u = uniforms[name];
    u.name = name;
    u.value.vec3 = value;
}

void ShaderProgram::setUniformVector4(const std::string &name,
                                      const glm::vec4 &value) {
    auto &u = uniforms[name];
    u.name = name;
    u.value.vec4 = value;
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
                                    const ShaderUniformValue &value) {
    auto &u = uniforms[name];
    u.name = name;
    u.value = value;
}

void ShaderProgram::setUniformValue(const std::string &name, float value) {
    this->setUniformFloat(name, value);
}

void ShaderProgram::setUniformValue(const std::string &name, int value) {
    this->setUniformInteger(name, value);
}

void ShaderProgram::copyAttributesFrom(const ShaderProgram &program) {
    for (auto iter = program.attributes.begin();
         iter != program.attributes.end(); iter++) {
        const auto &name = iter->first;
        const auto &attr = iter->second;

        this->attribute(name, attr.size, attr.type, attr.normalized,
                        attr.stride, attr.pointer);
    }
}

void ShaderProgram::copyUniformsFrom(const ShaderProgram &program) {
    for (auto iter = program.uniforms.begin(); iter != program.uniforms.end();
         iter++) {
        const auto &name = iter->first;
        const auto &u = iter->second;

        setUniformValue(name, u.value);
    }
}

void ShaderProgram::applyUniforms() {
    for (auto iter = uniforms.begin(); iter != uniforms.end(); iter++) {
        const auto &name = iter->first;
        const auto &u = iter->second;

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

GLuint ShaderProgram::compile(const std::string &vsPath,
                              const std::string &fsPath,
                              const std::string &vsSource,
                              const std::string &fsSource, time_t vsMTime,
                              time_t fsMTime) {
    if (program != 0) {
        glDeleteProgram(program);
        program = 0;
    }

    AppLog::getInstance().addLog("(%s, %s): compling started.\n",
                                 vsPath.c_str(), fsPath.c_str());

    auto t0 = glfwGetTime();

    if (!vertexShader.compile(vsPath, GL_VERTEX_SHADER, vsSource,
                              getMTime(vsPath))) {
        AppLog::getInstance().addLog("(%s) Shader compilation failed:\n",
                                     vertexShader.getPath().c_str());
        const auto &errors = vertexShader.getErrors();
        for (auto iter = errors.begin(); errors.end() != iter; iter++) {
            AppLog::getInstance().addLog("%04d: %s\n", iter->first,
                                         iter->second.c_str());
        }
        return 0;
    }

    if (!fragmentShader.compile(fsPath, GL_FRAGMENT_SHADER, fsSource,
                                getMTime(fsPath))) {
        AppLog::getInstance().addLog("(%s) Shader compilation failed:\n",
                                     fragmentShader.getPath().c_str());
        const auto &errors = fragmentShader.getErrors();
        for (auto iter = errors.begin(); errors.end() != iter; iter++) {
            AppLog::getInstance().addLog("%04d: %s\n", iter->first,
                                         iter->second.c_str());
        }
        return 0;
    }

    link();

    if (!checkLinked(program, error)) {
        AppLog::getInstance().addLog("Program linking failed:\n%s",
                                     error.c_str());
        return 0;
    }

    loadAttributes();
    loadUniforms();

    compileTime = glfwGetTime() - t0;
    ok = true;

    AppLog::getInstance().addLog("(%s, %s): Program linking ok (%.2fs)\n",
                                 vertexShader.getPath().c_str(),
                                 fragmentShader.getPath().c_str(), compileTime);

    return program;
}

void ShaderProgram::loadUniforms() {
    GLint count;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &count);
    AppLog::getInstance().addLog("Active Uniforms: %d\n", count);
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
                AppLog::getInstance().addLog(
                    "Uniform #%d Type: Float, Name: %s\n", i, name);
                break;
            case GL_FLOAT_VEC2:
                uniform(name, UniformType::Vector2);
                setUniformValue(name, glm::vec2(0.0f, 0.0f));
                AppLog::getInstance().addLog(
                    "Uniform #%d Type: Vector2, Name: %s\n", i, name);
                break;
            case GL_FLOAT_VEC3:
                uniform(name, UniformType::Vector3);
                setUniformValue(name, glm::vec3(0.0f, 0.0f, 0.0f));
                AppLog::getInstance().addLog(
                    "Uniform #%d Type: Vector3, Name: %s\n", i, name);
                break;
            case GL_FLOAT_VEC4:
                uniform(name, UniformType::Vector4);
                setUniformValue(name, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
                AppLog::getInstance().addLog(
                    "Uniform #%d Type: Vector4, Name: %s\n", i, name);
                break;
            case GL_INT:
                uniform(name, UniformType::Integer);
                setUniformValue(name, 0);
                AppLog::getInstance().addLog(
                    "Uniform #%d Type: Integer, Name: %s\n", i, name);
                break;
            case GL_SAMPLER_2D:
                uniform(name, UniformType::Sampler2D);
                setUniformValue(name, 0);
                AppLog::getInstance().addLog(
                    "Uniform #%d Type: Sampler2D, Name: %s\n", i, name);
                break;
            default:
                AppLog::getInstance().addLog(
                    "[Warning] Unsupported uniform #%d Type: 0x%04X Name: %s\n",
                    i, type, name);
                break;
        }
    }
}

void ShaderProgram::loadAttributes() {
    GLint count;

    glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &count);
    AppLog::getInstance().addLog("Active Attributes: %d\n", count);

    for (int i = 0; i < count; i++) {
        const GLsizei bufSize = 128;
        GLchar name[bufSize];
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

        AppLog::getInstance().addLog("Attribute #%d Type: %04X Name: %s\n", i,
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
