#include <string>
#include <iostream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <memory>

#include <stb_image_write.h>

#include <imgui.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>
#include <TextEditor.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "common.hpp"
#include "file_utils.hpp"
#include "shader_utils.hpp"
#include "shader_program.hpp"
#include "app_log.hpp"

namespace {

const char *const DefaultAssetPath = "default";
const char *const DefaultVertexShaderName = "vert.glsl";
const char *const DefaultFragmentShaderName = "frag.glsl";
const char *const CopyFragmentShaderName = "copy.glsl";

std::string assetPath;

int windowWidth = 1024;
int windowHeight = 768;

int bufferWidth;
int bufferHeight;

GLint positionLocation = 0;

GLFWwindow *mainWindow = nullptr;

GLuint vIndex = 0;
GLuint vPosition = 0;
GLuint vertexArraysObject = 0;

GLint uTime = 0;
GLint uMouse = 0;
GLint uResolution = 0;
std::shared_ptr<ShaderProgram> program;

GLint uCopyResolution = 0;
GLint uCopyBackBuffer = 0;
ShaderProgram copyProgram;

int writeBufferIndex = 0;
int readBufferIndex = 1;
GLuint frameBuffers[2];
GLuint depthBuffers[2];
GLuint backBuffers[2];

const float CheckInterval = 0.5f;
float lastCheckUpdate = 0;
float timeStart = 0;

time_t lastMTimeVS = 0;
time_t lastMTimeFS = 0;

uint8_t *rgbaBuffer = nullptr;
uint8_t *yuvBuffer = nullptr;
time_t currentFrame = 0;
time_t frameEnd = 0;
FILE *fVideo;

TextEditor editor;

const GLfloat positions[] = {-1.0f, 1.0f,  0.0f, 1.0f, 1.0f,  0.0f,
                             -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f};

const GLushort indices[] = {0, 2, 1, 1, 2, 3};

void glfwErrorCallback(int error, const char *description) {
    AppLog::getInstance().addLog("error %d: %s\n", error, description);
}

void updateFrameBuffers(GLint width, GLint height) {
    bufferWidth = width;
    bufferHeight = height;

    for (int i = 0; i < 2; i++) {
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
    }

    if (rgbaBuffer != nullptr) {
        delete[] rgbaBuffer;
    }
    if (yuvBuffer != nullptr) {
        delete[] yuvBuffer;
    }

    rgbaBuffer = new uint8_t[bufferWidth * bufferHeight * 4];
    yuvBuffer = new uint8_t[bufferWidth * bufferHeight * 3];
}

void ShowTextEditor(bool &showTextEditor) {
    auto cpos = editor.GetCursorPosition();
    ImGui::Begin(
        "Text Editor", nullptr,
        ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
    ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save")) {
                const std::string &textToSave = editor.GetText();
                const char *const buffer = textToSave.c_str();
                const size_t size = textToSave.size();
                writeText(program->getFragmentShader().getPath(), buffer, size);
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
                                !ro && ImGui::GetClipboardText() != nullptr))
                editor.Paste();

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

}  // namespace

