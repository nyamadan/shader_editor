#include "shader_compiler.hpp"

#include <array>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../glslang/SPIRV/GlslangToSpv.h"
#include "../glslang/SPIRV/disassemble.h"
#include "../glslang/glslang/Include/ShHandle.h"

#include "../glslang/StandAlone/ResourceLimits.h"
#include "../glslang/StandAlone/DirStackFileIncluder.h"

#include "../SPIRV-Cross/spirv_glsl.hpp"

namespace {
const char *const TemplateFileName = "<template>";
const int32_t DefaultGlslVersion = 100;

static void setStringIfNotNull(std::string &str, const char *const p) {
    if (p && p[0]) {
        str.assign(p);
    }
}

class Includer : public DirStackFileIncluder {
   protected:
    std::string contentFileName;
    std::string contentFileText;

    std::vector<std::string> dependencies;

   public:
    Includer(const std::string &_contentFileName,
             const std::string &_contentFileText)
        : contentFileName(_contentFileName),
          contentFileText(_contentFileText) {}
    virtual ~Includer() {}

    virtual IncludeResult *includeLocal(const char *headerName,
                                        const char *includerName,
                                        size_t inclusionDepth) override {
        const auto result =
            readLocalPath(headerName, includerName, (int)inclusionDepth);

        if (result != nullptr) {
            dependencies.push_back(result->headerName);
        }

        return result;
    }

    virtual IncludeResult *includeSystem(const char *headerName,
                                         const char *includerName,
                                         size_t /*inclusionDepth*/) override {
        if (std::string(headerName).compare("content") == 0 &&
            std::string(includerName).compare(TemplateFileName) == 0) {
            return new IncludeResult(contentFileName.c_str(),
                                     contentFileText.c_str(),
                                     contentFileText.size(), nullptr);
        }

        return readSystemPath(headerName);
    }

    virtual const std::vector<std::string> &getDependencies() {
        return this->dependencies;
    }
};
}  // namespace

