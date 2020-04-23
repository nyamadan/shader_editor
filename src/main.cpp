#include "app.hpp"

namespace {
App app;

void update(void *) {
    app.update(nullptr);
}
}  // namespace

int main(void) {
    app.start();

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
