#include "app.hpp"

#include <args.hxx>
#include <iostream>

namespace {
shader_editor::App app;

void update(void*) { app.update(nullptr); }
}  // namespace

int main(const int argc, const char** const argv) {
    args::ArgumentParser parser("Shader Editor for GLSL");
    args::HelpFlag help(parser, "help", "Display this help menu",
                        {'h', "help"});
    args::Flag top(parser, "top", "always on top", {'t'});

    try {
        parser.ParseCLI(argc, argv);
    } catch (const args::Completion& e) {
        std::cout << e.what();
        return 0;
    } catch (const args::Help&) {
        std::cout << parser;
        return 0;
    } catch (const args::ParseError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    app.start(top.Get());

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