void update(void *) {
    // ImGUI
    static bool showImGuiDemoWindow = false;
    static bool showDebugWindow = true;
    static bool showAppLogWindow = false;
    static bool showTextEditor = false;

    static int uiBufferQuality = 1;
    static bool uiPlaying = true;
    static float uiTimeValue = 0;
    static int uiVideoResolution = 2;
    static float uiVideoTime = 5.0f;

    // Uniforms
    float uTimeValue;
    glm::vec2 uResolutionValue;
    glm::vec2 uMouseValue;

    int currentWidth, currentHeight;
    double xpos, ypos;
    float bufferScale =
        1.0f / powf(2.0f, static_cast<float>(uiBufferQuality - 1));

    float now = static_cast<float>(ImGui::GetTime());

    std::shared_ptr<ShaderProgram> newProgram(new ShaderProgram());
    if (now - lastCheckUpdate > CheckInterval) {
        if (program->checkExpiredWithReset()) {
            if (showTextEditor) {
                char *buffer;
                readText(program->getFragmentShader().getPath(), buffer);
                editor.SetText(buffer);
                delete[] buffer;
            } else {
                newProgram->compile(program->getVertexShader().getPath(),
                                    program->getFragmentShader().getPath());
            }
        }

        lastCheckUpdate = now;
    }

    // OnUpdateText
    if (showTextEditor && editor.IsTextChanged()) {
        newProgram->compileWithSource(program->getVertexShader().getPath(),
                                      program->getVertexShader().getSource(),
                                      program->getFragmentShader().getPath(),
                                      editor.GetText());
        TextEditor::ErrorMarkers markers;
        if (!newProgram->isOK()) {
            const Shader &fs = newProgram->getFragmentShader();

            if (!fs.isOK()) {
                const std::string &error = fs.getError();
#ifdef __EMSCRIPTEN__
                const std::regex re("^ERROR: \\d+:(\\d+): (.*)$");
#else
                const std::regex re("^\\d\\((\\d+)\\) : (.*)$");
#endif
                std::istringstream ss(error);
                std::string line;
                while (std::getline(ss, line)) {
                    std::smatch m;
                    std::regex_match(line, m, re);
                    if (m.size() == 3) {
                        int lineNumber = std::atoi(m[1].str().c_str());
                        const std::string message = m[2].str();

                        auto pair = std::make_pair(lineNumber, message);
                        markers.insert(pair);
                    }
                }
            }
        }
        editor.SetErrorMarkers(markers);
    }

    if (newProgram->isOK()) {
        positionLocation =
            glGetAttribLocation(newProgram->getProgram(), "aPosition");
        uTime = glGetUniformLocation(newProgram->getProgram(), "time");
        uMouse = glGetUniformLocation(newProgram->getProgram(), "mouse");
        uResolution =
            glGetUniformLocation(newProgram->getProgram(), "resolution");
        glUseProgram(newProgram->getProgram());
        glDeleteProgram(program->getProgram());

        program.swap(newProgram);

        glBindVertexArray(vertexArraysObject);
        glBindBuffer(GL_ARRAY_BUFFER, vPosition);
        glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindVertexArray(0);
    }

    glfwMakeContextCurrent(mainWindow);
    glfwGetFramebufferSize(mainWindow, &currentWidth, &currentHeight);
    glfwGetCursorPos(mainWindow, &xpos, &ypos);

    // onResizeWindow
    if (frameEnd == 0) {
        if (currentWidth != bufferWidth || currentHeight != bufferHeight) {
            updateFrameBuffers(static_cast<GLint>(currentWidth * bufferScale),
                               static_cast<GLint>(currentHeight * bufferScale));
        }
    }

    windowWidth = currentWidth;
    windowHeight = currentHeight;

    // uniform values
    uResolutionValue.x = static_cast<GLfloat>(bufferWidth);
    uResolutionValue.y = static_cast<GLfloat>(bufferHeight);
    uMouseValue.x = static_cast<GLfloat>(xpos) / windowWidth;
    uMouseValue.y = 1.0f - static_cast<GLfloat>(ypos) / windowHeight;

    if (frameEnd > 0) {
        uTimeValue = currentFrame * 30.0f / 1001.0f;
    } else {
        if (!uiPlaying) {
            timeStart = now - uiTimeValue;
            uTimeValue = uiTimeValue;
        } else {
            uTimeValue = now - timeStart;
        }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (showDebugWindow) {
        if (ImGui::CollapsingHeader("Stats")) {
            std::stringstream ss;

            ss.str(std::string());
            ss << 1000.0f / ImGui::GetIO().Framerate;
            ImGui::LabelText("ms/frame", "%s", ss.str().c_str());

            ss.str(std::string());
            ss << ImGui::GetIO().Framerate;
            ImGui::LabelText("fps", "%s", ss.str().c_str());
        }

        if (ImGui::CollapsingHeader("Uniforms")) {
            std::stringstream ss;

            ss.str(std::string());
            ss << uTimeValue;
            ImGui::LabelText("time", "%s", ss.str().c_str());

            ss.str(std::string());
            ss << uResolutionValue.x << ", " << uResolutionValue.y;
            ImGui::LabelText("resolution", "%s", ss.str().c_str());

            ss.str(std::string());
            ss << uMouseValue.x << ", " << uMouseValue.y;
            ImGui::LabelText("mouse", "%s", ss.str().c_str());
            ss.clear();
        }

        if (ImGui::CollapsingHeader("Time")) {
            if (uiPlaying) {
                std::stringstream ss;
                ss << uTimeValue;
                ImGui::LabelText("time", "%s", ss.str().c_str());
            } else {
                ImGui::DragFloat("time", &uiTimeValue, 0.5f);
            }

            if (ImGui::Checkbox("Playing", &uiPlaying)) {
                uiTimeValue = uTimeValue;
            }

            if (ImGui::Button("Reset")) {
                timeStart = now;
                uiTimeValue = 0;
                uTimeValue = 0;
            }
        }

        if (ImGui::CollapsingHeader("Buffer")) {
            const char *items[] = {"0.5", "1", "2", "4", "8"};
            if (ImGui::Combo("quality", &uiBufferQuality, items,
                             IM_ARRAYSIZE(items))) {
                // update framebuffers
                bufferScale =
                    1.0f / powf(2.0f, static_cast<float>(uiBufferQuality - 1));
                updateFrameBuffers(
                    static_cast<GLint>(currentWidth * bufferScale),
                    static_cast<GLint>(currentHeight * bufferScale));

                // uniform values
                uResolutionValue.x = static_cast<GLfloat>(bufferWidth);
                uResolutionValue.y = static_cast<GLfloat>(bufferHeight);
                uMouseValue.x =
                    static_cast<GLfloat>(xpos) * bufferScale / (bufferWidth);
                uMouseValue.y = 1.0f - static_cast<GLfloat>(ypos) *
                                           bufferScale / (bufferHeight);
            }

            std::stringstream windowStringStream;
            windowStringStream << windowWidth << ", " << windowHeight;
            ImGui::LabelText("window", "%s", windowStringStream.str().c_str());

            std::stringstream bufferStringStream;
            bufferStringStream << bufferWidth << ", " << bufferHeight;
            ImGui::LabelText("buffer", "%s", bufferStringStream.str().c_str());
        }

        if (ImGui::CollapsingHeader("Export")) {
            const char *items[] = {"256x144",   "427x240",  "640x360",
                                   "720x480",   "1280x720", "1920x1080",
                                   "2560x1440", "3840x2160"};

            if (ImGui::Combo("resolution", &uiVideoResolution, items,
                             IM_ARRAYSIZE(items))) {
            }

            ImGui::DragFloat("seconds", &uiVideoTime, 0.5f, 0.5f, 600.0f, "%f");

            std::stringstream ss;
            ss.str(std::string());
            ss << 30000.0f / 1001.0f << " hz";
            ImGui::LabelText("framerate", "%s", ss.str().c_str());

            if (ImGui::Button("Save")) {
                frameEnd =
                    static_cast<time_t>(30000.0f * uiVideoTime / 1001.0f);
                currentFrame = 0;
                uTimeValue = 0;

                switch (uiVideoResolution) {
                    case 0:
                        updateFrameBuffers(256, 144);
                        break;
                    case 1:
                        updateFrameBuffers(427, 240);
                        break;
                    case 2:
                        updateFrameBuffers(640, 360);
                        break;
                    case 3:
                        updateFrameBuffers(720, 480);
                        break;
                    case 4:
                        updateFrameBuffers(1280, 720);
                        break;
                    case 5:
                        updateFrameBuffers(1920, 1080);
                        break;
                    case 6:
                        updateFrameBuffers(2560, 1440);
                        break;
                    case 7:
                        updateFrameBuffers(3840, 2160);
                        break;
                }

                fVideo = fopen("video.y4m", "wb");

                fprintf(
                    fVideo,
                    "YUV4MPEG2 W%d H%d F30000:1001 Ip A0:0 C444 XYSCSS=444\n",
                    bufferWidth, bufferHeight);

                ImGui::OpenPopup("Export Progress");
            }

            if (ImGui::BeginPopupModal("Export Progress", NULL,
                                       ImGuiWindowFlags_AlwaysAutoResize)) {
                if (frameEnd == 0) {
                    ImGui::ProgressBar(1.0f, ImVec2(200.0f, 15.0f));

                    ImGui::Separator();

                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                } else {
                    ImGui::ProgressBar(
                        static_cast<float>(currentFrame) / frameEnd,
                        ImVec2(200.0f, 15.0f));
                }

                ImGui::EndPopup();
            }
        }

        if (ImGui::CollapsingHeader("Window")) {
            ImGui::Checkbox("Log", &showAppLogWindow);
            ImGui::Checkbox("TextEditor", &showTextEditor);
            ImGui::Checkbox("ImGui Demo Window", &showImGuiDemoWindow);
        }
    }

    if (showTextEditor) {
        ShowTextEditor(showTextEditor);
    }

    if (showAppLogWindow) {
        AppLog::getInstance().showAppLogWindow(showAppLogWindow);
    }

    if (showImGuiDemoWindow) {
        ImGui::ShowDemoWindow(&showImGuiDemoWindow);
    }

    ImGui::Render();

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[writeBufferIndex]);
    glViewport(0, 0, bufferWidth, bufferHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program->getProgram());

    if (uResolution >= 0) {
        glUniform2fv(uResolution, 1, glm::value_ptr(uResolutionValue));
    }

    if (uMouse >= 0) {
        glUniform2fv(uMouse, 1, glm::value_ptr(uMouseValue));
    }

    if (uTime >= 0) {
        glUniform1f(uTime, uTimeValue);
    }

    glBindVertexArray(vertexArraysObject);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    if (frameEnd != 0) {
        glReadPixels(0, 0, bufferWidth, bufferHeight, GL_RGBA, GL_UNSIGNED_BYTE,
                     rgbaBuffer);

        for (int x = 0; x < bufferWidth; x++) {
            for (int y = 0; y < bufferHeight; y++) {
                const int i = y * bufferWidth + x;
                const int j = i * 4;
                const int size = bufferWidth * bufferHeight;
                const uint8_t R = rgbaBuffer[j + 0];
                const uint8_t G = rgbaBuffer[j + 1];
                const uint8_t B = rgbaBuffer[j + 2];
                const uint8_t Y =
                    static_cast<uint8_t>(0.299 * R + 0.587 * G + 0.114 * B);
                const uint8_t U = static_cast<uint8_t>(-0.169 * R - 0.331 * G +
                                                       0.5 * B + 128);
                const uint8_t V =
                    static_cast<uint8_t>(0.5 * R - 0.419 * G - 0.081 * B + 128);

                yuvBuffer[i] = Y;
                yuvBuffer[i + size] = U;
                yuvBuffer[i + size * 2] = V;
            }
        }

        fputs("FRAME\n", fVideo);
        fwrite(yuvBuffer, sizeof(uint8_t), bufferWidth * bufferHeight * 3,
               fVideo);

        currentFrame++;

        if (frameEnd <= currentFrame) {
            fclose(fVideo);
            fVideo = nullptr;
            frameEnd = 0;
        }
    }

    // copy to frontbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(copyProgram.getProgram());

    if (uCopyResolution >= 0) {
        glm::vec2 uCopyResolutionValue(windowWidth, windowHeight);
        glUniform2fv(uCopyResolution, 1, glm::value_ptr(uCopyResolutionValue));
    }

    if (uCopyBackBuffer >= 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, backBuffers[writeBufferIndex]);
        glUniform1i(uCopyBackBuffer, 0);
    }

    glBindVertexArray(vertexArraysObject);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwMakeContextCurrent(mainWindow);

    glfwSwapBuffers(mainWindow);

    std::swap(writeBufferIndex, readBufferIndex);

    for (GLenum error = glGetError(); error; error = glGetError()) {
        AppLog::getInstance().addLog("error code: %0X\n", error);
    }

    glfwPollEvents();
}  // namespace
}  // namespace

