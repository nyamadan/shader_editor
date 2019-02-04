#include "common.hpp"
#include "shader_utils.hpp"
#include "app_log.hpp"

#include <iostream>

#ifdef __EMSCRIPTEN__
const char *const GlslVersion = "#version 300 es\n";
#else
const char *const GlslVersion = "#version 330 core\n";
#endif

GLint checkLinked(GLuint program) {
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    return success;
}

GLint checkLinked(GLuint program, std::string &error) {
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success == GL_FALSE) {
        GLint max_len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_len);

        char *err_log = new char[max_len];
        glGetProgramInfoLog(program, max_len, &max_len, err_log);

        error = err_log;

        delete[] err_log;
    }

    return success;
}

GLint checkCompiled(GLuint shader) {
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    return success;
};

GLint checkCompiled(GLuint shader, std::string &error) {
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE) {
        GLint max_len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_len);

        char *err_log = new char[max_len];

        glGetShaderInfoLog(shader, max_len, &max_len, err_log);

        error = err_log;

        delete[] err_log;
    }

    return success;
};
