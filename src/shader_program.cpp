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
    path = "(empty)";
    source = "";
    error = "";
    ok = false;

    mTime = 0;
}

bool Shader::compileWithSource(const std::string &path,
                               const std::string &source, GLuint type) {
    reset();

    this->type = type;
    this->path = path;
    this->mTime = getMTime(path);
    this->source = std::string(source);

    shader = glCreateShader(type);
    const char *const pSource = source.c_str();
    glShaderSource(shader, 1, &pSource, NULL);
    glCompileShader(shader);

    if (!checkCompiled(shader, error)) {
        shader = 0;
        return false;
    }

    ok = true;

    return true;
}

bool Shader::compile(const std::string &path, GLuint type) {
    char *source = nullptr;
    readText(path, source, mTime);
    bool result = compileWithSource(path, source, type);
    delete[] source;
    return result;
}

bool Shader::checkExpired() const {
    time_t mTime = getMTime(this->path.c_str());
    return mTime != this->mTime;
}

bool Shader::checkExpiredWithReset() {
    time_t mTime = getMTime(this->path.c_str());
    bool expired = mTime != this->mTime;
    this->mTime = mTime;
    return expired;
}

void ShaderProgram::attribute(const std::string &name, GLint size, GLenum type,
                              GLboolean normalized, GLsizei stride,
                              const void *pointer) {
    ShaderAttribute &attr = this->attributes[name];
    attr.name = name;
    attr.size = size;
    attr.type = type;
    attr.normalized = normalized;
    attr.stride = stride;
    attr.pointer = pointer;
    attr.location = glGetAttribLocation(program, name.c_str());
}

void ShaderProgram::applyAttributes() {
    for (auto iter = attributes.begin(); iter != attributes.end(); iter++) {
        const std::string &name = iter->first;
        const ShaderAttribute &attr = iter->second;

        if (attr.location < 0) {
            continue;
        }

        glVertexAttribPointer(attr.location, attr.size, attr.type,
                              attr.normalized, attr.stride, attr.pointer);
    }
}

GLint ShaderProgram::uniform(const std::string &name, UniformType type) {
    GLint location = glGetUniformLocation(program, name.c_str());
    ShaderUniform &u = uniforms[name];
    u.name = name;
    u.location = location;
    u.type = type;
    return location;
}

void ShaderProgram::setUniformInteger(const std::string &name, int value) {
    ShaderUniform &u = uniforms[name];
    u.value.i = value;
}

void ShaderProgram::setUniformFloat(const std::string &name, float value) {
    ShaderUniform &u = uniforms[name];
    u.value.f = value;
}

void ShaderProgram::setUniformVector2(const std::string &name,
                                      const glm::vec2 &value) {
    ShaderUniform &u = uniforms[name];
    u.value.vec2 = value;
}

void ShaderProgram::setUniformValue(const std::string &name,
                                    const glm::vec2 &value) {
    this->setUniformVector2(name, value);
}

void ShaderProgram::setUniformValue(const std::string &name, float value) {
    this->setUniformFloat(name, value);
}

void ShaderProgram::setUniformValue(const std::string &name, int value) {
    this->setUniformInteger(name, value);
}

void ShaderProgram::applyUniforms() {
    for (auto iter = uniforms.begin(); iter != uniforms.end(); iter++) {
        const std::string &name = iter->first;
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
            case UniformType::Vector2:
                glUniform2fv(u.location, 1, glm::value_ptr(u.value.vec2));
                break;
        }
    }
}

void ShaderProgram::reset() {
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
    bool expiredVS = vertexShader.checkExpiredWithReset();
    bool expiredFS = fragmentShader.checkExpiredWithReset();
    return expiredVS || expiredFS;
}

GLuint ShaderProgram::compile(const std::string &vsPath,
                              const std::string &fsPath) {
    double t0 = ImGui::GetTime();

    reset();

    if (!vertexShader.compile(vsPath, GL_VERTEX_SHADER)) {
        AppLog::getInstance().addLog("(%s) Shader compilation failed:\n%s",
                                     vertexShader.getPath().c_str(),
                                     vertexShader.getError().c_str());
        return 0;
    }

    if (!fragmentShader.compile(fsPath, GL_FRAGMENT_SHADER)) {
        AppLog::getInstance().addLog("(%s) Shader compilation failed:\n%s",
                                     fragmentShader.getPath().c_str(),
                                     fragmentShader.getError().c_str());
        return 0;
    }

    link();

    if (!checkLinked(program, error)) {
        AppLog::getInstance().addLog("Program linking failed:\n%s",
                                     error.c_str());
        return 0;
    }

    compileTime = ImGui::GetTime() - t0;
    ok = true;

    AppLog::getInstance().addLog("(%s, %s): Program linking ok (%.2fs)\n",
                                 vertexShader.getPath().c_str(),
                                 fragmentShader.getPath().c_str(), compileTime);

    return program;
}

GLuint ShaderProgram::compileWithSource(const std::string &vsPath,
                                        const std::string &vsSource,
                                        const std::string &fsPath,
                                        const std::string &fsSource) {
    double t0 = ImGui::GetTime();

    reset();

    if (!vertexShader.compileWithSource(vsPath, vsSource, GL_VERTEX_SHADER)) {
        AppLog::getInstance().addLog("(%s) Shader compilation failed:\n%s",
                                     vertexShader.getPath().c_str(),
                                     vertexShader.getError().c_str());
        return 0;
    }

    if (!fragmentShader.compileWithSource(fsPath, fsSource,
                                          GL_FRAGMENT_SHADER)) {
        AppLog::getInstance().addLog("(%s) Shader compilation failed:\n%s",
                                     fragmentShader.getPath().c_str(),
                                     fragmentShader.getError().c_str());
        return 0;
    }

    link();

    if (!checkLinked(program, error)) {
        AppLog::getInstance().addLog("Program linking failed:\n%s",
                                     error.c_str());
        return 0;
    }

    compileTime = ImGui::GetTime() - t0;
    ok = true;

    AppLog::getInstance().addLog("(%s, %s): Program linking ok (%.2fs)\n",
                                 vertexShader.getPath().c_str(),
                                 fragmentShader.getPath().c_str(), compileTime);

    return program;
}

void ShaderProgram::link() {
    program = glCreateProgram();
    glAttachShader(program, vertexShader.getShader());
    glAttachShader(program, fragmentShader.getShader());
    glLinkProgram(program);
}
