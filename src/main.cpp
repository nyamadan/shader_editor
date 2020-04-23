#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <memory>

#include <imgui.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>
#include <TextEditor.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "common.hpp"
#include "file_utils.hpp"
#include "platform.hpp"
#include "shader_utils.hpp"
#include "shader_program.hpp"
#include "app_log.hpp"
#include "image.hpp"
#include "recording.hpp"
#include "buffers.hpp"

namespace {

const char* const DefaultAssetPath = "./default";
const char* const DefaultVertexShaderName = "vert.glsl";
const char* const DefaultFragmentShaderName = "frag.glsl";
const char* const CopyFragmentShaderName = "copy.glsl";
const char* const ShaderToyPreSourceName = "pre_shadertoy.glsl";

std::string assetPath;

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

std::vector<std::shared_ptr<Image>> imageFiles;
int32_t numImageFileNames = 0;
char** imageFileNames = nullptr;

std::vector<std::shared_ptr<ShaderProgram>> shaderFiles;
int32_t numShaderFileNames = 0;
char** shaderFileNames = nullptr;

Buffers buffers;

TextEditor editor;

const GLfloat positions[] = {-1.0f, 1.0f,  0.0f, 1.0f, 1.0f,  0.0f,
                             -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f};

const GLushort indices[] = {0, 2, 1, 1, 2, 3};

std::unique_ptr<Recording> recording = std::make_unique<Recording>();
bool h264enabled = false;

void glfwErrorCallback(int error, const char* description) {
    AppLog::getInstance().addLog("error %d: %s\n", error, description);
}

void deleteShaderFileNamse() {
    if (shaderFileNames != nullptr) {
        for (int64_t i = 0; i < numShaderFileNames; i++) {
            delete[] shaderFileNames[i];
        }
        delete[] shaderFileNames;
    }
    shaderFileNames = nullptr;
}

void deleteImageFileNames() {
    if (imageFileNames != nullptr) {
        for (int64_t i = 0; i < numImageFileNames; i++) {
            delete[] imageFileNames[i];
        }
        delete[] imageFileNames;
    }
    imageFileNames = nullptr;
}

void genShaderFileNames() {
    deleteShaderFileNamse();

    numShaderFileNames = static_cast<int32_t>(shaderFiles.size());
    shaderFileNames = new char*[numShaderFileNames];

    for (int64_t i = 0; i < numShaderFileNames; i++) {
        std::string fileStr = shaderFiles[i]->getFragmentShader().getPath();
        const int64_t fileNameLength = fileStr.size();
        char* fileName = new char[fileNameLength + 1];
        fileName[fileNameLength] = '\0';
        fileStr.copy(fileName, fileNameLength, 0);
        shaderFileNames[i] = fileName;
    }
}

void genImageFileNames() {
    deleteImageFileNames();

    numImageFileNames = static_cast<int32_t>(imageFiles.size());
    imageFileNames = new char*[numImageFileNames];
    for (int64_t i = 0; i < numImageFileNames; i++) {
        std::shared_ptr<Image> image = imageFiles[i];
        const std::string& path = image->getPath();
        const int64_t fileNameLength = path.size();
        char* fileName = new char[fileNameLength + 1];
        fileName[fileNameLength] = '\0';
        image->getPath().copy(fileName, fileNameLength, 0);
        imageFileNames[i] = fileName;
    }
}

void compileShaderFromFile(const std::shared_ptr<ShaderProgram> program,
                           const std::string& vsPath,
                           const std::string& fsPath) {
    const int64_t vsTime = getMTime(vsPath);
    const int64_t fsTime = getMTime(fsPath);
    std::string vsSource;
    std::string fsSource;
    readText(vsPath, vsSource);
    readText(fsPath, fsSource);
    program->compile(vsPath, fsPath, vsSource, fsSource, vsTime, fsTime);
}

void recompileFragmentShader(const std::shared_ptr<ShaderProgram> program,
                             std::shared_ptr<ShaderProgram> newProgram,
                             const std::string& fsSource) {
    const std::string& vsPath = program->getVertexShader().getPath();
    const std::string& fsPath = program->getFragmentShader().getPath();
    const int64_t vsTime = getMTime(vsPath);
    const int64_t fsTime = getMTime(fsPath);
    const std::string& vsSource = program->getVertexShader().getSource();
    newProgram->compile(vsPath, fsPath, vsSource, fsSource, vsTime, fsTime);
}

void recompileShaderFromFile(const std::shared_ptr<ShaderProgram> program,
                             std::shared_ptr<ShaderProgram> newProgram) {
    const std::string& vsPath = program->getVertexShader().getPath();
    const std::string& fsPath = program->getFragmentShader().getPath();
    const int64_t vsTime = getMTime(vsPath);
    const int64_t fsTime = getMTime(fsPath);
    std::string vsSource;
    std::string fsSource;
    readText(vsPath, vsSource);
    readText(fsPath, fsSource);
    newProgram->compile(vsPath, fsPath, vsSource, fsSource, vsTime, fsTime);
}

void ShowTextEditor(bool& showTextEditor, int32_t& uiShader,
                    int32_t uiPlatform) {
    auto cpos = editor.GetCursorPosition();
    ImGui::Begin("Text Editor", &showTextEditor, ImGuiWindowFlags_MenuBar);
    ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save")) {
                const std::string& textToSave = editor.GetText();
                const char* const buffer = textToSave.c_str();
                const int64_t size = textToSave.size();

                if (uiShader != 0) {
                    writeText(program->getFragmentShader().getPath(), buffer,
                              static_cast<int32_t>(size));
                } else {
                    std::string path;

                    if (saveFileDialog(path, "Shader file (*.glsl)\0*.glsl\0",
                                       "glsl")) {
                        std::shared_ptr<ShaderProgram> newProgram =
                            std::make_shared<ShaderProgram>();

                        if (uiPlatform == 1) {
                            newProgram->setFragmentShaderPreSource(
                                shaderToyPreSource);
                        }

                        writeText(path, buffer, static_cast<int32_t>(size));

                        const std::string& vsPath =
                            program->getVertexShader().getPath();
                        const std::string& fsPath = path;
                        const int64_t vsTime = getMTime(vsPath);
                        const int64_t fsTime = getMTime(fsPath);
                        const std::string& vsSource =
                            program->getVertexShader().getSource();
                        const std::string& fsSource = textToSave;

                        newProgram->compile(vsPath, fsPath, vsSource, fsSource,
                                            vsTime, fsTime);

                        shaderFiles.push_back(newProgram);
                        genShaderFileNames();

                        uiShader = numShaderFileNames - 1;

                        program.swap(newProgram);
                    }
                }
            }

