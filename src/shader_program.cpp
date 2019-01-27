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

bool Shader::compile(const char *const path, GLuint type) {
    reset();

    this->path = std::string(path);
    this->type = type;

    char *source = nullptr;
    readText(path, source, &mTime);

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

bool Shader::checkExpired() const
{
    time_t mTime = getMTime(this->path.c_str());
    return mTime != this->mTime;
}

bool Shader::checkExpiredWithReset()
{
    time_t mTime = getMTime(this->path.c_str());
    bool expired = mTime != this->mTime;
    this->mTime = mTime;
    return expired;
}

GLint ShaderProgram::uniform(const char *const name, UniformType type)
{
    GLint location = glGetUniformLocation(program, name);
    ShaderUniform &u = uniforms[name];
    u.name = name;
    u.location = location;
    u.type = type;
    return location;
}

void ShaderProgram::reset() {
    vertexShader.reset();
    fragmentShader.reset();
    uniforms.clear();
    program = 0;
    error = "";
    ok = false;
}

bool ShaderProgram::checkExpired() const
{
    return vertexShader.checkExpired() || fragmentShader.checkExpired();
}

bool ShaderProgram::checkExpiredWithReset()
{
    bool expiredVS = vertexShader.checkExpiredWithReset();
    bool expiredFS = fragmentShader.checkExpiredWithReset();
    return expiredVS || expiredFS;
}

GLuint ShaderProgram::compile(const char *const vsPath,
                              const char *const fsPath) {
    reset();

    if (!vertexShader.compile(vsPath, GL_VERTEX_SHADER)) {
        AppLog::getInstance().addLog("(%s) %s", vertexShader.getPath().c_str(),
                           vertexShader.getError().c_str());
        return 0;
    }

    if (!fragmentShader.compile(fsPath, GL_FRAGMENT_SHADER)) {
        AppLog::getInstance().addLog("(%s) %s", fragmentShader.getPath().c_str(),
                           fragmentShader.getError().c_str());
        return 0;
    }

    program = glCreateProgram();
    glAttachShader(program, vertexShader.getShader());
    glAttachShader(program, fragmentShader.getShader());
    glLinkProgram(program);

    if (!checkLinked(program, &error)) {
        AppLog::getInstance().addLog("%s", error.c_str());
        return 0;
    }

    ok = true;

    return program;
}
