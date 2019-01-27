#pragma once

#include <imgui.h>

class AppLog {
   private:
    ImGuiTextBuffer buf;
    ImGuiTextFilter filter;
    ImVector<int>
        lineOffsets;  // Index to lines offset. We maintain this with AddLog()
                      // calls, allowing us to have a random access on lines
    bool scrolltoBottom;

   public:
    static AppLog& getInstance();

    AppLog() { clear(); }

    void clear();

    void addLog(const char* fmt, ...);

    void draw(const char* title, bool* p_open = NULL);

    void showAppLogWindow(bool& open);
};
