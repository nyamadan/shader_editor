#include <stdio.h>
#include <sys/stat.h>

#include <fstream>
#include <string>
#include <iostream>
#include <ctime>

#include <stb_image_write.h>

#include <imgui.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "common.hpp"
#include "shader_utils.hpp"
#include "app_log.hpp"

namespace {
const char *const DefaultAssetPath = "default";
const char *const DefaultVertexShaderName = "vert.glsl";
const char *const DefaultFragmentShaderName = "frag.glsl";

std::string assetPath;
std::string currentVS;
std::string currentFS;

int windowWidth = 1024;
int windowHeight = 768;

int bufferWidth;
int bufferHeight;

int positionLocation = 0;

GLFWwindow *mainWindow = nullptr;

GLuint vIndex = 0;
GLuint vPosition = 0;
GLuint vertexArraysObject = 0;

GLint uTime = 0;
GLint uMouse = 0;
GLint uResolution = 0;
GLuint program = 0;

GLuint frameBuffers[2];
GLuint depthBuffers[2];
GLuint backBuffers[2];

const clock_t CheckInterval = 500;
clock_t lastCheckUpdate = 0;
time_t lastMTimeVS = 0;
time_t lastMTimeFS = 0;
time_t timeStart = 0;

const GLfloat positions[] = {-1.0f, 1.0f,  0.0f, 1.0f, 1.0f,  0.0f,
                             -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f};

const GLushort indices[] = {0, 2, 1, 1, 2, 3};

void glfwErrorCallback(int error, const char *description) {
    GetAppLog().AddLog("error %d: %s\n", error, description);
}

bool ReadText(char *&memblock, const char *const path) {
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        return false;
    }

    const size_t size = static_cast<size_t>(file.tellg());
    memblock = new char[size + 1];
    file.seekg(0, std::ios::beg);
    file.read(memblock, size);
    memblock[size] = '\0';
    file.close();

    return true;
}

GLuint linkProgram(const char *const vsPath, const char *const fsPath) {
    char *vsSource = nullptr;
    ReadText(vsSource, vsPath);

    const GLuint handleVS = glCreateShader(GL_VERTEX_SHADER);
    // const char *const vsSources[2] = {GlslVersion, vsSource};
    // glShaderSource(handleVS, 2, vsSources, NULL);
    glShaderSource(handleVS, 1, &vsSource, NULL);
    glCompileShader(handleVS);
    if (!checkCompiled(handleVS, vsPath)) {
        delete vsSource;
        return 0;
    }
    delete vsSource;

    char *fsSource = nullptr;
    ReadText(fsSource, fsPath);

    const GLuint handleFS = glCreateShader(GL_FRAGMENT_SHADER);
    // const char *const fsSources[2] = {GlslVersion, fsSource};
    // glShaderSource(handleFS, 2, fsSources, NULL);
    glShaderSource(handleFS, 1, &fsSource, NULL);
    glCompileShader(handleFS);
    if (!checkCompiled(handleFS, fsPath)) {
        delete fsSource;
        return 0;
    }
    delete fsSource;

    const GLuint program = glCreateProgram();
    glAttachShader(program, handleVS);
    glAttachShader(program, handleFS);
    glLinkProgram(program);

    if (!checkLinked(program)) {
        return 0;
    }

    positionLocation = glGetAttribLocation(program, "aPosition");

    uTime = glGetUniformLocation(program, "time");
    uMouse = glGetUniformLocation(program, "mouse");
    uResolution = glGetUniformLocation(program, "resolution");

    glDeleteShader(handleVS);
    glDeleteShader(handleFS);

    return program;
}

void updateFrameBuffers(GLuint width, GLuint height) {
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
}

