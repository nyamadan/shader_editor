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

    int32_t uiShaderFileIndex = 0;
    int32_t uiShaderPlatformIndex = 0;
    int32_t uiBufferQualityIndex = 2;
    int32_t uiVideoResolutionIndex = 2;
    int32_t uiVideoTypeIndex = 0;
    int32_t uiVideoQualityIndex = 1;
    float uiVideoMbps = 1.0f;
    float uiVideoTime = 5.0f;

    std::map<std::string, int32_t> uiImageFileNames;
    std::map<int32_t, std::string> programErrors;

    int32_t windowWidth = 1024;
    int32_t windowHeight = 768;

    GLFWwindow* mainWindow = nullptr;

    GLuint vIndex = 0;
    GLuint vPosition = 0;
    GLuint vertexArraysObject = 0;

    std::string shaderToyPreSource;

    std::shared_ptr<ShaderProgram> program;
    std::shared_ptr<ShaderProgram> copyProgram;

    const float CheckInterval = 0.5f;
    float lastCheckUpdate = 0;
    float timeStart = 0;
    uint64_t currentFrame = 0;

    ShaderFiles sf;
    Buffers buffers;
    TextEditor editor;

    std::unique_ptr<Recording> recording = std::make_unique<Recording>();
    bool h264enabled = false;

    void startRecord(const std::string& fileName,
                     const int32_t uiVideoTypeIndex,
                     const int32_t uiVideoResolution, const int32_t kbps,
                     unsigned long encodeDeadline, float& uiTimeValue);

    void App::SetProgramErrors(std::map<int32_t, std::string>& programErrors);

    void App::swapProgram(std::shared_ptr<ShaderProgram> newProgram);
    void App::ShowTextEditor(bool& showTextEditor, int32_t& uiShader,
                             int32_t uiPlatform);

   public:
    GLFWwindow* getMainWindow();

    void update(void*);
    int32_t start();

    void onUiCaptureWindow();
    void onUiStatsWindow();
    void onUiTimeWindow(float now);
    void onUiBackBufferWindow(float& bufferScale, int32_t& currentWidth,
                              int32_t& currentHeight);

    void cleanup();
};