int main(void) {
    std::string vertexShaderPath = DefaultAssetPath;
#ifdef WIN32
    if (vertexShaderPath.back() != '\\' && vertexShaderPath.back() != '/') {
        vertexShaderPath.append("\\");
    }
#else
    if (vertexShaderPath.back() != '/') {
        vertexShaderPath.append("/");
    }
#endif
    vertexShaderPath.append(DefaultVertexShaderName);

    std::string fragmentShaderPath = DefaultAssetPath;
#ifdef WIN32
    if (fragmentShaderPath.back() != '\\' && fragmentShaderPath.back() != '/') {
        fragmentShaderPath.append("\\");
    }
#else
    if (fragmentShaderPath.back() != '/') {
        fragmentShaderPath.append("/");
    }
#endif
    fragmentShaderPath.append(DefaultFragmentShaderName);

    std::string copyFS = DefaultAssetPath;
#ifdef WIN32
    if (copyFS.back() != '\\' && copyFS.back() != '/') {
        copyFS.append("\\");
    }
#else
    if (copyFS.back() != '/') {
        copyFS.append("/");
    }
#endif
    copyFS.append(CopyFragmentShaderName);

    if (!glfwInit()) return -1;

    glfwSetErrorCallback(glfwErrorCallback);

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

#ifdef _DEBUG
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

    glfwMakeContextCurrent(mainWindow);

    // Compile shaders.
    program.reset(new ShaderProgram());
    program->compile(vertexShaderPath, fragmentShaderPath);
    assert(program->getProgram());

    // getProgramLocation
    positionLocation = glGetAttribLocation(program->getProgram(), "aPosition");
    uTime = glGetUniformLocation(program->getProgram(), "time");
    uMouse = glGetUniformLocation(program->getProgram(), "mouse");
    uResolution = glGetUniformLocation(program->getProgram(), "resolution");

    copyProgram.compile(vertexShaderPath, copyFS);
    assert(copyProgram.getProgram());
    uCopyResolution =
        glGetUniformLocation(copyProgram.getProgram(), "resolution");
    uCopyBackBuffer =
        glGetUniformLocation(copyProgram.getProgram(), "backbuffer");

    // Initialize Buffers
    glGenVertexArrays(1, &vertexArraysObject);
    glGenBuffers(1, &vIndex);
    glGenBuffers(1, &vPosition);

    glBindVertexArray(vertexArraysObject);
    glBindBuffer(GL_ARRAY_BUFFER, vPosition);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 12, positions,
                 GL_STATIC_DRAW);
    glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vIndex);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * 6, indices,
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Framebuffers
    glGenFramebuffers(2, frameBuffers);
    glGenRenderbuffers(2, depthBuffers);
    glGenTextures(2, backBuffers);

    updateFrameBuffers(windowWidth, windowHeight);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    timeStart = static_cast<float>(ImGui::GetTime());

    auto lang = TextEditor::LanguageDefinition::GLSL();
    editor.SetLanguageDefinition(lang);
    editor.SetText(program->getFragmentShader().getSource());

#ifndef __EMSCRIPTEN__
    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(mainWindow)) {
        update(nullptr);
    }

    glfwTerminate();
#else
    emscripten_set_main_loop_arg(update, nullptr, 0, 0);
    glfwSwapInterval(1);
#endif

    return 0;
}
