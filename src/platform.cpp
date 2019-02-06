#include "platform.hpp"

#ifdef WIN32
#include <windows.h>
#include <commdlg.h>
#endif

#ifdef WIN32
bool openFileDialog(std::string &path, const char *const filter) {
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH] = "";
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
bool openFileDialog(std::string &path, std::string &filter) { return false; }
#endif
