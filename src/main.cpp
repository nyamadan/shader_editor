#include <string>
#include <iostream>
#include <sstream>
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
#include "webm_encoder.hpp"

namespace {

const char *const DefaultAssetPath = "./default";
const char *const DefaultVertexShaderName = "vert.glsl";
const char *const DefaultFragmentShaderName = "frag.glsl";
const char *const CopyFragmentShaderName = "copy.glsl";
const char *const ShaderToyPreSourceName = "pre_shadertoy.glsl";

std::string assetPath;

int32_t windowWidth = 1024;
int32_t windowHeight = 768;

int32_t bufferWidth;
int32_t bufferHeight;

GLFWwindow *mainWindow = nullptr;

GLuint vIndex = 0;
GLuint vPosition = 0;
GLuint vertexArraysObject = 0;

std::string shaderToyPreSource;

std::shared_ptr<ShaderProgram> program;
std::shared_ptr<ShaderProgram> copyProgram;

int32_t writeBufferIndex = 0;
int32_t readBufferIndex = 1;
GLuint frameBuffers[2];
GLuint depthBuffers[2];
GLuint backBuffers[2];

GLuint pixelBuffer;

const float CheckInterval = 0.5f;
float lastCheckUpdate = 0;
float timeStart = 0;

uint8_t *yuvBuffer = nullptr;
int64_t currentFrame = 0;
bool isRecording = false;
FILE *fVideo;

std::unique_ptr<WebmEncoder> pEncoder = nullptr;

std::vector<std::shared_ptr<Image>> imageFiles;
int32_t numImageFileNames = 0;
char **imageFileNames = nullptr;

std::vector<std::shared_ptr<ShaderProgram>> shaderFiles;
int32_t numShaderFileNames = 0;
char **shaderFileNames = nullptr;

TextEditor editor;

const GLfloat positions[] = {-1.0f, 1.0f,  0.0f, 1.0f, 1.0f,  0.0f,
                             -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f};

const GLushort indices[] = {0, 2, 1, 1, 2, 3};

void glfwErrorCallback(int error, const char *description) {
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
    shaderFileNames = new char *[numShaderFileNames];

    for (int64_t i = 0; i < numShaderFileNames; i++) {
        std::string fileStr = shaderFiles[i]->getFragmentShader().getPath();
        const int64_t fileNameLength = fileStr.size();
        char *fileName = new char[fileNameLength + 1];
        fileName[fileNameLength] = '\0';
        fileStr.copy(fileName, fileNameLength, 0);
        shaderFileNames[i] = fileName;
    }
}

void genImageFileNames() {
    deleteImageFileNames();

    numImageFileNames = static_cast<int32_t>(imageFiles.size());
    imageFileNames = new char *[numImageFileNames];
    for (int64_t i = 0; i < numImageFileNames; i++) {
        std::shared_ptr<Image> image = imageFiles[i];
        const std::string &path = image->getPath();
        const int64_t fileNameLength = path.size();
        char *fileName = new char[fileNameLength + 1];
        fileName[fileNameLength] = '\0';
        image->getPath().copy(fileName, fileNameLength, 0);
        imageFileNames[i] = fileName;
    }
}

void compileShaderFromFile(const std::shared_ptr<ShaderProgram> program,
                           const std::string &vsPath,
                           const std::string &fsPath) {
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
                             const std::string &fsSource) {
    const std::string &vsPath = program->getVertexShader().getPath();
    const std::string &fsPath = program->getFragmentShader().getPath();
    const int64_t vsTime = getMTime(vsPath);
    const int64_t fsTime = getMTime(fsPath);
    const std::string &vsSource = program->getVertexShader().getSource();
    newProgram->compile(vsPath, fsPath, vsSource, fsSource, vsTime, fsTime);
}

void recompileShaderFromFile(const std::shared_ptr<ShaderProgram> program,
                             std::shared_ptr<ShaderProgram> newProgram) {
    const std::string &vsPath = program->getVertexShader().getPath();
    const std::string &fsPath = program->getFragmentShader().getPath();
    const int64_t vsTime = getMTime(vsPath);
    const int64_t fsTime = getMTime(fsPath);
    std::string vsSource;
    std::string fsSource;
    readText(vsPath, vsSource);
    readText(fsPath, fsSource);
    newProgram->compile(vsPath, fsPath, vsSource, fsSource, vsTime, fsTime);
}

void updateFrameBuffersSize(GLint width, GLint height) {
    bufferWidth = width;
    bufferHeight = height;

    glBindBuffer(GL_PIXEL_PACK_BUFFER, pixelBuffer);
    glBufferData(GL_PIXEL_PACK_BUFFER, bufferWidth * bufferHeight * 4, 0,
                 GL_STREAM_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    for (int32_t i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[i]);

        glBindRenderbuffer(GL_RENDERBUFFER, depthBuffers[i]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                              bufferWidth, bufferHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER, depthBuffers[i]);

        glBindTexture(GL_TEXTURE_2D, backBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufferWidth, bufferHeight, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D, backBuffers[i], 0);

        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    if (yuvBuffer != nullptr) {
        delete[] yuvBuffer;
    }

    const int32_t ySize = bufferWidth * bufferHeight;
    const int32_t uSize = ySize / 4;
    const int32_t vSize = uSize;
    yuvBuffer = new uint8_t[ySize + uSize + vSize];
}

void ShowTextEditor(bool &showTextEditor, int32_t &uiShader,
                    int32_t uiPlatform) {
    auto cpos = editor.GetCursorPosition();
    ImGui::Begin(
        "Text Editor", &showTextEditor,
        ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
    ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save")) {
                const std::string &textToSave = editor.GetText();
                const char *const buffer = textToSave.c_str();
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

                        const std::string &vsPath =
                            program->getVertexShader().getPath();
                        const std::string &fsPath = path;
                        const int64_t vsTime = getMTime(vsPath);
                        const int64_t fsTime = getMTime(fsPath);
                        const std::string &vsSource =
                            program->getVertexShader().getSource();
                        const std::string &fsSource = textToSave;

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

void swapProgram(std::shared_ptr<ShaderProgram> newProgram) {
    newProgram->copyAttributesFrom(*program);
    newProgram->copyUniformsFrom(*program);

    glBindVertexArray(vertexArraysObject);
    glBindBuffer(GL_ARRAY_BUFFER, vPosition);
    newProgram->applyAttributes();
    glBindVertexArray(0);

    glUseProgram(newProgram->getProgram());

    program.swap(newProgram);
}

void startRecord(const std::string &fileName, const int32_t uiVideoTypeIndex,
                 const int32_t uiVideoResolution, const int32_t kbps,
                 float &uiTimeValue) {
    isRecording = true;
    currentFrame = 0;
    uiTimeValue = 0;

    switch (uiVideoResolution) {
        case 0:
            updateFrameBuffersSize(256, 144);
            break;
        case 1:
            updateFrameBuffersSize(427, 240);
            break;
        case 2:
            updateFrameBuffersSize(640, 360);
            break;
        case 3:
            updateFrameBuffersSize(720, 480);
            break;
        case 4:
            updateFrameBuffersSize(1280, 720);
            break;
        case 5:
            updateFrameBuffersSize(1920, 1080);
            break;
        case 6:
            updateFrameBuffersSize(2560, 1440);
            break;
        case 7:
            updateFrameBuffersSize(3840, 2160);
            break;
    }

    switch (uiVideoTypeIndex) {
        case 0: {
            fVideo = fopen(fileName.c_str(), "wb");
            fprintf(fVideo,
                    "YUV4MPEG2 W%d H%d F30000:1001 Ip A0:0 C420 XYSCSS=420\n",
                    bufferWidth, bufferHeight);
        } break;
        case 1: {
            pEncoder = std::make_unique<WebmEncoder>(
                fileName, 1001, 30000, bufferWidth, bufferHeight, kbps);
        } break;
    }
}

void writeOneFrame(const uint8_t *rgbaBuffer, const int32_t uiVideoTypeIndex) {
    const int32_t ySize = bufferWidth * bufferHeight;
    const int32_t uSize = ySize / 4;
    const int32_t vSize = uSize;

    const int32_t yStride = bufferWidth;
    const int32_t uStride = bufferWidth / 2;
    const int32_t vStride = uStride;

    uint8_t *yBuffer = yuvBuffer;
    uint8_t *uBuffer = yBuffer + ySize;
    uint8_t *vBuffer = uBuffer + uSize;

    switch (uiVideoTypeIndex) {
        case 0: {
            libyuv::ABGRToI420(rgbaBuffer, bufferWidth * 4, yBuffer, yStride,
                               uBuffer, uStride, vBuffer, vStride, bufferWidth,
                               -bufferHeight);

            fputs("FRAME\n", fVideo);
            fwrite(yuvBuffer, sizeof(uint8_t), ySize + uSize + vSize, fVideo);
        } break;
        case 1: {
            pEncoder->addRGBAFrame(rgbaBuffer);
        } break;
    }
}

void update(void *) {
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
    static bool showAppLogWindow = false;

    static float uiTimeValue = 0;
    static bool uiPlaying = true;

    static int32_t uiShaderFileIndex = 0;
    static int32_t uiShaderPlatformIndex = 0;
    static int32_t uiBufferQualityIndex = 2;
    static int32_t uiVideoResolutionIndex = 2;
    static int32_t uiVideoTypeIndex = 0;
    static float uiVideoMbps = 1.0f;
    static float uiVideoTime = 5.0f;
    static std::map<std::string, int32_t> uiImages;

    std::map<std::string, std::shared_ptr<Image>> usedTextures;
    int currentWidth, currentHeight;
    float bufferScale =
        1.0f / powf(2.0f, static_cast<float>(uiBufferQualityIndex - 1));

    float now = static_cast<float>(ImGui::GetTime());

    const char *uMouseName = nullptr;
    const char *uResolutionName = nullptr;
    const char *uTimeName = nullptr;
    const char *uFrameName = nullptr;
    const char *uBackbuffer = nullptr;

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
            if (uiShowTextEditor) {
                std::string text;
                readText(program->getFragmentShader().getPath(), text);
                editor.SetText(text);
            } else {
                if (uiShaderPlatformIndex == 1) {
                    newProgram->setFragmentShaderPreSource(shaderToyPreSource);
                } else {
                    newProgram->setFragmentShaderPreSource(std::string());
                }
                recompileShaderFromFile(program, newProgram);
                editor.SetErrorMarkers(
                    newProgram->getFragmentShader().getErrors());
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
        editor.SetErrorMarkers(newProgram->getFragmentShader().getErrors());
    }

    if (newProgram->isOK()) {
        swapProgram(newProgram);

        shaderFiles[uiShaderFileIndex] = program;
    }

    const bool *const mouseDown = ImGui::GetIO().MouseDown;
    const ImVec2 &mousePos = ImGui::IsMousePosValid() && !isRecording
                                 ? ImGui::GetMousePos()
                                 : ImVec2(0.5f, 0.5f);

    // onResizeWindow
    if (!isRecording &&
        (currentWidth != windowWidth || currentHeight != windowHeight)) {
        updateFrameBuffersSize(static_cast<GLint>(currentWidth * bufferScale),
                               static_cast<GLint>(currentHeight * bufferScale));
    }

    windowWidth = currentWidth;
    windowHeight = currentHeight;

    if (isRecording) {
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
        auto &uniforms = program->getUniforms();
        for (auto it = uniforms.begin(); it != uniforms.end(); it++) {
            ShaderUniform &u = it->second;

            if (u.type == UniformType::Sampler2D && u.name != uBackbuffer) {
                int32_t &uiImage = uiImages[u.name];

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
        ImGui::Begin("Shader Editor", &uiDebugWindow,
                     ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Checkbox("File", &uiShaderFileWindow);
        ImGui::Checkbox("Time", &uiTimeWindow);
        ImGui::Checkbox("Stats", &uiStatsWindow);
        ImGui::Checkbox("Uniforms", &uiUniformWindow);
        ImGui::Checkbox("BackBuffer", &uiBackBufferWindow);
        ImGui::Checkbox("Capture", &uiCaptureWindow);

        ImGui::Checkbox("Log", &showAppLogWindow);
#ifndef NDEBUG
        ImGui::Checkbox("ImGui Demo Window", &showImGuiDemoWindow);
#endif

        ImGui::End();

        if (uiShaderFileWindow) {
            ImGui::Begin("File", &uiShaderFileWindow,
                         ImGuiWindowFlags_AlwaysAutoResize);

            const char *const items[] = {"glslsandbox", "shadertoy"};
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

                editor.SetErrorMarkers(
                    newProgram->getFragmentShader().getErrors());

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
                editor.SetErrorMarkers(
                    newProgram->getFragmentShader().getErrors());

                swapProgram(newProgram);
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
                    editor.SetErrorMarkers(
                        newProgram->getFragmentShader().getErrors());

                    shaderFiles.push_back(newProgram);
                    genShaderFileNames();

                    uiShaderFileIndex = numShaderFileNames - 1;

                    program.swap(newProgram);
                }
            }
#endif

            ImGui::Checkbox("TextEditor", &uiShowTextEditor);

            if (uiShowTextEditor) {
                ShowTextEditor(uiShowTextEditor, uiShaderFileIndex,
                               uiShaderPlatformIndex);
            }

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
            ImGui::Begin("Capture", &uiCaptureWindow,
                         ImGuiWindowFlags_AlwaysAutoResize);
            const char *resolutionItems[] = {
                "256x144",  "427x240",   "640x360",   "720x480",
                "1280x720", "1920x1080", "2560x1440", "3840x2160"};

            const char *typeItems[] = {"I420", "WebM"};

            ImGui::Combo("resolution", &uiVideoResolutionIndex, resolutionItems,
                         IM_ARRAYSIZE(resolutionItems));

            ImGui::Combo("type", &uiVideoTypeIndex, typeItems,
                         IM_ARRAYSIZE(typeItems));

            if (uiVideoTypeIndex == 1) {
                ImGui::DragFloat("MBps", &uiVideoMbps, 0.1f, 0.5f, 15.0f);
            }

            ImGui::DragFloat("seconds", &uiVideoTime, 0.5f, 0.5f, 600.0f, "%f");

            std::stringstream ss;
            ss.str(std::string());
            ss << 30000.0f / 1001.0f << " hz";
            ImGui::LabelText("framerate", "%s", ss.str().c_str());

            std::string fileName;
            const char *const y4mFilter = "Video file (*.y4m)\0*.y4m\0";
            const char *const y4mExt = "y4m";
            const char *const webmFilter = "Video file (*.webm)\0*.webm\0";
            const char *const webmExt = "webm";
            const char *filter = nullptr;
            const char *ext = nullptr;
            bool ok = false;

            if (ImGui::Button("Save")) {
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
                                uiTimeValue);
                    ImGui::OpenPopup("Export Progress");
                }
            }

            if (ImGui::BeginPopupModal("Export Progress", NULL,
                                       ImGuiWindowFlags_AlwaysAutoResize)) {
                if (!isRecording) {
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
            std::map<const std::string, ShaderUniform> &uniforms =
                program->getUniforms();
            for (auto it = uniforms.begin(); it != uniforms.end(); it++) {
                const std::string &name = it->first;
                ShaderUniform &u = it->second;
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
                            int32_t &uiImage = uiImages[u.name];

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
            const char *const items[] = {"0.5", "1", "2", "4", "8"};
            if (ImGui::Combo("quality", &uiBufferQualityIndex, items,
                             IM_ARRAYSIZE(items))) {
                // update framebuffers
                bufferScale =
                    1.0f /
                    powf(2.0f, static_cast<float>(uiBufferQualityIndex - 1));
                updateFrameBuffersSize(
                    static_cast<GLint>(currentWidth * bufferScale),
                    static_cast<GLint>(currentHeight * bufferScale));
            }

            std::stringstream windowStringStream;
            windowStringStream << windowWidth << ", " << windowHeight;
            ImGui::LabelText("window", "%s", windowStringStream.str().c_str());

            std::stringstream bufferStringStream;
            bufferStringStream << bufferWidth << ", " << bufferHeight;
            ImGui::LabelText("buffer", "%s", bufferStringStream.str().c_str());
            ImGui::End();
        }

        if (showAppLogWindow) {
            AppLog::getInstance().showAppLogWindow(showAppLogWindow);
        }

#ifndef NDEBUG
        if (showImGuiDemoWindow) {
            ImGui::ShowDemoWindow(&showImGuiDemoWindow);
        }
#endif
    }

    ImGui::Render();

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[writeBufferIndex]);
    glViewport(0, 0, bufferWidth, bufferHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // uniform values
    program->setUniformValue(uResolutionName,
                             glm::vec2(static_cast<GLfloat>(bufferWidth),
                                       static_cast<GLfloat>(bufferHeight)));

    switch (uiShaderPlatformIndex) {
        case 0:
            program->setUniformValue(
                uMouseName, glm::vec2(mousePos.x / windowWidth,
                                      1.0f - mousePos.y / windowHeight));
            break;
        case 1:
            program->setUniformValue(uFrameName,
                                     static_cast<int>(currentFrame));
            if (mouseDown[0] || mouseDown[1]) {
                program->setUniformValue(
                    uMouseName, glm::vec4(mousePos.x, windowHeight - mousePos.y,
                                          mouseDown[0] ? 1.0f : 0.0f,
                                          mouseDown[1] ? 1.0f : 0.0f));
            } else {
                if (program->containsUniform(uMouseName)) {
                    const glm::vec4 &prevMousePos =
                        program->getUniform(uMouseName).value.vec4;
                    program->setUniformValue(
                        uMouseName,
                        glm::vec4(prevMousePos.x, prevMousePos.y, 0, 0));
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
            const std::string &name = iter->first;
            std::shared_ptr<Image> image = iter->second;

            if (!image->isLoaded()) {
                image->load();
            }

            glActiveTexture(GL_TEXTURE0 + channel);
            glBindTexture(GL_TEXTURE_2D, image->getTexture());
            program->setUniformValue(iter->first, channel++);
        }

        glActiveTexture(GL_TEXTURE0 + channel);
        glBindTexture(GL_TEXTURE_2D, backBuffers[readBufferIndex]);
        program->setUniformValue(uBackbuffer, channel++);

        program->applyUniforms();

        if (isRecording) {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, pixelBuffer);
        }

        glBindVertexArray(vertexArraysObject);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    }

    if (isRecording) {
        glReadPixels(0, 0, bufferWidth, bufferHeight, GL_RGBA, GL_UNSIGNED_BYTE,
                     0);
        auto ptr = static_cast<uint8_t *>(
            glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));
        writeOneFrame(ptr, uiVideoTypeIndex);
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        ptr = nullptr;
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        currentFrame++;

        if (uiTimeValue >= uiVideoTime) {
            switch (uiVideoTypeIndex) {
                case 0: {
                    fclose(fVideo);
                    fVideo = nullptr;
                } break;
                case 1: {
                    pEncoder->finalize();
                } break;
            }

            isRecording = false;
        }
    } else {
        if (uiPlaying) {
            currentFrame++;
        }
    }

    // swap buffer
    std::swap(writeBufferIndex, readBufferIndex);

    // copy to frontbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(copyProgram->getProgram());

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, backBuffers[readBufferIndex]);

    copyProgram->setUniformValue("backbuffer", 0);
    copyProgram->setUniformValue("resolution",
                                 glm::vec2(windowWidth, windowHeight));
    copyProgram->applyUniforms();

    glBindVertexArray(vertexArraysObject);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwMakeContextCurrent(mainWindow);

    glfwSwapBuffers(mainWindow);

    for (GLenum error = glGetError(); error; error = glGetError()) {
        AppLog::getInstance().addLog("error code: 0x%0X\n", error);
    }

    glfwPollEvents();
}

void loadFiles(const std::string &path) {
    AppLog::getInstance().addLog("Assets:\n");

    std::vector<std::string> files = openDir(path);
    for (auto iter = files.begin(); iter != files.end(); iter++) {
        const std::string &file = *iter;
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
}  // namespace

int main(void) {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) return -1;

#ifdef __EMSCRIPTEN__
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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

    // Initialize Buffers
    glGenVertexArrays(1, &vertexArraysObject);
    glGenBuffers(1, &vIndex);
    glGenBuffers(1, &vPosition);
    glGenBuffers(1, &pixelBuffer);

    glBindVertexArray(vertexArraysObject);
    glBindBuffer(GL_ARRAY_BUFFER, vPosition);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 12, positions,
                 GL_STATIC_DRAW);
    program->applyAttributes();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vIndex);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * 6, indices,
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Framebuffers
    glGenFramebuffers(2, frameBuffers);
    glGenRenderbuffers(2, depthBuffers);
    glGenTextures(2, backBuffers);

    updateFrameBuffersSize(windowWidth / 2, windowHeight / 2);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

    glfwTerminate();
#else
    emscripten_set_main_loop_arg(update, nullptr, 0, 0);
    glfwSwapInterval(1);
#endif

    return 0;
}
