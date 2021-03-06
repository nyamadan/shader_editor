#include "app_log.hpp"

#include <iostream>

namespace {
AppLog appLog;
}

AppLog& AppLog::getInstance() { return appLog; }

void AppLog::setLogLevel(AppLogLevel logLevel) {
    this->logLevel = logLevel;
}

AppLogLevel AppLog::getLogLevel() {
    return this->logLevel;
}

void AppLog::clear() {
    buf.clear();
    lineOffsets.clear();
    lineOffsets.push_back(0);
}

void AppLog::addLog(va_list args, const char* fmt) {
    int old_size = buf.size();
    buf.appendfv(fmt, args);

    std::cout << buf.c_str() + old_size;

    for (int new_size = buf.size(); old_size < new_size; old_size++)
        if (buf[old_size] == '\n') lineOffsets.push_back(old_size + 1);
    scrolltoBottom = true;
}

void AppLog::detail(const char* fmt, ...) {
    if(logLevel > AppLogLevel::Detail) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    addLog(args, fmt);
    va_end(args);
}

void AppLog::debug(const char* fmt, ...) {
    if(logLevel > AppLogLevel::Debug) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    addLog(args, fmt);
    va_end(args);
}

void AppLog::info(const char* fmt, ...) {
    if(logLevel > AppLogLevel::Info) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    addLog(args, fmt);
    va_end(args);
}

void AppLog::error(const char* fmt, ...) {
    if(logLevel > AppLogLevel::Error) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    addLog(args, fmt);
    va_end(args);
}


void AppLog::draw(const char* title, bool* p_open) {
    if (!ImGui::Begin(title, p_open)) {
        ImGui::End();
        return;
    }
    if (ImGui::Button("Clear")) clear();
    ImGui::SameLine();
    bool copy = ImGui::Button("Copy");
    ImGui::SameLine();
    filter.Draw("Filter", -100.0f);
    ImGui::Separator();
    ImGui::BeginChild("scrolling", ImVec2(0, 0), false,
                      ImGuiWindowFlags_HorizontalScrollbar);
    if (copy) ImGui::LogToClipboard();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    const char* buf = this->buf.begin();
    const char* buf_end = this->buf.end();
    if (filter.IsActive()) {
        for (int line_no = 0; line_no < lineOffsets.Size; line_no++) {
            const char* line_start = buf + lineOffsets[line_no];
            const char* line_end = (line_no + 1 < lineOffsets.Size)
                                       ? (buf + lineOffsets[line_no + 1] - 1)
                                       : buf_end;
            if (filter.PassFilter(line_start, line_end))
                ImGui::TextUnformatted(line_start, line_end);
        }
    } else {
        // The simplest and easy way to display the entire buffer:
        //   ImGui::TextUnformatted(buf_begin, buf_end);
        // And it'll just work. TextUnformatted() has specialization for
        // large blob of text and will fast-forward to skip non-visible
        // lines. Here we instead demonstrate using the clipper to only
        // process lines that are within the visible area. If you have tens
        // of thousands of items and their processing cost is
        // non-negligible, coarse clipping them on your side is recommended.
        // Using ImGuiListClipper requires A) random access into your data,
        // and B) items all being the  same height, both of which we can
        // handle since we an array pointing to the beginning of each line
        // of text. When using the filter (in the block of code above) we
        // don't have random access into the data to display anymore, which
        // is why we don't use the clipper. Storing or skimming through the
        // search result would make it possible (and would be recommended if
        // you want to search through tens of thousands of entries)
        ImGuiListClipper clipper;
        clipper.Begin(lineOffsets.Size);
        while (clipper.Step()) {
            for (int line_no = clipper.DisplayStart;
                 line_no < clipper.DisplayEnd; line_no++) {
                const char* line_start = buf + lineOffsets[line_no];
                const char* line_end =
                    (line_no + 1 < lineOffsets.Size)
                        ? (buf + lineOffsets[line_no + 1] - 1)
                        : buf_end;
                ImGui::TextUnformatted(line_start, line_end);
            }
        }
        clipper.End();
    }
    ImGui::PopStyleVar();

    if (scrolltoBottom) ImGui::SetScrollHereY(1.0f);
    scrolltoBottom = false;
    ImGui::EndChild();
    ImGui::End();
}

void AppLog::showAppLogWindow(bool& open) {
    const char* const Title = "Log";
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    this->draw(Title, &open);
}
