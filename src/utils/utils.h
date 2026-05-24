#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <cstring>

inline void RemoveFileNameFromPath(char* path) {
    char* lastSlash = strrchr(path, '\\');
    if (lastSlash != NULL) {
        *(lastSlash + 1) = '\0';
    }
}

std::string ReadWorkFile();

#endif // UTILS_H
