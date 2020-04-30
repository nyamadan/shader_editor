#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <memory>
#include <filesystem>

#include <imgui.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>
#include <TextEditor.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "app.hpp"

#include "file_utils.hpp"
#include "platform.hpp"
#include "shader_utils.hpp"
#include "shader_program.hpp"
#include "app_log.hpp"
#include "image.hpp"
#include "recording.hpp"
#include "buffers.hpp"
#include "shader_files.hpp"
#include "default_shader.hpp"

namespace fs = std::filesystem;

namespace {
void glfwErrorCallback(int error, const char* description) {
    AppLog::getInstance().error("GLFW_ERROR %d: %s\n", error, description);
}

#ifndef __EMSCRIPTEN__
void glDebugOutput(GLenum source, GLenum type, GLuint eid, GLenum severity,
                   GLsizei length, const GLchar* message,
                   const void* user_param) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            AppLog::getInstance().debug("GL_DEBUG_SEVERITY_DEBUG(%X): %s\n", eid,
                                        message);
            break;
        case GL_DEBUG_SEVERITY_LOW:
            AppLog::getInstance().error("GL_DEBUG_SEVERITY_LOW(%X): %s\n", eid,
                                        message);
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            AppLog::getInstance().error("GL_DEBUG_SEVERITY_MEDIUM(%X): %s\n",
                                        eid, message);
            break;
        case GL_DEBUG_SEVERITY_HIGH:
            AppLog::getInstance().error("GL_DEBUG_SEVERITY_HIGH(%X): %s\n", eid,
                                        message);
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

namespace shader_editor {
UniformNames App::getUniformNames(ShaderPlatform platform) {
    UniformNames uNames;
    switch (platform) {
        case GLSL_SANDBOX:
            uNames.time = "time";
            uNames.resolution = "resolution";
            uNames.mouse = "mouse";
            uNames.backbuffer = "backbuffer";
            uNames.frame = "";
            break;
        case SHADER_TOY:
            uNames.time = "iTime";
            uNames.resolution = "iResolution";
            uNames.mouse = "iMouse";
            uNames.frame = "iFrame";
            uNames.backbuffer = "";
            break;
    }

    return uNames;
}

std::shared_ptr<ShaderProgram> App::refreshShaderProgram(float now,
                                                         int32_t& cursorLine) {
    std::shared_ptr<ShaderProgram> newProgram =
        std::make_shared<ShaderProgram>();

    newProgram->setCompileInfo(
        "<default-vertex-shader>", "<default-fragment-shader>",
        DefaultVertexShaderSource, DefaultFragmentShaderSource, 0, 0);

    if (now - lastCheckUpdate > CheckInterval) {
        if (program->checkExpiredWithReset()) {
            if (uiDebugWindow && uiShowTextEditor) {
                std::string text;
                readText(program->getFragmentShader().getPath(), text);
                cursorLine = editor.GetCursorPosition().mLine;
                editor.SetText(text);
                editor.SetCursorPosition(TextEditor::Coordinates());
            } else {
                if (uiShaderPlatformIndex == SHADER_TOY) {
                    newProgram->setFragmentShaderSourceTemplate(
                        ShaderToyTemplate);
                } else {
                    newProgram->setFragmentShaderSourceTemplate(std::string());
                }

                recompileShaderFromFile(program, newProgram);

                programErrors = newProgram->getFragmentShader().getErrors();
                if (programErrors.begin() != programErrors.end()) {
                    cursorLine = programErrors.begin()->getLineNumber() - 1;
                }

                shaderFiles.replaceNewProgram(uiShaderFileIndex, newProgram);

                SetProgramErrors(programErrors, this->program);
            }
        }

        lastCheckUpdate = now;
    }

    return newProgram;
}

void App::update(void*) {
    std::map<std::string, std::shared_ptr<Image>> usedTextures;

    int currentWidth, currentHeight;
    float bufferScale =
        1.0f / powf(2.0f, static_cast<float>(uiBufferQualityIndex - 1));

    int32_t cursorLine = -1;

    float now = static_cast<float>(ImGui::GetTime());

    auto uNames = getUniformNames(uiShaderPlatformIndex);

    glfwMakeContextCurrent(mainWindow);
    glfwGetFramebufferSize(mainWindow, &currentWidth, &currentHeight);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    auto newProgram = refreshShaderProgram(now, cursorLine);

    if (uiShowTextEditor && editor.IsTextChanged()) {
        needRecompile = true;
        lastTextEdited = now;
    }

    if(needRecompile && now > lastTextEdited + recompileDelay){
        needRecompile = false;

        if (uiShaderPlatformIndex == SHADER_TOY) {
            newProgram->setFragmentShaderSourceTemplate(ShaderToyTemplate);
        } else {
            newProgram->setFragmentShaderSourceTemplate(std::string());
        }

        recompileFragmentShader(program, newProgram, editor.GetText());
        programErrors = newProgram->getFragmentShader().getErrors();
        if (editor.IsReadOnly() &&
            programErrors.begin() != programErrors.end()) {
            cursorLine = programErrors.begin()->getLineNumber() - 1;
        }

        shaderFiles.replaceNewProgram(uiShaderFileIndex, newProgram);

        SetProgramErrors(programErrors, this->program);
    }

    if (newProgram->isOK()) {
        swapProgram(newProgram);

        needRecompile = false;
    }

    const bool* const mouseDown = ImGui::GetIO().MouseDown;
    const ImVec2& mousePos =
        ImGui::IsMousePosValid() ? ImGui::GetMousePos() : ImVec2(0.5f, 0.5f);

    // onResizeWindow
    if (!recording->getIsRecording() &&
        (currentWidth != windowWidth || currentHeight != windowHeight)) {
        buffers.updateFrameBuffersSize(
            static_cast<GLint>(currentWidth * bufferScale),
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

    getUsedTextures(uNames, usedTextures);

    if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)) &&
        !recording->getIsRecording()) {
        uiDebugWindow = !uiDebugWindow;
    }

