#include "app.hpp"
#include "app_log.hpp"

#include <args.hxx>

#include <iostream>
#include <memory>

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
using namespace emscripten;
#endif

namespace {
shader_editor::App app;

#ifdef __EMSCRIPTEN__
static auto clipboardText = std::string();

void SetClipboardText(const std::string& text) { clipboardText = text; }

EMSCRIPTEN_BINDINGS(clipboard_module) {
    function("SetClipboardText", &SetClipboardText);
}

void SetClipboardTextImpl(void*, const char* text) {
    // clang-format off
    EM_ASM({
        const copy = document.createElement("textarea");
        document.body.appendChild(copy);
        copy.style.position = "absolute";
        copy.textContent = UTF8ToString($0);
        copy.focus();
        copy.select();
        document.execCommand("copy");
        document.body.removeChild(copy);
        Module.canvas.focus();
    }, text);
    // clang-format on

    SetClipboardText(text);
}

const char* GetClipboardTextImpl(void*) { return clipboardText.c_str(); }
#endif

void update(void*) { app.update(nullptr); }
}  // namespace

int main(const int argc, const char** const argv) {
    args::ArgumentParser parser("Shader Editor for GLSL");
    args::HelpFlag help(parser, "help", "display this help menu",
                        {'h', "help"});
    args::Flag top(parser, "top", "always on top", {'t', "top"}, false);

    args::ValueFlag<std::string> log(
        parser, "log", "loglevel(detail, debug, info, error)", {'l', "log"});

    args::ValueFlag<int32_t> width(parser, "width", "window width", {"width"},
                                   1024);

    args::ValueFlag<int32_t> height(parser, "height", "window height",
                                    {"height"}, 768);

    args::Positional<std::string> assetPath(parser, "asset path",
                                            "path to asset", ".");

    try {
        parser.ParseCLI(argc, argv);
    } catch (const args::Completion& e) {
        std::cout << e.what();
        return 0;
    } catch (const args::Help&) {
        std::cout << parser;
        return 0;
    } catch (const args::ParseError& e) {
        std::cerr << e.what() << std::endl << std::endl << parser;
        return 1;
    } catch (const args::ValidationError& e) {
        std::cerr << e.what() << std::endl << std::endl << parser;
        return 1;
    }

    if (width.Get() <= 0) {
        std::cerr << "width must be greater than 0." << std::endl
                  << std::endl
                  << parser;
        return 1;
    }

    if (height.Get() <= 0) {
        std::cerr << "height must be greater than 0." << std::endl
                  << std::endl
                  << parser;
        return 1;
    }

    if (log.Get().compare("detail") == 0) {
        AppLog::getInstance().setLogLevel(AppLogLevel::Detail);
    }
    if (log.Get().compare("debug") == 0) {
        AppLog::getInstance().setLogLevel(AppLogLevel::Debug);
    }
    if (log.Get().compare("info") == 0) {
        AppLog::getInstance().setLogLevel(AppLogLevel::Info);
    }
    if (log.Get().compare("error") == 0) {
        AppLog::getInstance().setLogLevel(AppLogLevel::Error);
    }

    app.start(width.Get(), height.Get(), assetPath.Get(), top.Get());

#ifdef __EMSCRIPTEN__
    ImGui::GetIO().SetClipboardTextFn = SetClipboardTextImpl;
    ImGui::GetIO().GetClipboardTextFn = GetClipboardTextImpl;

    // clang-format off
    EM_ASM({
        document.body.addEventListener("paste", (event) => {
            if (event.target !== document.body) {
                return;
            }

            const text = (event.clipboardData || window.clipboardData).getData("text");
            Module.SetClipboardText(text);
            event.preventDefault();
        }, false);
    });
    // clang-format on
#endif

#ifndef __EMSCRIPTEN__
    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(app.getMainWindow())) {
        update(nullptr);
    }

    app.cleanup();

    glfwTerminate();
#else
    emscripten_set_main_loop_arg(update, nullptr, 0, 0);
    glfwSwapInterval(1);
#endif

    return 0;
}
