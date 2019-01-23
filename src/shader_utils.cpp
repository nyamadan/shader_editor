#include "common.hpp"
#include "shader_utils.hpp"
#include "app_log.hpp"

#include <iostream>

#ifdef __EMSCRIPTEN__
const char *const GlslVersion = "#version 300 es\n";
#else
const char *const GlslVersion = "#version 330 core\n";
#endif

int checkLinked(unsigned int program, std::string *error) {
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success == GL_FALSE && error != nullptr) {
        int max_len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_len);

        char *err_log = new char[max_len];
        glGetProgramInfoLog(program, max_len, &max_len, err_log);

        *error = "Program linking failed:\n";
        *error += err_log;
        *error += "\n";

        delete[] err_log;
    }

    return success;
}

int checkCompiled(unsigned int shader, std::string *error) {
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE && error != nullptr) {
        int max_len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_len);

        char *err_log = new char[max_len];

        glGetShaderInfoLog(shader, max_len, &max_len, err_log);

        *error = "Shader compilation failed : \n";
        *error += err_log;
        *error += "\n";

        delete[] err_log;
    }

    return success;
};