            if (ImGui::MenuItem("Close")) {
                showTextEditor = false;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            bool ro = editor.IsReadOnly();
            if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
                editor.SetReadOnly(ro);
            ImGui::Separator();

            if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr,
                                !ro && editor.CanUndo()))
                editor.Undo();
            if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr,
                                !ro && editor.CanRedo()))
                editor.Redo();

            ImGui::Separator();

            if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr,
                                editor.HasSelection()))
                editor.Copy();
            if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr,
                                !ro && editor.HasSelection()))
                editor.Cut();
            if (ImGui::MenuItem("Delete", "Del", nullptr,
                                !ro && editor.HasSelection()))
                editor.Delete();
            if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr,
                                !ro && ImGui::GetClipboardText() != nullptr)) {
                editor.Paste();
                editor.SetText(editor.GetText());
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Select all", nullptr, nullptr))
                editor.SetSelection(
                    TextEditor::Coordinates(),
                    TextEditor::Coordinates(editor.GetTotalLines(), 0));

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            if (ImGui::MenuItem("Dark palette"))
                editor.SetPalette(TextEditor::GetDarkPalette());
            if (ImGui::MenuItem("Light palette"))
                editor.SetPalette(TextEditor::GetLightPalette());
            if (ImGui::MenuItem("Retro blue palette"))
                editor.SetPalette(TextEditor::GetRetroBluePalette());
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1,
                cpos.mColumn + 1, editor.GetTotalLines(),
                editor.IsOverwrite() ? "Ovr" : "Ins",
                editor.CanUndo() ? "*" : " ",
                editor.GetLanguageDefinition().mName.c_str(),
                program->getFragmentShader().getPath().c_str());

    editor.Render("TextEditor");
    ImGui::End();
}

void SetProgramErrors(std::map<int32_t, std::string>& programErrors) {
    editor.SetErrorMarkers(programErrors);
}

void swapProgram(std::shared_ptr<ShaderProgram> newProgram) {
    newProgram->copyAttributesFrom(*program);
    newProgram->copyUniformsFrom(*program);

    glBindVertexArray(vertexArraysObject);
    glBindBuffer(GL_ARRAY_BUFFER, vPosition);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vIndex);
    newProgram->applyAttributes();
    glBindVertexArray(0);

    glUseProgram(newProgram->getProgram());

    program.swap(newProgram);
}

void startRecord(const std::string& fileName, const int32_t uiVideoTypeIndex,
                 const int32_t uiVideoResolution, const int32_t kbps,
                 unsigned long encodeDeadline,
                 float& uiTimeValue) {
    currentFrame = 0;
    uiTimeValue = 0;

    switch (uiVideoResolution) {
        case 0:
            buffers.updateFrameBuffersSize(256, 144);
            break;
        case 1:
            buffers.updateFrameBuffersSize(427, 240);
            break;
        case 2:
            buffers.updateFrameBuffersSize(640, 360);
            break;
        case 3:
            buffers.updateFrameBuffersSize(720, 480);
            break;
        case 4:
            buffers.updateFrameBuffersSize(1280, 720);
            break;
        case 5:
            buffers.updateFrameBuffersSize(1920, 1080);
            break;
        case 6:
            buffers.updateFrameBuffersSize(2560, 1440);
            break;
        case 7:
            buffers.updateFrameBuffersSize(3840, 2160);
            break;
    }

    recording->start(buffers.getWidth(), buffers.getHeight(), fileName,
                     uiVideoTypeIndex, kbps, encodeDeadline);
}