void update(void *) {
    int currentWidth, currentHeight;
    double xpos, ypos;

    // ImGUI
    static bool showImGuiDemoWindow = false;
    static bool showDebugWindow = true;
    static bool showAppLogWindow = false;

    static bool uiPlaying = true;
    static bool uiSyncWindow = true;
    static float uiTimeValue = 0;

    // Uniforms
    float uTimeValue;
    glm::vec2 uResolutionValue;
    glm::vec2 uMouseValue;

    // check update.
    clock_t now = std::clock();
    if (now - lastCheckUpdate > CheckInterval) {
        struct stat stVS;
        struct stat stFS;

        stat(currentVS.c_str(), &stVS);
        stat(currentFS.c_str(), &stFS);

        if (stFS.st_mtime != lastMTimeFS || stVS.st_mtime != lastMTimeVS) {
            GLuint newProgram =
                linkProgram(currentVS.c_str(), currentFS.c_str());
            if (newProgram) {
                glUseProgram(newProgram);
                glDeleteProgram(program);
                program = newProgram;

                glBindVertexArray(vertexArraysObject);
                glBindBuffer(GL_ARRAY_BUFFER, vPosition);
                glVertexAttribPointer(positionLocation, 3, GL_FLOAT, GL_FALSE,
                                      0, 0);
                glBindVertexArray(0);
            }
            lastMTimeVS = stVS.st_mtime;
            lastMTimeFS = stFS.st_mtime;
        }

        lastCheckUpdate = now;
    }

    glfwMakeContextCurrent(mainWindow);
    glfwGetFramebufferSize(mainWindow, &currentWidth, &currentHeight);
    glfwGetCursorPos(mainWindow, &xpos, &ypos);

    // update buffer width
    if (uiSyncWindow) {
        if (currentWidth != bufferWidth || currentHeight != bufferHeight) {
            updateFrameBuffers(currentWidth, currentHeight);
        }
    }

    windowWidth = currentWidth;
    windowHeight = currentHeight;

    // uniform values
    uTimeValue = static_cast<GLfloat>(now - timeStart) * 0.001f;
    uResolutionValue.x = static_cast<GLfloat>(bufferWidth);
    uResolutionValue.y = static_cast<GLfloat>(bufferHeight);
    uMouseValue.x = static_cast<GLfloat>(xpos) / bufferWidth;
    uMouseValue.y = 1.0f - static_cast<GLfloat>(ypos) / bufferHeight;

    if (!uiPlaying) {
        timeStart = now - static_cast<clock_t>(uiTimeValue * 1000.0f);
        uTimeValue = uiTimeValue;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (showDebugWindow) {
        if (ImGui::CollapsingHeader("Stats")) {
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                        1000.0f / ImGui::GetIO().Framerate,
                        ImGui::GetIO().Framerate);
        }

        if (ImGui::CollapsingHeader("Uniforms")) {
            ImGui::Text("time: %.3f", uTimeValue);
            ImGui::Text("resolution: %.1f, %.1f", uResolutionValue.x,
                        uResolutionValue.y);
            ImGui::Text("mouse: %.3f, %.3f", uMouseValue.x, uMouseValue.y);
        }

        if (ImGui::CollapsingHeader("Time")) {
            ImGui::Text("Time:");
            if (uiPlaying) {
                ImGui::Text("%.3f sec", uTimeValue);
            } else {
                ImGui::PushItemWidth(200.0f);
                ImGui::DragFloat("sec", &uiTimeValue, 0.5f);
                ImGui::PopItemWidth();
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
            ImGui::Checkbox("Sync", &uiSyncWindow);
        }

        if (ImGui::CollapsingHeader("Window")) {
            ImGui::Checkbox("Log",
                            &showAppLogWindow);  // Edit bools storing our
                                                 // windows open/close state

            ImGui::Checkbox("ImGui Demo Window",
                            &showImGuiDemoWindow);  // Edit bools storing our
                                                    // windows open/close state
        }
    }

    if (showAppLogWindow) {
        ShowAppLogWindow(&showAppLogWindow);
    }

    if (showImGuiDemoWindow) {
        ImGui::ShowDemoWindow(&showImGuiDemoWindow);
    }

    ImGui::Render();

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[0]);

    glViewport(0, 0, bufferWidth, bufferHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);

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

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, backBuffers[0]);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, bufferWidth, bufferHeight, 0, 0, windowWidth,
                      windowHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwMakeContextCurrent(mainWindow);

    glfwSwapBuffers(mainWindow);

    for (GLint error = glGetError(); error; error = glGetError()) {
        GetAppLog().AddLog("error code: %0X\n", error);
    }

    glfwPollEvents();
}

}  // namespace

int main(void) {
    currentVS = DefaultAssetPath;
#ifdef WIN32
    if (currentVS.back() != '\\' && currentVS.back() != '/') {
        currentVS.append("\\");
    }
#else
    if (currentVS.back() != '/') {
        currentVS.append("/");
    }
#endif
    currentVS.append(DefaultVertexShaderName);

    currentFS = DefaultAssetPath;
    std::cout << currentFS.back();
#ifdef WIN32
    if (currentFS.back() != '\\' && currentFS.back() != '/') {
        currentFS.append("\\");
    }
#else
    if (currentFS.back() != '/') {
        currentFS.append("/");
    }
#endif
    currentFS.append(DefaultFragmentShaderName);

    if (false) {
        FILE *fp = fopen("video.y4m", "wb");
        fprintf(fp,
                "YUV4MPEG2 W480 H270 F30000:1001 Ip A1:1 C444 XYSCSS=444\n");
        for (int i = 0; i < 60; i++) {
            fprintf(fp, "FRAME\n");

            uint8_t r = 0xff;
            uint8_t g = 0x00;
            uint8_t b = 0x00;

            for (int x = 0; x < 480; x++) {
                for (int y = 0; y < 270; y++) {
                    uint8_t Y = 0.299 * r + 0.587 * g + 0.114 * b;
                    fwrite(&Y, sizeof(uint8_t), 1, fp);
                }
            }

            for (int x = 0; x < 480; x++) {
                for (int y = 0; y < 270; y++) {
                    uint8_t U = -0.169 * r - 0.331 * g + 0.5 * b + 128;
                    fwrite(&U, sizeof(uint8_t), 1, fp);
                }
            }

            for (int x = 0; x < 480; x++) {
                for (int y = 0; y < 270; y++) {
                    uint8_t V = 0.5 * r - 0.419 * g - 0.081 * b + 128;
                    fwrite(&V, sizeof(uint8_t), 1, fp);
                }
            }
        }
        fclose(fp);
    }
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
    program = linkProgram(currentVS.c_str(), currentFS.c_str());
    assert(program);

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

    // check update.
    struct stat st;
    stat(currentVS.c_str(), &st);
    lastMTimeVS = st.st_mtime;
    stat(currentFS.c_str(), &st);
    lastMTimeFS = st.st_mtime;

    timeStart = std::clock();

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
