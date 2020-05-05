#pragma once

#include "common.hpp"

#include <map>
#include <string>
#include <memory>

#include <TextEditor.h>

#include "shader_files.hpp"
#include "shader_program.hpp"
#include "buffers.hpp"
#include "recording.hpp"

namespace shader_editor {
struct UniformNames {
    const char* mouse = nullptr;
    const char* resolution = nullptr;
    const char* time = nullptr;
    const char* frame = nullptr;
    const char* backbuffer = nullptr;
};

typedef enum {
    GLSL_SANDBOX = 0,
    GLSL_DEFAULT = 1,
    GLSL_CANVAS = 2,
    SHADER_TOY = 3,
} AppShaderPlatform;

const char* const AppShaderPlatformNames[] = {
    "default",
    "glsl-sandbox",
    "glsl-canvas",
    "shadertoy",
};

typedef enum {
    I420 = 0,
    WebM = 1,
    H264 = 2,
} AppVideoType;

const char* const VideoTypeNames[] = {
    "I420",
    "WebM",
    "MP4",
};

class App {
   private:
#ifndef NDEBUG
    bool showImGuiDemoWindow = false;
#endif
    bool uiShaderFileWindow = false;
    bool uiTimeWindow = false;
    bool uiStatsWindow = false;
    bool uiUniformWindow = false;
    bool uiBackBufferWindow = false;
    bool uiCaptureWindow = false;

    bool uiDebugWindow = true;
    bool uiShowTextEditor = false;
    bool uiErrorWindow = false;
    bool uiAppLogWindow = false;

    float uiTimeValue = 0;
    bool uiPlaying = true;

    AppShaderPlatform uiShaderPlatformIndex = GLSL_SANDBOX;
    AppVideoType uiVideoTypeIndex = AppVideoType::I420;
    int32_t uiShaderFileIndex = 0;
    int32_t uiBufferQualityIndex = 2;
    int32_t uiVideoResolutionIndex = 2;
    int32_t uiVideoQualityIndex = 1;
    float uiVideoMbps = 1.0f;
    float uiVideoTime = 5.0f;

    std::map<std::string, int32_t> imageUniformNameToIndex;
    std::vector<CompileError> programErrors;

    int32_t windowWidth;
    int32_t windowHeight;

    GLFWwindow* mainWindow = nullptr;

    GLuint vIndex = 0;
    GLuint vPosition = 0;
    GLuint vertexArraysObject = 0;

    std::shared_ptr<ShaderProgram> program;
    std::shared_ptr<ShaderProgram> copyProgram;

    const float CheckInterval = 0.5f;
    float lastCheckUpdate = 0;
    float timeStart = 0;
    uint64_t currentFrame = 0;

    const float recompileDelay = 0.5f;
    bool needRecompile = false;
    float lastTextEdited = 0;

    ShaderFiles shaderFiles;
    Buffers buffers;
    TextEditor editor;

    std::unique_ptr<Recording> recording = std::make_unique<Recording>();
    bool h264enabled = false;

    void startRecord(const std::string& fileName, const int32_t kbps,
                     unsigned long encodeDeadline);

    void SetProgramErrors(const std::vector<CompileError>& programErrors,
                          std::shared_ptr<ShaderProgram> program);

    void swapProgram(std::shared_ptr<ShaderProgram> newProgram);

    void getUsedTextures(
        const UniformNames& uNames,
        std::map<std::string, std::shared_ptr<Image>>& usedTextures);

    UniformNames getCurrentUniformNames();

    void setupShaderTemplate(std::shared_ptr<ShaderProgram> newProgram);
    void setupPlatformUniform(const UniformNames& uNames,
                              const bool* const mouseDown,
                              const ImVec2& mousePos);

    std::shared_ptr<ShaderProgram> refreshShaderProgram(float now,
                                                        int32_t& cursorLine);

    void onUiCaptureWindow();
    void onTextEditor();
    void onUiStatsWindow();
    void onUiErrorWindow();
    void onUiTimeWindow(float now);
    void onUiBackBufferWindow(float& bufferScale, int32_t& currentWidth,
                              int32_t& currentHeight);
    void onUiUniformWindow(
        const UniformNames& uNames,
        std::map<std::string, std::shared_ptr<Image>>& usedTextures);

    void onUiShaderFileWindow(int32_t& cursorLine);

   public:
    GLFWwindow* getMainWindow();

    int32_t start(int32_t width, int32_t height, const std::string& asetPath,
                  bool alwaysOnTop);
    void update(void*);
    void cleanup();
};
}  // namespace shader_editor