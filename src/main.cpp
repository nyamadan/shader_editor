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
const int PositionLocation = 0;
const int PositionSize = 3;

GLFWwindow *window = nullptr;

GLuint vIndex = 0;
GLuint vPosition = 0;
GLuint vertexArraysObject = 0;

GLint uTime = 0;
GLint uMouse = 0;
GLint uResolution = 0;

GLuint program = 0;

time_t lastMTime = 0;

const clock_t CheckInterval = 500;
clock_t lastCheckUpdate = 0;

const GLfloat positions[] = {-1.0f, 1.0f,  0.0f, 1.0f, 1.0f,  0.0f,
                             -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f};

const GLushort indices[] = {0, 2, 1, 1, 2, 3};

void glfw_error_callback(int error, const char *description) {
    std::cerr << "error " << error << ": " << description << std::endl;
}

void OnGUI() {
    static bool showDemoWindow = false;
    static bool showDebugWindow = true;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New")) {
            }
            if (ImGui::MenuItem("Open", "Ctrl+O")) {
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Quit", "Alt+F4")) {
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

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

void update(void *) {
    static int width, height;
    static double xpos, ypos;

    // check update.
    auto now = std::clock();
    if (now - lastCheckUpdate > CheckInterval) {
        struct stat st;

        stat("./assets/basic.frag", &st);
        if (st.st_mtime != lastMTime) {
            std::cout << st.st_mtime << std::endl;
            lastMTime = st.st_mtime;
        }
        lastCheckUpdate = now;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    OnGUI();

    ImGui::Render();

    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &width, &height);
    glfwGetCursorPos(window, &xpos, &ypos);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);

    if (uResolution >= 0) {
        glUniform2f(uResolution, static_cast<GLfloat>(width),
                    static_cast<GLfloat>(height));
    }

    if (uMouse >= 0) {
        glUniform2f(uMouse, static_cast<GLfloat>(xpos),
                    static_cast<GLfloat>(ypos));
    }

    glBindVertexArray(vertexArraysObject);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwMakeContextCurrent(window);
    glfwSwapBuffers(window);

    for (GLint error = glGetError(); error; error = glGetError()) {
        std::cerr << "error code: " << std::hex << std::showbase << error
                  << std::endl;
    }

    glfwPollEvents();
}

static void readText(char *&memblock, const char *const path) {
    std::ifstream file(path, std::ios::in | std::ios::ate);

    if (!file.is_open()) {
        return;
    }

    const size_t size = static_cast<size_t>(file.tellg());
    memblock = new char[size + 1];
    file.seekg(0, std::ios::beg);
    file.read(memblock, size);
    memblock[size] = '\0';
    file.close();
}
}  // namespace

int main(void) {
    const int Width = 1024;
    const int Height = 768;
    const int Step = 10;

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

    window = glfwCreateWindow(Width, Height, "ShaderEditor", NULL, NULL);
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

    // Initialize Buffers
    glGenVertexArrays(1, &vertexArraysObject);
    glGenBuffers(1, &vIndex);
    glGenBuffers(1, &vPosition);

    glBindVertexArray(vertexArraysObject);
    glBindBuffer(GL_ARRAY_BUFFER, vPosition);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 12, positions,
                 GL_STATIC_DRAW);
    glVertexAttribPointer(PositionLocation, PositionSize, GL_FLOAT, GL_FALSE, 0,
                          0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vIndex);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * 6, indices,
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    // Compile shaders.
    char *vsSource = nullptr;

    readText(vsSource, "./assets/basic.vert");

    char *fsSource = nullptr;

    readText(fsSource, "./assets/basic.frag");

    const char *const vsSources[2] = {GlslVersion, vsSource};
    const char *const fsSources[2] = {GlslVersion, fsSource};

    const GLuint handleVS = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(handleVS, 2, vsSources, NULL);
    glCompileShader(handleVS);
    assert(checkCompiled(handleVS));

    const GLuint handleFS = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(handleFS, 2, fsSources, NULL);
    glCompileShader(handleFS);
    assert(checkCompiled(handleFS));

    program = glCreateProgram();
    glAttachShader(program, handleVS);
    glAttachShader(program, handleFS);
    glLinkProgram(program);
    assert(checkLinked(program));

    uTime = glGetUniformLocation(program, "time");
    uMouse = glGetUniformLocation(program, "mouse");
    uResolution = glGetUniformLocation(program, "resolution");

    glDeleteShader(handleVS);
    glDeleteShader(handleFS);

    delete[] vsSource;
    delete[] fsSource;

    // check update.
    struct stat st;
    stat("./assets/basic.frag", &st);
    lastMTime = st.st_mtime;

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
