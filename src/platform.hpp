#include <string>

bool openFileDialog(std::string &path, const char * const filter);
bool saveFileDialog(std::string &path, const char * const filter, const char *const defExt);
