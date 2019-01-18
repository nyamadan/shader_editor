#include <stdio.h>
#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <ctime>

#include <stb_image_write.h>

#include <imgui.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>

#include "common.hpp"
#include "shader_utils.hpp"

namespace {
const char *const DefaultVertexShader = "assets/default.vert";
const char *const DefaultFragmentShader = "assets/default.frag";

int windowWidth = 1024;
int windowHeight = 768;

int bufferWidth;
int bufferHeight;

int positionLocation = 0;

GLFWwindow *window = nullptr;

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

const char *currentVS = DefaultVertexShader;
const char *currentFS = DefaultFragmentShader;

const GLfloat positions[] = {-1.0f, 1.0f,  0.0f, 1.0f, 1.0f,  0.0f,
                             -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f};

const GLushort indices[] = {0, 2, 1, 1, 2, 3};

void glfw_error_callback(int error, const char *description) {
    std::cerr << "error " << error << ": " << description << std::endl;
}

void OnGUI() {
    static bool showDemoWindow = false;
    static bool showDebugWindow = true;

    // if (ImGui::BeginMainMenuBar()) {
    //    if (ImGui::BeginMenu("File")) {
    //        if (ImGui::MenuItem("New")) {
    //        }
    //        if (ImGui::MenuItem("Open", "Ctrl+O")) {
    //        }
    //        ImGui::Separator();
    //        if (ImGui::MenuItem("Quit", "Alt+F4")) {
    //        }
    //        ImGui::EndMenu();
    //    }
    //    ImGui::EndMainMenuBar();
    //}

    if (showDebugWindow) {
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);

        ImGui::Checkbox("Demo Window",
                        &showDemoWindow);  // Edit bools storing our windows
                                           // open/close state
    }

    if (showDemoWindow) {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }
}

bool readText(char *&memblock, const char *const path) {
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
    readText(vsSource, vsPath);

    const GLuint handleVS = glCreateShader(GL_VERTEX_SHADER);
    // const char *const vsSources[2] = {GlslVersion, vsSource};
    // glShaderSource(handleVS, 2, vsSources, NULL);
    glShaderSource(handleVS, 1, &vsSource, NULL);
    glCompileShader(handleVS);
    delete vsSource;
    if (!checkCompiled(handleVS)) {
        return 0;
    }

    char *fsSource = nullptr;
    readText(fsSource, fsPath);

    const GLuint handleFS = glCreateShader(GL_FRAGMENT_SHADER);
    // const char *const fsSources[2] = {GlslVersion, fsSource};
    // glShaderSource(handleFS, 2, fsSources, NULL);
    glShaderSource(handleFS, 1, &fsSource, NULL);
    glCompileShader(handleFS);
    delete fsSource;
    if (!checkCompiled(handleFS)) {
        return 0;
    }

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
    clock_t now;

    // check update.
    now = std::clock();
    if (now - lastCheckUpdate > CheckInterval) {
        struct stat stVS;
        struct stat stFS;

        stat(currentVS, &stVS);
        stat(currentFS, &stFS);

        if (stFS.st_mtime != lastMTimeFS || stVS.st_mtime != lastMTimeVS) {
            auto newProgram = linkProgram(currentVS, currentFS);
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

    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
    glfwGetCursorPos(window, &xpos, &ypos);

    if (currentWidth != bufferWidth || currentHeight != bufferHeight) {
        windowWidth = currentWidth;
        windowHeight = currentHeight;
        updateFrameBuffers(currentWidth, currentHeight);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    OnGUI();

    ImGui::Render();

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffers[0]);

    glViewport(0, 0, windowWidth, windowHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);

    if (uResolution >= 0) {
        glUniform2f(uResolution, static_cast<GLfloat>(bufferWidth),
                    static_cast<GLfloat>(bufferHeight));
    }

    if (uMouse >= 0) {
        glUniform2f(uMouse, static_cast<GLfloat>(xpos) / bufferWidth,
                    1.0f - static_cast<GLfloat>(ypos) / bufferHeight);
    }

    if (uTime >= 0) {
        glUniform1f(uTime, static_cast<GLfloat>(now) * 0.001f);
    }

    glBindVertexArray(vertexArraysObject);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwMakeContextCurrent(window);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, backBuffers[0]);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, bufferWidth, bufferHeight, 0, 0, windowWidth,
                      windowHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

    glfwSwapBuffers(window);

    for (GLint error = glGetError(); error; error = glGetError()) {
        std::cerr << "error code: " << std::hex << std::showbase << error
                  << std::endl;
    }

    glfwPollEvents();
}

}  // namespace

int main(void) {
    /* Initialize the library */
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

#ifdef _DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
#endif

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(windowWidth, windowHeight, "Shader Editor", NULL,
                              NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

#ifndef __EMSCRIPTEN__
    // Initialize OpenGL loader
    if (gl3wInit() != 0) {
        return 1;
    }
#endif

    ImGui::SetCurrentContext(ImGui::CreateContext());
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(GlslVersion);

    glfwMakeContextCurrent(window);

    // Compile shaders.
    program = linkProgram(currentVS, currentFS);
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
    stat(currentVS, &st);
    lastMTimeVS = st.st_mtime;
    stat(currentFS, &st);
    lastMTimeFS = st.st_mtime;

#ifndef __EMSCRIPTEN__
    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(window)) {
        update(nullptr);
    }

    glfwTerminate();
#else
    emscripten_set_main_loop_arg(update, nullptr, 0, 0);
    glfwSwapInterval(1);
#endif

    return 0;
}
