#include "platform.hpp"

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <windows.h>
#include <commdlg.h>
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
bool openFileDialog(std::string &path, const char *const filter) {
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH + 1] = "";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;

    char cwd[MAX_PATH];
    GetCurrentDirectory(sizeof(cwd), cwd);
    bool result = GetOpenFileName(&ofn);
    SetCurrentDirectory(cwd);

    if (result) {
        path.assign(szFile);
    }

    return result;
}
#else
bool openFileDialog(std::string &path, const char * const filter) { return false; }
#endif
