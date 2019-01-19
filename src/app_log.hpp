#pragma once

#include <imgui.h>

class AppLog {
   private:
    ImGuiTextBuffer Buf;
    ImGuiTextFilter Filter;
    ImVector<int>
        LineOffsets;  // Index to lines offset. We maintain this with AddLog()
                      // calls, allowing us to have a random access on lines
    bool ScrollToBottom;

   public:
    AppLog() { Clear(); }

    void Clear();

    void AddLog(const char* fmt, ...);

    void Draw(const char* title, bool* p_open = NULL);
};

AppLog& GetAppLog();
void ShowAppLogWindow(bool* pOpen);
