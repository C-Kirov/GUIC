#include "utils.h"
#include <windows.h>
#include <fstream>

std::string ReadWorkFile()
{
    char workFilePath[MAX_PATH];
    GetModuleFileNameA(NULL, workFilePath, MAX_PATH);
    RemoveFileNameFromPath(workFilePath);
    strcat(workFilePath, "work.txt");
    std::ifstream file(workFilePath);
    if (!file) return "";
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    size_t pos = 0;
    while ((pos = content.find("\r\n")) != std::string::npos)
        content.replace(pos, 2, "\n");
    return content;
}
