#pragma once

#include <imgui.h>

typedef enum {
    Detail,
    Debug,
    Info,
    Error,
} AppLogLevel;

class AppLog {
   private:
    AppLogLevel logLevel = Info;
    ImGuiTextBuffer buf;
    ImGuiTextFilter filter;
    ImVector<int>
        lineOffsets;  // Index to lines offset. We maintain this with AddLog()
                      // calls, allowing us to have a random access on lines
    bool scrolltoBottom;

    void addLog(va_list args, const char* fmt);

   public:
    static AppLog& getInstance();

    AppLog() { clear(); }

    void setLogLevel(AppLogLevel logLevel);

    AppLogLevel getLogLevel();

    void clear();

    void detail(const char* fmt, ...);

    void debug(const char* fmt, ...);

    void info(const char* fmt, ...);

    void error(const char* fmt, ...);

    void draw(const char* title, bool* p_open = NULL);

    void showAppLogWindow(bool& open);
};