namespace shader_compiler {
void validate(EShLanguage shaderStage, const std::string &sourceFileName,
              const std::string &sourceFileText, ValidateResult &result) {
    std::string entryPoint = "main";
    std::map<std::string, std::string> defines;
    bool isHlsl = false;
    bool enableDebugOutput = false;

    EShMessages messages = EShMsgDefault;
    messages = (EShMessages)(messages | EShMsgCascadingErrors);
    if (enableDebugOutput) {
        messages = (EShMessages)(messages | EShMsgDebugInfo);
    }

    glslang::InitializeProcess();

    std::array<std::array<unsigned int, EShLangCount>, glslang::EResCount>
        baseBinding;
    for (int res = 0; res < glslang::EResCount; ++res) baseBinding[res].fill(0);

    std::array<std::array<std::map<unsigned int, unsigned int>, EShLangCount>,
               glslang::EResCount>
        baseBindingForSet;

    std::array<std::vector<std::string>, EShLangCount> baseResourceSetBinding;

    std::vector<std::string> processes;

    bool isCompiled = false;
    bool isLinked = false;
    std::string shaderLog = "";
    std::string shaderDebugLog = "";
    std::string programLog = "";
    std::string programDebugLog = "";

    auto program = new glslang::TProgram();
    auto shader = new glslang::TShader(shaderStage);
    std::vector<unsigned int> spirv;

    const char *const sources = {sourceFileText.c_str()};
    const char *const names = {sourceFileName.c_str()};

    shader->setStringsWithLengthsAndNames(&sources, NULL, &names, 1);
    shader->setEntryPoint(entryPoint.c_str());

    std::string preamable = "";

    if (!defines.empty()) {
        std::stringstream ssPreamable;

        for (auto it = defines.cbegin(); it != defines.cend(); it++) {
            std::stringstream ssProcess;
            const auto &name = it->first;

            ssPreamable << "#define " << name;
            ssProcess << "define macro " << name;

            const auto &value = it->second;
            if (!value.empty()) {
                ssPreamable << " " << value;
                ssProcess << "=" << value;
            }

            ssPreamable << "\n";

            processes.push_back(ssProcess.str());
        }

        preamable.assign(ssPreamable.str());
    }

    shader->setPreamble(preamable.c_str());
    shader->addProcesses(processes);

    for (int r = 0; r < glslang::EResCount; ++r) {
        const glslang::TResourceType res = glslang::TResourceType(r);
        shader->setShiftBinding(res, baseBinding[res][shaderStage]);
        for (auto i = baseBindingForSet[res][shaderStage].begin();
             i != baseBindingForSet[res][shaderStage].end(); ++i)
            shader->setShiftBindingForSet(res, i->second, i->first);
    }

    shader->setNoStorageFormat(true);
    shader->setResourceSetBinding(baseResourceSetBinding[shaderStage]);
    shader->setUniformLocationBase(0);
    shader->setNanMinMaxClamp(false);
    shader->setFlattenUniformArrays(true);
    shader->setEnvTarget(glslang::EShTargetNone,
                         (glslang::EShTargetLanguageVersion)0);
    if (isHlsl) {
        shader->setEnvInput(glslang::EShSourceHlsl, shaderStage,
                            glslang::EShClientNone, 0);
    } else {
        shader->setEnvInput(glslang::EShSourceGlsl, shaderStage,
                            glslang::EShClientNone, 0);
    }

    isCompiled = shader->parse(&glslang::DefaultTBuiltInResource,
                               DefaultGlslVersion, false, messages);

    setStringIfNotNull(shaderLog, shader->getInfoLog());
    setStringIfNotNull(shaderDebugLog, shader->getInfoDebugLog());

    if (isCompiled) {
        program->addShader(shader);

        isLinked = program->link(messages) && program->mapIO();

        setStringIfNotNull(programLog, program->getInfoLog());
        setStringIfNotNull(programDebugLog, program->getInfoDebugLog());
    }

    delete program;
    program = nullptr;

    delete shader;
    shader = nullptr;

    glslang::FinalizeProcess();

    result.isCompiled = isCompiled;
    result.isLinked = isLinked;
    result.programDebugLog = programDebugLog;
    result.programLog = programLog;
    result.shaderDebugLog = shaderDebugLog;
    result.shaderLog = shaderLog;
}

void compile(EShLanguage shaderStage, int32_t version, bool isGlslEs,
             const std::string &sourceFileName,
             const std::string &sourceFileText,
             const std::string &templateFileText, CompileResult &result) {
    std::string entryPoint = "main";
    std::map<std::string, std::string> defines;
    bool disableSourceCode = false;
    bool enableReadableSpirv = false;
    bool disableOptimizer = false;
    bool optimizeSize = false;
    bool enableDebugOutput = false;
    bool enable16bitTypes = false;
    bool isHlsl = false;
    bool useTemplate = !templateFileText.empty();

    EShMessages messages = EShMsgDefault;
    messages = (EShMessages)(messages | EShMsgSpvRules);
    messages = (EShMessages)(messages | EShMsgCascadingErrors);

    if (isHlsl) {
        messages = (EShMessages)(messages | EShMsgReadHlsl);
        messages = (EShMessages)(messages | EShMsgHlslLegalization);
    }

    if (enableDebugOutput) {
        messages = (EShMessages)(messages | EShMsgDebugInfo);
    }

    if (isHlsl && enable16bitTypes) {
        messages = (EShMessages)(messages | EShMsgHlslEnable16BitTypes);
    }

    glslang::InitializeProcess();

    std::array<std::array<unsigned int, EShLangCount>, glslang::EResCount>
        baseBinding;
    for (int res = 0; res < glslang::EResCount; ++res) baseBinding[res].fill(0);

    std::array<std::array<std::map<unsigned int, unsigned int>, EShLangCount>,
               glslang::EResCount>
        baseBindingForSet;

    std::array<std::vector<std::string>, EShLangCount> baseResourceSetBinding;

    std::vector<std::string> processes;

    bool isCompiled = false;
    bool isLinked = false;
    std::string shaderLog = "";
    std::string shaderDebugLog = "";
    std::string programLog = "";
    std::string programDebugLog = "";
    std::string spirvOutputLog = "";
    std::string sourceCode = "";
    std::string readableSpirv = "";

    auto program = new glslang::TProgram();
    auto shader = new glslang::TShader(shaderStage);
    std::vector<unsigned int> spirv;

    const char *const sources = {useTemplate ? templateFileText.c_str()
                                             : sourceFileText.c_str()};
    const char *const names = {useTemplate ? TemplateFileName
                                           : sourceFileName.c_str()};

    shader->setStringsWithLengthsAndNames(&sources, NULL, &names, 1);
    shader->setEntryPoint(entryPoint.c_str());

    std::string preamable = "";

    if (!defines.empty()) {
        std::stringstream ssPreamable;

        for (auto it = defines.cbegin(); it != defines.cend(); it++) {
            std::stringstream ssProcess;
            const auto &name = it->first;

            ssPreamable << "#define " << name;
            ssProcess << "define macro " << name;

            const auto &value = it->second;
            if (!value.empty()) {
                ssPreamable << " " << value;
                ssProcess << "=" << value;
            }

            ssPreamable << "\n";

            processes.push_back(ssProcess.str());
        }

        preamable.assign(ssPreamable.str());
    }

    shader->setPreamble(preamable.c_str());
    shader->addProcesses(processes);

    for (int r = 0; r < glslang::EResCount; ++r) {
        const glslang::TResourceType res = glslang::TResourceType(r);
        shader->setShiftBinding(res, baseBinding[res][shaderStage]);
        for (auto i = baseBindingForSet[res][shaderStage].begin();
             i != baseBindingForSet[res][shaderStage].end(); ++i)
            shader->setShiftBindingForSet(res, i->second, i->first);
    }

    shader->setNoStorageFormat(true);
    shader->setResourceSetBinding(baseResourceSetBinding[shaderStage]);
    shader->setUniformLocationBase(0);
    shader->setNanMinMaxClamp(false);
    shader->setFlattenUniformArrays(true);
    shader->setEnvClient(glslang::EShClientOpenGL,
                         glslang::EShTargetOpenGL_450);
    shader->setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_0);
    if (isHlsl) {
        shader->setEnvInput(glslang::EShSourceHlsl, shaderStage,
                            glslang::EShClientOpenGL, 450);
    } else {
        shader->setEnvInput(glslang::EShSourceGlsl, shaderStage,
                            glslang::EShClientOpenGL, 450);
    }

