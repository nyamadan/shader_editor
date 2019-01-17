#include "shader_utils.hpp"
#include "common.hpp"

#include <iostream>

#ifdef __EMSCRIPTEN__
const char *const GlslVersion = "#version 300 es\n";
#else
const char *const GlslVersion = "#version 330 core\n";
#endif

int checkLinked(unsigned int program) {
    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (success == GL_FALSE) {
        int max_len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &max_len);

        char *err_log = static_cast<char *>(malloc(sizeof(char) * max_len));
        glGetProgramInfoLog(program, max_len, &max_len, err_log);

        glDeleteProgram(program);

        std::cerr << "Program linking failed: " << err_log << std::endl;
        free(err_log);
    }

    return success;
}

int checkCompiled(unsigned int shader) {
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_FALSE) {
        int max_len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_len);

        char *err_log = static_cast<char *>(malloc(sizeof(char) * max_len));
        glGetShaderInfoLog(shader, max_len, &max_len, err_log);
        glDeleteShader(shader);

        std::cerr << "Shader compilation failed: " << err_log << std::endl;

        free(err_log);
    }

    return success;
};