void update(void*) {
#ifndef NDEBUG
    static bool showImGuiDemoWindow = false;
#endif

    static bool uiShaderFileWindow = false;
    static bool uiTimeWindow = false;
    static bool uiStatsWindow = false;
    static bool uiUniformWindow = false;
    static bool uiBackBufferWindow = false;
    static bool uiCaptureWindow = false;

    static bool uiDebugWindow = true;
    static bool uiShowTextEditor = false;
    static bool uiErrorWindow = false;
    static bool uiAppLogWindow = false;

    static float uiTimeValue = 0;
    static bool uiPlaying = true;

    static int32_t uiShaderFileIndex = 0;
    static int32_t uiShaderPlatformIndex = 0;
    static int32_t uiBufferQualityIndex = 2;
    static int32_t uiVideoResolutionIndex = 2;
    static int32_t uiVideoTypeIndex = 0;
    static int32_t uiVideoQualityIndex = 1;
    static float uiVideoMbps = 1.0f;
    static float uiVideoTime = 5.0f;
    static std::map<std::string, int32_t> uiImageFileNames;
    static std::map<int32_t, std::string> programErrors;

    std::map<std::string, std::shared_ptr<Image>> usedTextures;
    int currentWidth, currentHeight;
    float bufferScale =
        1.0f / powf(2.0f, static_cast<float>(uiBufferQualityIndex - 1));

    int32_t cursorLine = -1;

    float now = static_cast<float>(ImGui::GetTime());
    unsigned long encodeDeadline = VPX_DL_GOOD_QUALITY;

    const char* uMouseName = nullptr;
    const char* uResolutionName = nullptr;
    const char* uTimeName = nullptr;
    const char* uFrameName = nullptr;
    const char* uBackbuffer = nullptr;

    switch (uiShaderPlatformIndex) {
        case 0:
            uTimeName = "time";
            uResolutionName = "resolution";
            uMouseName = "mouse";
            uBackbuffer = "backbuffer";
            uFrameName = "";
            break;
        case 1:
            uTimeName = "iTime";
            uResolutionName = "iResolution";
            uMouseName = "iMouse";
            uFrameName = "iFrame";
            uBackbuffer = "";
            break;
    }

    glfwMakeContextCurrent(mainWindow);
    glfwGetFramebufferSize(mainWindow, &currentWidth, &currentHeight);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    std::shared_ptr<ShaderProgram> newProgram =
        std::make_shared<ShaderProgram>();

    if (now - lastCheckUpdate > CheckInterval) {
        if (program->checkExpiredWithReset()) {
            if (uiDebugWindow && uiShowTextEditor) {
                std::string text;
                readText(program->getFragmentShader().getPath(), text);
                cursorLine = editor.GetCursorPosition().mLine;
                editor.SetText(text);
                editor.SetCursorPosition(TextEditor::Coordinates());
            } else {
                if (uiShaderPlatformIndex == 1) {
                    newProgram->setFragmentShaderPreSource(shaderToyPreSource);
                } else {
                    newProgram->setFragmentShaderPreSource(std::string());
                }
                recompileShaderFromFile(program, newProgram);
                programErrors = newProgram->getFragmentShader().getErrors();
                if (programErrors.begin() != programErrors.end()) {
                    cursorLine = programErrors.begin()->first - 1;
                }
                shaderFiles[uiShaderFileIndex] = newProgram;

                SetProgramErrors(programErrors);
            }
        }

        lastCheckUpdate = now;
    }

    if (uiShowTextEditor && editor.IsTextChanged()) {
        if (uiShaderPlatformIndex == 1) {
            newProgram->setFragmentShaderPreSource(shaderToyPreSource);
        } else {
            newProgram->setFragmentShaderPreSource(std::string());
        }

        recompileFragmentShader(program, newProgram, editor.GetText());
        programErrors = newProgram->getFragmentShader().getErrors();
        if (editor.IsReadOnly() &&
            programErrors.begin() != programErrors.end()) {
            cursorLine = programErrors.begin()->first - 1;
        }
        shaderFiles[uiShaderFileIndex] = newProgram;

        SetProgramErrors(programErrors);
    }

    if (newProgram->isOK()) {
        swapProgram(newProgram);
    }

    const bool* const mouseDown = ImGui::GetIO().MouseDown;
    const ImVec2& mousePos =
        ImGui::IsMousePosValid() ? ImGui::GetMousePos() : ImVec2(0.5f, 0.5f);

    // onResizeWindow
    if (!recording->getIsRecording() &&
        (currentWidth != windowWidth || currentHeight != windowHeight)) {
        buffers.updateFrameBuffersSize(static_cast<GLint>(currentWidth * bufferScale),
                               static_cast<GLint>(currentHeight * bufferScale));
    }

    windowWidth = currentWidth;
    windowHeight = currentHeight;

    if (recording->getIsRecording()) {
        uiTimeValue = static_cast<float>(static_cast<double>(currentFrame) *
                                         (1001.0 / 30000.0));
    } else {
        if (!uiPlaying) {
            timeStart = now - uiTimeValue;
            uiTimeValue = uiTimeValue;
        } else {
            uiTimeValue = now - timeStart;
        }
    }

    usedTextures.clear();

    if (numImageFileNames > 0) {
        auto& uniforms = program->getUniforms();
        for (auto it = uniforms.begin(); it != uniforms.end(); it++) {
            ShaderUniform& u = it->second;

            if (u.type == UniformType::Sampler2D && u.name != uBackbuffer) {
                int32_t& uiImage = uiImageFileNames[u.name];

                if (uiImage < 0 || uiImage >= numImageFileNames) {
                    uiImage = 0;
                }

                usedTextures[u.name] = imageFiles[uiImage];
            }
        }
    }

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
        uiDebugWindow = !uiDebugWindow;
    }

    if (uiDebugWindow) {
        ImGui::Begin("Control", &uiDebugWindow,
                     ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Checkbox("File", &uiShaderFileWindow);
        ImGui::Checkbox("Time", &uiTimeWindow);
        ImGui::Checkbox("Stats", &uiStatsWindow);
        ImGui::Checkbox("Uniforms", &uiUniformWindow);
        ImGui::Checkbox("BackBuffer", &uiBackBufferWindow);
        ImGui::Checkbox("Export Video", &uiCaptureWindow);
        ImGui::Checkbox("Errors", &uiErrorWindow);
        ImGui::Checkbox("Log", &uiAppLogWindow);

#ifndef NDEBUG
        ImGui::Checkbox("ImGui Demo Window", &showImGuiDemoWindow);
#endif

        ImGui::End();

        if (uiShaderFileWindow) {
            ImGui::Begin("File", &uiShaderFileWindow,
                         ImGuiWindowFlags_AlwaysAutoResize);

            const char* const items[] = {"glslsandbox", "shadertoy"};
            if (ImGui::Combo("platform", &uiShaderPlatformIndex, items,
                             IM_ARRAYSIZE(items))) {
                std::shared_ptr<ShaderProgram> newProgram =
                    std::make_shared<ShaderProgram>();

                if (uiShaderPlatformIndex == 1) {
                    newProgram->setFragmentShaderPreSource(shaderToyPreSource);
                } else {
                    newProgram->setFragmentShaderPreSource(std::string());
                }

                recompileFragmentShader(program, newProgram, editor.GetText());
                programErrors = newProgram->getFragmentShader().getErrors();
                if (programErrors.begin() != programErrors.end()) {
                    cursorLine = programErrors.begin()->first - 1;
                }
                shaderFiles[uiShaderFileIndex] = newProgram;

                SetProgramErrors(programErrors);

                swapProgram(newProgram);
            }

            if (ImGui::Combo("File", &uiShaderFileIndex, shaderFileNames,
                             numShaderFileNames)) {
                std::shared_ptr<ShaderProgram> newProgram =
                    shaderFiles[uiShaderFileIndex];

                if (uiShaderPlatformIndex == 1) {
                    newProgram->setFragmentShaderPreSource(shaderToyPreSource);
                } else {
                    newProgram->setFragmentShaderPreSource(std::string());
                }

                newProgram->compile();

                editor.SetText(newProgram->getFragmentShader().getSource());
                editor.SetCursorPosition(TextEditor::Coordinates());

                programErrors = newProgram->getFragmentShader().getErrors();
                if (programErrors.begin() != programErrors.end()) {
                    cursorLine = programErrors.begin()->first - 1;
                }

                shaderFiles[uiShaderFileIndex] = newProgram;

                SetProgramErrors(programErrors);

                swapProgram(newProgram);
            }

            ImGui::Checkbox("TextEditor", &uiShowTextEditor);

            if (uiShowTextEditor) {
                ShowTextEditor(uiShowTextEditor, uiShaderFileIndex,
                               uiShaderPlatformIndex);
                if (cursorLine >= 0) {
                    editor.SetCursorPosition(
                        TextEditor::Coordinates(cursorLine, 0));
                    cursorLine = -1;
                }
            }

#if defined(_MSC_VER) || defined(__MINGW32__)
            if (ImGui::Button("Open")) {
                std::string path;

                if (openFileDialog(path, "Shader file (*.glsl)\0*.glsl\0")) {
                    std::shared_ptr<ShaderProgram> newProgram =
                        std::make_shared<ShaderProgram>();

                    if (uiShaderPlatformIndex == 1) {
                        newProgram->setFragmentShaderPreSource(
                            shaderToyPreSource);
                    }

                    compileShaderFromFile(
                        newProgram, program->getVertexShader().getPath(), path);
                    editor.SetText(newProgram->getFragmentShader().getSource());
                    editor.SetCursorPosition(TextEditor::Coordinates());

                    programErrors = newProgram->getFragmentShader().getErrors();
                    if (programErrors.begin() != programErrors.end()) {
                        cursorLine = programErrors.begin()->first - 1;
                    }

                    SetProgramErrors(programErrors);

                    shaderFiles.push_back(newProgram);
                    uiShaderFileIndex = shaderFiles.size() - 1;
                    genShaderFileNames();

                    uiShaderFileIndex = numShaderFileNames - 1;

                    program.swap(newProgram);
                }
            }
#endif

            ImGui::End();
        }

        if (uiTimeWindow) {
            ImGui::Begin("Time", &uiTimeWindow,
                         ImGuiWindowFlags_AlwaysAutoResize);
            if (uiPlaying) {
                ImGui::LabelText("time", "%s",
                                 std::to_string(uiTimeValue).c_str());
            } else {
                ImGui::DragFloat("time", &uiTimeValue, 0.5f);
            }

            ImGui::Checkbox("Playing", &uiPlaying);

            if (ImGui::Button("Reset")) {
                timeStart = now;
                uiTimeValue = 0;
                currentFrame = 0;
            }
            ImGui::End();
        }

        if (uiCaptureWindow) {
            ImGui::Begin("Export Video", &uiCaptureWindow,
                         ImGuiWindowFlags_AlwaysAutoResize);
            const char* resolutionItems[] = {
                "256x144",  "427x240",   "640x360",   "720x480",
                "1280x720", "1920x1080", "2560x1440", "3840x2160"};

            const char* typeItems[] = {"I420", "WebM", "MP4"};

            ImGui::Combo("resolution", &uiVideoResolutionIndex, resolutionItems,
                         IM_ARRAYSIZE(resolutionItems));

            ImGui::Combo(
                "format", &uiVideoTypeIndex, typeItems,
                IM_ARRAYSIZE(typeItems) - (h264enabled ? 0 : 1));

            if (uiVideoTypeIndex == 1) {
                const char* qualityItems[] = {"fast", "good", "best"};

                ImGui::Combo("quality", &uiVideoQualityIndex, qualityItems,
                             IM_ARRAYSIZE(qualityItems));

                switch (uiVideoQualityIndex) {
                    case 0:
                        encodeDeadline = VPX_DL_REALTIME;
                        break;
                    case 1:
                        encodeDeadline = VPX_DL_GOOD_QUALITY;
                        break;
                    case 2:
                        encodeDeadline = VPX_DL_BEST_QUALITY;
                        break;
                }

                ImGui::DragFloat("Mbps", &uiVideoMbps, 0.1f, 0.5f, 15.0f);
            }

            ImGui::DragFloat("seconds", &uiVideoTime, 0.5f, 0.5f, 600.0f, "%f");

            std::stringstream ss;
            ss.str(std::string());
            ss << 30000.0f / 1001.0f << " hz";
            ImGui::LabelText("framerate", "%s", ss.str().c_str());

            std::string fileName;
            const char* const y4mFilter = "Video file (*.y4m)\0*.y4m\0";
            const char* const y4mExt = "y4m";
            const char* const webmFilter = "Video file (*.webm)\0*.webm\0";
            const char* const webmExt = "webm";
            const char* const mp4Filter = "Video file (*.mp4)\0*.mp4\0";
            const char* const mp4Ext = "mp4";
            const char* filter = nullptr;
            const char* ext = nullptr;
            bool ok = false;

            if (ImGui::Button("Export")) {
                switch (uiVideoTypeIndex) {
                    case 0: {
                        fileName = "video.y4m";
                        filter = y4mFilter;
                        ext = y4mExt;
                    } break;
                    case 1: {
                        fileName = "video.webm";
                        filter = webmFilter;
                        ext = webmExt;
                    } break;
                    case 2: {
                        fileName = "video.mp4";
                        filter = mp4Filter;
                        ext = mp4Ext;
                    } break;
                }

#if defined(_MSC_VER) || defined(__MINGW32__)
                ok = saveFileDialog(fileName, filter, ext);
#else
                ok = true;
#endif
                if (ok) {
                    startRecord(fileName, uiVideoTypeIndex,
                                uiVideoResolutionIndex,
                                static_cast<int32_t>(uiVideoMbps * 1000.0f),
                                encodeDeadline,
                                uiTimeValue);
                    ImGui::OpenPopup("Export Progress");
                }
            }

            if (ImGui::BeginPopupModal("Export Progress", NULL,
                                       ImGuiWindowFlags_AlwaysAutoResize)) {
                if (!recording->getIsRecording()) {
                    ImGui::ProgressBar(1.0f, ImVec2(200.0f, 15.0f));

                    ImGui::Separator();

                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                } else {
                    float value = uiTimeValue / uiVideoTime;
                    ImGui::ProgressBar(value, ImVec2(200.0f, 15.0f));
                }

                ImGui::EndPopup();
            }
            ImGui::End();
        }

        if (uiStatsWindow) {
            ImGui::Begin("Stats", &uiStatsWindow,
                         ImGuiWindowFlags_AlwaysAutoResize);

            std::stringstream ss;

            ss.str(std::string());
            ss << 1000.0f / ImGui::GetIO().Framerate;
            ImGui::LabelText("ms/frame", "%s", ss.str().c_str());

            ss.str(std::string());
            ss << ImGui::GetIO().Framerate;
            ImGui::LabelText("fps", "%s", ss.str().c_str());
            ImGui::End();
        }

        if (uiUniformWindow) {
            ImGui::Begin("Uniforms", &uiUniformWindow,
                         ImGuiWindowFlags_AlwaysAutoResize);
            std::map<const std::string, ShaderUniform>& uniforms =
                program->getUniforms();
            for (auto it = uniforms.begin(); it != uniforms.end(); it++) {
                const std::string& name = it->first;
                ShaderUniform& u = it->second;
                if (u.location < 0) {
                    continue;
                }

                if (u.name == uTimeName || u.name == uResolutionName ||
                    u.name == uMouseName || u.name == uFrameName ||
                    u.name == uBackbuffer) {
                    ImGui::LabelText(u.name.c_str(), "%s",
                                     u.toString().c_str());
                    continue;
                }

                switch (u.type) {
                    case UniformType::Float:
                        ImGui::DragFloat(u.name.c_str(), &u.value.f);
                        break;
                    case UniformType::Integer:
                        ImGui::DragInt(u.name.c_str(), &u.value.i);
                        break;
                    case UniformType::Sampler2D:
                        if (numImageFileNames > 0) {
                            int32_t& uiImage = uiImageFileNames[u.name];

                            if (ImGui::Combo(u.name.c_str(), &uiImage,
                                             imageFileNames,
                                             numImageFileNames)) {
                                usedTextures[u.name] = imageFiles[uiImage];
                            }
                        }
                        break;
                    case UniformType::Vector1:
                        break;
                    case UniformType::Vector2:
                        ImGui::DragFloat2(u.name.c_str(),
                                          glm::value_ptr(u.value.vec2));
                        break;
                    case UniformType::Vector3:
                        ImGui::DragFloat3(u.name.c_str(),
                                          glm::value_ptr(u.value.vec3));
                        break;
                    case UniformType::Vector4:
                        ImGui::DragFloat4(u.name.c_str(),
                                          glm::value_ptr(u.value.vec4));
                        break;
                }
            }
            ImGui::End();
        }

        if (uiBackBufferWindow) {
            ImGui::Begin("BackBuffer", &uiBackBufferWindow,
                         ImGuiWindowFlags_AlwaysAutoResize);
            const char* const items[] = {"0.5", "1", "2", "4", "8"};
            if (ImGui::Combo("quality", &uiBufferQualityIndex, items,
                             IM_ARRAYSIZE(items))) {
                // update framebuffers
                bufferScale =
                    1.0f /
                    powf(2.0f, static_cast<float>(uiBufferQualityIndex - 1));
                buffers.updateFrameBuffersSize(
                    static_cast<GLint>(currentWidth * bufferScale),
                    static_cast<GLint>(currentHeight * bufferScale));
            }

            std::stringstream windowStringStream;
            windowStringStream << windowWidth << ", " << windowHeight;
            ImGui::LabelText("window", "%s", windowStringStream.str().c_str());

            std::stringstream bufferStringStream;
            bufferStringStream << buffers.getWidth() << ", " << buffers.getHeight();
            ImGui::LabelText("buffer", "%s", bufferStringStream.str().c_str());
            ImGui::End();
        }

        if (uiErrorWindow) {
            ImGui::Begin("Errors", &uiErrorWindow);
            ImGui::SetWindowSize(ImVec2(800, 80), ImGuiCond_FirstUseEver);

            for (auto iter = programErrors.begin(); iter != programErrors.end();
                 iter++) {
                const auto& line = iter->first;
                const auto& message = iter->second;
                std::stringstream ss;
                ss << std::setw(4) << std::setfill('0') << line << ": "
                   << message;
                bool selected = false;
                if (ImGui::Selectable(ss.str().c_str(), selected)) {
                    TextEditor::Coordinates cursor;
                    cursor.mLine = line - 1;
                    editor.SetCursorPosition(cursor);
                }
            }

            ImGui::End();
        }

        if (uiAppLogWindow) {
            AppLog::getInstance().showAppLogWindow(uiAppLogWindow);
        }

#ifndef NDEBUG
        if (showImGuiDemoWindow) {
            ImGui::ShowDemoWindow(&showImGuiDemoWindow);
        }
#endif
    }

    ImGui::Render();

    glBindFramebuffer(GL_FRAMEBUFFER, buffers.getFrameBuffer(WRITE));
    glViewport(0, 0, buffers.getWidth(), buffers.getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // uniform values
    program->setUniformValue(uResolutionName,
                             glm::vec2(static_cast<GLfloat>(buffers.getWidth()),
                                       static_cast<GLfloat>(buffers.getHeight())));

    switch (uiShaderPlatformIndex) {
        case 0:
            if (recording->getIsRecording()) {
                program->setUniformValue(uMouseName, glm::vec2(0.5f, 0.5f));
            } else {
                program->setUniformValue(
                    uMouseName, glm::vec2(mousePos.x / windowWidth,
                                          1.0f - mousePos.y / windowHeight));
            }
            break;
        case 1:
            program->setUniformValue(uFrameName,
                                     static_cast<int>(currentFrame));

            if (recording->getIsRecording()) {
                program->setUniformValue(
                    uMouseName, glm::vec4(0.5f * windowWidth,
                                          0.5f * windowHeight, 0.0f, 0.0f));
            } else {
                if (mouseDown[0] || mouseDown[1]) {
                    program->setUniformValue(
                        uMouseName,
                        glm::vec4(mousePos.x, windowHeight - mousePos.y,
                                  mouseDown[0] ? 1.0f : 0.0f,
                                  mouseDown[1] ? 1.0f : 0.0f));
                } else {
                    if (program->containsUniform(uMouseName)) {
                        const glm::vec4& prevMousePos =
                            program->getUniform(uMouseName).value.vec4;
                        program->setUniformValue(
                            uMouseName,
                            glm::vec4(prevMousePos.x, prevMousePos.y, 0, 0));
                    }
                }
            }
            break;
    }

    program->setUniformValue(uTimeName, uiTimeValue);

    if (program->isOK()) {
        glUseProgram(program->getProgram());

        int32_t channel = 0;

        for (auto iter = usedTextures.begin(); iter != usedTextures.end();
             iter++) {
            const std::string& name = iter->first;
            std::shared_ptr<Image> image = iter->second;

            if (!image->isLoaded()) {
                image->load();
            }

            glActiveTexture(GL_TEXTURE0 + channel);
            glBindTexture(GL_TEXTURE_2D, image->getTexture());
            program->setUniformValue(iter->first, channel++);
        }

        glActiveTexture(GL_TEXTURE0 + channel);
        glBindTexture(GL_TEXTURE_2D, buffers.getBackBuffer(READ));
        program->setUniformValue(uBackbuffer, channel++);
        program->applyUniforms();

        glBindVertexArray(vertexArraysObject);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
        glBindVertexArray(0);
    }

    if (recording->getIsRecording()) {
        bool isLastFrame = uiTimeValue >= uiVideoTime;

        recording->update(isLastFrame, currentFrame,
                          buffers.getPixelBuffer(WRITE),
                          buffers.getPixelBuffer(READ));

        if(isLastFrame) {
            bufferScale =
                1.0f / powf(2.0f, static_cast<float>(uiBufferQualityIndex - 1));
            buffers.updateFrameBuffersSize(
                static_cast<GLint>(currentWidth * bufferScale),
                static_cast<GLint>(currentHeight * bufferScale));
        }

        currentFrame++;
    } else {
        if (uiPlaying) {
            currentFrame++;
        }
    }

    // swap buffer
    buffers.swap();

    // copy to frontbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(copyProgram->getProgram());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, buffers.getBackBuffer(READ));

    copyProgram->setUniformValue("backbuffer", 0);
    copyProgram->setUniformValue("resolution",
                                 glm::vec2(windowWidth, windowHeight));
    copyProgram->applyUniforms();

    glBindVertexArray(vertexArraysObject);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwMakeContextCurrent(mainWindow);

    glfwSwapBuffers(mainWindow);

    for (GLenum error = glGetError(); error; error = glGetError()) {
        AppLog::getInstance().addLog("error code: 0x%0X\n", error);
    }

    glfwPollEvents();
}

void loadFiles(const std::string& path) {
    AppLog::getInstance().addLog("Assets:\n");

    std::vector<std::string> files = openDir(path);
    for (auto iter = files.begin(); iter != files.end(); iter++) {
        const std::string& file = *iter;
        std::string ext = getExtention(*iter);
        AppLog::getInstance().addLog("%s(%s)\n", iter->c_str(), ext.c_str());

        if (ext == ".jpg" || ext == ".jpeg" || ext == ".png") {
            std::shared_ptr<Image> image = std::make_shared<Image>();
            image->setPath(path, *iter, ext);
            imageFiles.push_back(image);
        }

        if (ext == ".glsl" || ext == ".frag") {
            std::shared_ptr<ShaderProgram> newProgram =
                std::make_shared<ShaderProgram>();

            std::string vertexShaderPath = DefaultAssetPath;
            appendPath(vertexShaderPath, DefaultVertexShaderName);

            std::string fragmentShaderPath = path;
            appendPath(fragmentShaderPath, file);

            const int64_t vsTime = getMTime(vertexShaderPath);
            const int64_t fsTime = getMTime(fragmentShaderPath);
            std::string vsSource;
            std::string fsSource;
            readText(vertexShaderPath, vsSource);
            readText(fragmentShaderPath, fsSource);
            newProgram->setCompileInfo(vertexShaderPath, fragmentShaderPath,
                                       vsSource, fsSource, vsTime, fsTime);
            shaderFiles.push_back(newProgram);
        }
    }
}

#ifndef __EMSCRIPTEN__

void glDebugOutput(GLenum source, GLenum type, GLuint eid, GLenum severity,
                   GLsizei length, const GLchar* message,
                   const void* user_param) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            break;
        case GL_DEBUG_SEVERITY_LOW:
        case GL_DEBUG_SEVERITY_MEDIUM:
        case GL_DEBUG_SEVERITY_HIGH:
            AppLog::getInstance().addLog("ERROR(%X): %s\n", eid, message);
            break;
    }
}

