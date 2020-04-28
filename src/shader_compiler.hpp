#pragma once

#include "../glslang/glslang/Include/ShHandle.h"

namespace shader_compiler {
struct CompileResult {
    bool isCompiled = false;
    bool isLinked = false;
    std::string shaderLog = "";
    std::string shaderDebugLog = "";
    std::string programLog = "";
    std::string programDebugLog = "";
    std::string spirvOutputLog = "";
    std::string sourceCode = "";
    std::string readableSpirv = "";
    std::vector<std::string> dependencies;
};

struct ValidateResult {
    bool isCompiled = false;
    bool isLinked = false;
    std::string shaderLog = "";
    std::string shaderDebugLog = "";
    std::string programLog = "";
    std::string programDebugLog = "";
};

void validate(EShLanguage shaderStage, const std::string &sourceFileName,
              const std::string &sourceFileText, ValidateResult &result);

void compile(EShLanguage shadaerStage, int32_t version, bool es,
             const std::string &sourceFileName,
             const std::string &sourceFileText,
             const std::string &templateFileText, CompileResult &result);
}  // namespace shader_compiler