    if (uiDebugWindow) {
        ImGui::Begin("Control", &uiDebugWindow,
                     ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Checkbox("File", &uiShaderFileWindow);
        ImGui::Checkbox("Time", &uiTimeWindow);
        ImGui::Checkbox("Stats", &uiStatsWindow);
        ImGui::Checkbox("Uniforms", &uiUniformWindow);
        ImGui::Checkbox("Buffer Size", &uiBackBufferWindow);
        ImGui::Checkbox("Save as Video", &uiCaptureWindow);
        ImGui::Checkbox("Errors", &uiErrorWindow);
        ImGui::Checkbox("Log", &uiAppLogWindow);

#ifndef NDEBUG
        ImGui::Checkbox("ImGui Demo Window", &showImGuiDemoWindow);
#endif

        ImGui::End();

        if (uiShaderFileWindow) {
            onUiShaderFileWindow(cursorLine);
        }

        if (uiTimeWindow) {
            onUiTimeWindow(now);
        }

        if (uiCaptureWindow) {
            onUiCaptureWindow();
        }

        if (uiStatsWindow) {
            onUiStatsWindow();
        }

        if (uiUniformWindow) {
            onUiUniformWindow(uNames, usedTextures);
        }

        if (uiBackBufferWindow) {
            onUiBackBufferWindow(bufferScale, currentWidth, currentHeight);
        }

        if (uiErrorWindow) {
            onUiErrorWindow();
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
    program->setUniformValue(
        uNames.resolution,
        glm::vec2(static_cast<GLfloat>(buffers.getWidth()),
                  static_cast<GLfloat>(buffers.getHeight())));

    switch (uiShaderPlatformIndex) {
        case 0:
            if (recording->getIsRecording()) {
                program->setUniformValue(uNames.mouse, glm::vec2(0.5f, 0.5f));
            } else {
                program->setUniformValue(
                    uNames.mouse, glm::vec2(mousePos.x / windowWidth,
                                            1.0f - mousePos.y / windowHeight));
            }
            break;
        case 1:
            program->setUniformValue(uNames.frame,
                                     static_cast<int>(currentFrame));

            if (recording->getIsRecording()) {
                program->setUniformValue(
                    uNames.mouse, glm::vec4(0.5f * windowWidth,
                                            0.5f * windowHeight, 0.0f, 0.0f));
            } else {
                if (mouseDown[0] || mouseDown[1]) {
                    program->setUniformValue(
                        uNames.mouse,
                        glm::vec4(mousePos.x, windowHeight - mousePos.y,
                                  mouseDown[0] ? 1.0f : 0.0f,
                                  mouseDown[1] ? 1.0f : 0.0f));
                } else {
                    if (program->containsUniform(uNames.mouse)) {
                        const glm::vec4& prevMousePos =
                            program->getUniform(uNames.mouse).value.vec4;
                        program->setUniformValue(
                            uNames.mouse,
                            glm::vec4(prevMousePos.x, prevMousePos.y, 0, 0));
                    }
                }
            }
            break;
    }

    program->setUniformValue(uNames.time, uiTimeValue);

    if (program->isOK()) {
        glUseProgram(program->getProgram());

        int32_t channel = 0;

        for (auto iter = usedTextures.begin(); iter != usedTextures.end();
             iter++) {
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
        program->setUniformValue(uNames.backbuffer, channel++);
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

        if (isLastFrame) {
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
        AppLog::getInstance().debug("error code: 0x%0X\n", error);
    }

    glfwPollEvents();
}

void App::startRecord(const std::string& fileName,
                      const int32_t uiVideoTypeIndex,
                      const int32_t uiVideoResolution, const int32_t kbps,
                      unsigned long encodeDeadline, float& uiTimeValue) {
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

void App::SetProgramErrors(const std::vector<CompileError>& programErrors,
                           std::shared_ptr<ShaderProgram> program) {
    std::map<int32_t, std::string> markers;
    for (auto it = programErrors.cbegin(); it != programErrors.cend(); it++) {
        const auto lineNumber = it->getLineNumber();
        if (lineNumber < 0) {
            continue;
        }

        if (fs::path(program->getFragmentShader().getPath())
                .compare(it->getFileName()) != 0) {
            continue;
        }

        if (markers.count(lineNumber) != 0) {
            markers[lineNumber] += "\n" + it->getOriginal();
        } else {
            markers[lineNumber] += it->getOriginal();
        }
    }
    editor.SetErrorMarkers(markers);
}

void App::swapProgram(std::shared_ptr<ShaderProgram> newProgram) {
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

void App::ShowTextEditor(bool& showTextEditor, int32_t& uiShader,
                         int32_t uiShaderPlatformIndex) {
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

                        if (uiShaderPlatformIndex == SHADER_TOY) {
                            newProgram->setFragmentShaderSourceTemplate(
                                ShaderToyTemplate);
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

                        uiShader = shaderFiles.pushNewProgram(newProgram);

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

#ifdef __EMSCRIPTEN__
    std::string clipboardText = ImGui::GetClipboardText();
    if (!editor.IsReadOnly() && !clipboardText.empty()) {
        editor.Paste();
    }
#endif

    ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1,
                cpos.mColumn + 1, editor.GetTotalLines(),
                editor.IsOverwrite() ? "Ovr" : "Ins",
                editor.CanUndo() ? "*" : " ",
                editor.GetLanguageDefinition().mName.c_str(),
                program->getFragmentShader().getPath().c_str());

    editor.Render("TextEditor");
    ImGui::End();
}

int32_t App::start(const std::string& assetPath, bool alwaysOnTop) {
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

    if (alwaysOnTop) {
        glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    }

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

    // Compile shaders.
    program.reset(new ShaderProgram());
    program->compile("<default-vertex-shader>", "<default-fragment-shader>",
                     DefaultVertexShaderSource, DefaultFragmentShaderSource, -1,
                     -1);
    assert(program->isOK());

    copyProgram.reset(new ShaderProgram());
    copyProgram->compile("<default-vertex-shader>", "<default-copy-shader>",
                         DefaultVertexShaderSource, DefaultCopyShaderSource, -1,
                         -1);
    assert(copyProgram->isOK());

    const GLfloat positions[] = {-1.0f, 1.0f,  0.0f, 1.0f, 1.0f,  0.0f,
                                 -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f};

    const GLushort indices[] = {0, 2, 1, 1, 2, 3};

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
    shaderFiles.pushNewProgram(program);

    auto path = fs::path(assetPath);
    if (path.extension().empty()) {
        shaderFiles.loadFiles(assetPath);
    } else {
        shaderFiles.loadFiles(path.parent_path().string());

        const auto len = shaderFiles.getNumShaderFileNames();
        const auto fileNames = shaderFiles.getShaderFileNames();

        for (auto i = 0; i < len; i++) {
            if (path.compare(fileNames[i]) !=  0) {
                continue;
            }

            uiShaderFileIndex = i;

            std::shared_ptr<ShaderProgram> newProgram =
                std::make_shared<ShaderProgram>();

            if (uiShaderPlatformIndex == SHADER_TOY) {
                newProgram->setFragmentShaderSourceTemplate(ShaderToyTemplate);
            }

            compileShaderFromFile(newProgram,
                                  program->getVertexShader().getPath(),
                                  path.string());
            editor.SetText(newProgram->getFragmentShader().getSource());
            editor.SetCursorPosition(TextEditor::Coordinates());
            programErrors = newProgram->getFragmentShader().getErrors();
            SetProgramErrors(programErrors, this->program);
            program.swap(newProgram);
            break;
        }
    }

    timeStart = static_cast<float>(ImGui::GetTime());

    auto lang = TextEditor::LanguageDefinition::GLSL();
    editor.SetLanguageDefinition(lang);
    editor.SetText(program->getFragmentShader().getSource());

    return 0;
}

void App::cleanup() {
    shaderFiles.deleteImageFileNames();
    shaderFiles.deleteShaderFileNamse();

    recording->cleanup();

    h264encoder::UnloadEncoderLibrary();
}

void App::getUsedTextures(
    const UniformNames& uNames,
    std::map<std::string, std::shared_ptr<Image>>& usedTextures) {
    usedTextures.clear();
    if (shaderFiles.getNumImageFileNames() > 0) {
        auto& uniforms = program->getUniforms();
        for (auto it = uniforms.begin(); it != uniforms.end(); it++) {
            ShaderUniform& u = it->second;

            if (u.type == UniformType::Sampler2D &&
                u.name != uNames.backbuffer) {
                int32_t& uiImage = imageUniformNameToIndex[u.name];

                if (uiImage < 0 || uiImage >= shaderFiles.getNumImageFileNames()) {
                    uiImage = 0;
                }

                usedTextures[u.name] = shaderFiles.getImage(uiImage);
            }
        }
    }
}

GLFWwindow* App::getMainWindow() { return mainWindow; }

void App::onUiCaptureWindow() {
    unsigned long encodeDeadline = VPX_DL_GOOD_QUALITY;

    ImGui::Begin("Export Video", &uiCaptureWindow,
                 ImGuiWindowFlags_AlwaysAutoResize);
    const char* resolutionItems[] = {"256x144",   "427x240",  "640x360",
                                     "720x480",   "1280x720", "1920x1080",
                                     "2560x1440", "3840x2160"};

    const char* typeItems[] = {"I420", "WebM", "MP4"};

    ImGui::Combo("resolution", &uiVideoResolutionIndex, resolutionItems,
                 IM_ARRAYSIZE(resolutionItems));

    ImGui::Combo("format", &uiVideoTypeIndex, typeItems,
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
            startRecord(fileName, uiVideoTypeIndex, uiVideoResolutionIndex,
                        static_cast<int32_t>(uiVideoMbps * 1000.0f),
                        encodeDeadline, uiTimeValue);
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

void App::onUiStatsWindow() {
    ImGui::Begin("Stats", &uiStatsWindow, ImGuiWindowFlags_AlwaysAutoResize);

    std::stringstream ss;

    ss.str(std::string());
    ss << 1000.0f / ImGui::GetIO().Framerate;
    ImGui::LabelText("ms/frame", "%s", ss.str().c_str());

    ss.str(std::string());
    ss << ImGui::GetIO().Framerate;
    ImGui::LabelText("fps", "%s", ss.str().c_str());
    ImGui::End();
}

void App::onUiTimeWindow(float now) {
    ImGui::Begin("Time", &uiTimeWindow, ImGuiWindowFlags_AlwaysAutoResize);
    if (uiPlaying) {
        ImGui::LabelText("time", "%s", std::to_string(uiTimeValue).c_str());
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

void App::onUiBackBufferWindow(float& bufferScale, int32_t& currentWidth,
                               int32_t& currentHeight) {
    ImGui::Begin("Buffer Size", &uiBackBufferWindow,
                 ImGuiWindowFlags_AlwaysAutoResize);
    const char* const items[] = {"0.5", "1", "2", "4", "8"};
    if (ImGui::Combo("quality", &uiBufferQualityIndex, items,
                     IM_ARRAYSIZE(items))) {
        // update framebuffers
        bufferScale =
            1.0f / powf(2.0f, static_cast<float>(uiBufferQualityIndex - 1));
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

void App::onUiUniformWindow(
    const UniformNames& uNames,
    std::map<std::string, std::shared_ptr<Image>>& usedTextures) {
    ImGui::Begin("Uniforms", &uiUniformWindow,
                 ImGuiWindowFlags_AlwaysAutoResize);
    std::map<const std::string, ShaderUniform>& uniforms =
        program->getUniforms();
    for (auto it = uniforms.begin(); it != uniforms.end(); it++) {
        ShaderUniform& u = it->second;
        if (u.location < 0) {
            continue;
        }

        if (u.name == uNames.time || u.name == uNames.resolution ||
            u.name == uNames.mouse || u.name == uNames.frame ||
            u.name == uNames.backbuffer) {
            ImGui::LabelText(u.name.c_str(), "%s", u.toString().c_str());
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
                if (shaderFiles.getNumImageFileNames() > 0) {
                    int32_t& uiImage = imageUniformNameToIndex[u.name];

                    if (ImGui::Combo(u.name.c_str(), &uiImage,
                                     shaderFiles.getImageFileNames(),
                                     shaderFiles.getNumImageFileNames())) {
                        usedTextures[u.name] = shaderFiles.getImage(uiImage);
                    }
                }
                break;
            case UniformType::Vector1:
                break;
            case UniformType::Vector2:
                ImGui::DragFloat2(u.name.c_str(), glm::value_ptr(u.value.vec2));
                break;
            case UniformType::Vector3:
                ImGui::DragFloat3(u.name.c_str(), glm::value_ptr(u.value.vec3));
                break;
            case UniformType::Vector4:
                ImGui::DragFloat4(u.name.c_str(), glm::value_ptr(u.value.vec4));
                break;
        }
    }
    ImGui::End();
}

void App::onUiShaderFileWindow(int32_t& cursorLine) {
    ImGui::Begin("File", &uiShaderFileWindow,
                 ImGuiWindowFlags_AlwaysAutoResize);

    const char* const items[] = {"glslsandbox", "shadertoy"};
    if (ImGui::Combo("platform", (int*)&uiShaderPlatformIndex, items,
                     IM_ARRAYSIZE(items))) {
        std::shared_ptr<ShaderProgram> newProgram =
            std::make_shared<ShaderProgram>();

        if (uiShaderPlatformIndex == SHADER_TOY) {
            newProgram->setFragmentShaderSourceTemplate(ShaderToyTemplate);
        } else {
            newProgram->setFragmentShaderSourceTemplate(std::string());
        }

        recompileFragmentShader(program, newProgram, editor.GetText());
        programErrors = newProgram->getFragmentShader().getErrors();
        if (programErrors.begin() != programErrors.end()) {
            cursorLine = programErrors.begin()->getLineNumber() - 1;
        }
        shaderFiles.replaceNewProgram(uiShaderFileIndex, newProgram);

        SetProgramErrors(programErrors, newProgram);

        swapProgram(newProgram);

        needRecompile = false;
    }

    if (ImGui::Combo("File", &uiShaderFileIndex, shaderFiles.getShaderFileNames(),
                     shaderFiles.getNumShaderFileNames())) {
        auto newProgram = shaderFiles.getShaderFile(uiShaderFileIndex);

        if (uiShaderPlatformIndex == SHADER_TOY) {
            newProgram->setFragmentShaderSourceTemplate(ShaderToyTemplate);
        } else {
            newProgram->setFragmentShaderSourceTemplate(std::string());
        }

        newProgram->compile();

        editor.SetText(newProgram->getFragmentShader().getSource());
        editor.SetCursorPosition(TextEditor::Coordinates());

        programErrors = newProgram->getFragmentShader().getErrors();
        if (programErrors.begin() != programErrors.end()) {
            cursorLine = programErrors.begin()->getLineNumber() - 1;
        }

        shaderFiles.replaceNewProgram(uiShaderFileIndex, newProgram);

        SetProgramErrors(programErrors, newProgram);

        swapProgram(newProgram);

        needRecompile = false;
    }

    ImGui::Checkbox("TextEditor", &uiShowTextEditor);

    if (uiShowTextEditor) {
        ShowTextEditor(uiShowTextEditor, uiShaderFileIndex,
                       uiShaderPlatformIndex);
        if (cursorLine >= 0) {
            editor.SetCursorPosition(TextEditor::Coordinates(cursorLine, 0));
            cursorLine = -1;
        }
    }

#ifdef __EMSCRIPTEN__
    ImGui::GetIO().SetClipboardTextFn(mainWindow, "");
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
    if (ImGui::Button("Open")) {
        std::string path;

        if (openFileDialog(path, "Shader file (*.glsl)\0*.glsl\0")) {
            std::shared_ptr<ShaderProgram> newProgram =
                std::make_shared<ShaderProgram>();

            if (uiShaderPlatformIndex == SHADER_TOY) {
                newProgram->setFragmentShaderSourceTemplate(ShaderToyTemplate);
            }

            compileShaderFromFile(newProgram,
                                  program->getVertexShader().getPath(), path);
            editor.SetText(newProgram->getFragmentShader().getSource());
            editor.SetCursorPosition(TextEditor::Coordinates());

            programErrors = newProgram->getFragmentShader().getErrors();
            if (programErrors.begin() != programErrors.end()) {
                cursorLine = programErrors.begin()->getLineNumber() - 1;
            }

            SetProgramErrors(programErrors, newProgram);

            uiShaderFileIndex = shaderFiles.pushNewProgram(newProgram);

            program.swap(newProgram);

            shaderFiles.loadFiles(fs::path(path).parent_path().string());
        }
    }
#endif

    ImGui::End();
}

void App::onUiErrorWindow() {
    ImGui::Begin("Errors", &uiErrorWindow);
    ImGui::SetWindowSize(ImVec2(800, 80), ImGuiCond_FirstUseEver);

    for (auto iter = programErrors.begin(); iter != programErrors.end();
         iter++) {
        const auto line = iter->getLineNumber();

        std::stringstream ss;

        ss << iter->getOriginal() << std::endl;

        bool selected = false;
        if (ImGui::Selectable(ss.str().c_str(), selected)) {
            if (line > 0 &&
                fs::path(this->program->getFragmentShader().getPath())
                        .compare(iter->getFileName()) == 0) {
                TextEditor::Coordinates cursor;
                cursor.mLine = line - 1;
                editor.SetCursorPosition(cursor);
            }
        }
    }

    ImGui::End();
}
}  // namespace shader_editor