    auto includer = Includer(sourceFileName, sourceFileText);
    isCompiled = shader->parse(&glslang::DefaultTBuiltInResource,
                               DefaultGlslVersion, false, messages, includer);
    const auto &dependencies = includer.getDependencies();

    setStringIfNotNull(shaderLog, shader->getInfoLog());
    setStringIfNotNull(shaderDebugLog, shader->getInfoDebugLog());

    if (isCompiled) {
        program->addShader(shader);

        isLinked = program->link(messages) && program->mapIO();

        setStringIfNotNull(programLog, program->getInfoLog());
        setStringIfNotNull(programDebugLog, program->getInfoDebugLog());

        if (isLinked) {
            for (int stage = 0; stage < EShLangCount; ++stage) {
                if (program->getIntermediate((EShLanguage)stage)) {
                    std::string warningsErrors;
                    spv::SpvBuildLogger logger;
                    glslang::SpvOptions spvOptions;
                    spvOptions.generateDebugInfo = enableDebugOutput;
                    spvOptions.disableOptimizer = disableOptimizer;
                    spvOptions.optimizeSize = optimizeSize;
                    spvOptions.disassemble = false;
                    spvOptions.validate = false;
                    glslang::GlslangToSpv(
                        *program->getIntermediate((EShLanguage)stage), spirv,
                        &logger, &spvOptions);
                    spirvOutputLog.assign(logger.getAllMessages());

                    if (enableReadableSpirv) {
                        std::stringstream ss;
                        spv::Disassemble(ss, spirv);
                        readableSpirv = ss.str();
                    }
                }
            }
        }
    }

    delete program;
    program = nullptr;

    delete shader;
    shader = nullptr;

    glslang::FinalizeProcess();

    if (isCompiled && isLinked && !disableSourceCode) {
        spirv_cross::CompilerGLSL glsl(std::move(spirv));
        spirv_cross::ShaderResources resources = glsl.get_shader_resources();

        for (auto &resource : resources.sampled_images) {
            unsigned set =
                glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
            unsigned binding =
                glsl.get_decoration(resource.id, spv::DecorationBinding);

            glsl.unset_decoration(resource.id, spv::DecorationDescriptorSet);
            glsl.set_decoration(resource.id, spv::DecorationBinding,
                                set * 16 + binding);
        }

        spirv_cross::CompilerGLSL::Options options;
        options.version = version;
        options.es = isGlslEs;

        glsl.set_common_options(options);

        sourceCode = glsl.compile();

        result.sourceCode = sourceCode;
    }

    result.isCompiled = isCompiled;
    result.isLinked = isLinked;
    result.programDebugLog = programDebugLog;
    result.programLog = programLog;
    result.readableSpirv = readableSpirv;
    result.shaderDebugLog = shaderDebugLog;
    result.shaderLog = shaderLog;
    result.spirvOutputLog = spirvOutputLog;
    result.dependencies = dependencies;
}
}  // namespace shader_compiler