void EnableOpenGLDebugExtention() {
    GLint flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback((GLDEBUGPROC)glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
                              nullptr, GL_TRUE);
    }
}
#endif
}  // namespace

int main(void) {
#if defined(_MSC_VER) || defined(__MINGW32__)
    h264enabled = h264encoder::LoadEncoderLibrary();

#endif

    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) return -1;

#if defined(__EMSCRIPTEN__)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#elif defined(__APPLE__)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
#endif

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    mainWindow = glfwCreateWindow(windowWidth, windowHeight, "Shader Editor",
                                  NULL, NULL);
    if (!mainWindow) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(mainWindow);

#ifndef __EMSCRIPTEN__
    // Initialize OpenGL loader
    if (gl3wInit() != 0) {
        return 1;
    }

#ifndef NDEBUG
    EnableOpenGLDebugExtention();
#endif

#endif

    ImGui::SetCurrentContext(ImGui::CreateContext());
    ImGui_ImplGlfw_InitForOpenGL(mainWindow, true);
    ImGui_ImplOpenGL3_Init(GlslVersion);

    ImGui::StyleColorsClassic();

    glfwMakeContextCurrent(mainWindow);

    std::string vertexShaderPath = DefaultAssetPath;
    std::string fragmentShaderPath = DefaultAssetPath;
    std::string shaderToyPreSourcePath = DefaultAssetPath;
    std::string copyFS = DefaultAssetPath;

    appendPath(vertexShaderPath, DefaultVertexShaderName);
    appendPath(fragmentShaderPath, DefaultFragmentShaderName);
    appendPath(copyFS, CopyFragmentShaderName);
    appendPath(shaderToyPreSourcePath, ShaderToyPreSourceName);

    // load pre source
    readText(shaderToyPreSourcePath, shaderToyPreSource);

    // Compile shaders.
    program.reset(new ShaderProgram());
    compileShaderFromFile(program, vertexShaderPath, fragmentShaderPath);
    assert(program->isOK());

    copyProgram.reset(new ShaderProgram());
    compileShaderFromFile(copyProgram, vertexShaderPath, copyFS);
    assert(copyProgram->isOK());

    glGenBuffers(1, &vIndex);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vIndex);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * 6, indices,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glGenBuffers(1, &vPosition);
    glBindBuffer(GL_ARRAY_BUFFER, vPosition);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 12, positions,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Initialize Buffers
    glGenVertexArrays(1, &vertexArraysObject);
    glBindVertexArray(vertexArraysObject);
    glBindBuffer(GL_ARRAY_BUFFER, vPosition);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vIndex);
    program->applyAttributes();
    glBindVertexArray(0);

    // Framebuffers
    buffers.initialize(windowWidth / 2, windowHeight / 2);

    // Load files.

    loadFiles("./assets");

    shaderFiles.insert(shaderFiles.begin(), program);

    genImageFileNames();

    genShaderFileNames();

    timeStart = static_cast<float>(ImGui::GetTime());

    auto lang = TextEditor::LanguageDefinition::GLSL();
    editor.SetLanguageDefinition(lang);
    editor.SetText(program->getFragmentShader().getSource());

#ifndef __EMSCRIPTEN__
    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(mainWindow)) {
        update(nullptr);
    }

    deleteImageFileNames();
    deleteShaderFileNamse();

    recording->cleanup();

    h264encoder::UnloadEncoderLibrary();

    glfwTerminate();
#else
    emscripten_set_main_loop_arg(update, nullptr, 0, 0);
    glfwSwapInterval(1);
#endif

    return 0;
}
