// Minimal POSIX dirent shim for MSVC (gofileseq findSequencesOnDisk).
// Uses FindFirstFileA / FindNextFileA. Not a full POSIX emulation.
#pragma once

#include <cerrno>
#include <cstring>
#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#ifndef DT_UNKNOWN
#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12
#define DT_WHT 14
#endif

struct dirent {
    unsigned char d_type;
    char d_name[MAX_PATH];
};

struct DIR {
    HANDLE handle;
    WIN32_FIND_DATAA data;
    bool first;
    bool exhausted;
    dirent entry;
};

inline unsigned char fileseq_win_d_type(const WIN32_FIND_DATAA& data) {
    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        return DT_DIR;
    }
    if (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        return DT_LNK;
    }
    return DT_REG;
}

inline void fileseq_win_fill_entry(DIR* dir) {
    dir->entry.d_type = fileseq_win_d_type(dir->data);
    std::strncpy(dir->entry.d_name, dir->data.cFileName, MAX_PATH - 1);
    dir->entry.d_name[MAX_PATH - 1] = '\0';
}

inline DIR* opendir(const char* path) {
    if (path == nullptr || path[0] == '\0') {
        errno = ENOENT;
        return nullptr;
    }

    std::string pattern(path);
    const char last = pattern.back();
    if (last != '\\' && last != '/') {
        pattern.push_back('\\');
    }
    pattern.append("*");

    DIR* dir = new DIR();
    dir->handle = FindFirstFileA(pattern.c_str(), &dir->data);
    if (dir->handle == INVALID_HANDLE_VALUE) {
        delete dir;
        errno = ENOENT;
        return nullptr;
    }
    dir->first = true;
    dir->exhausted = false;
    return dir;
}

inline int closedir(DIR* dir) {
    if (dir == nullptr) {
        return -1;
    }
    if (dir->handle != INVALID_HANDLE_VALUE) {
        FindClose(dir->handle);
    }
    delete dir;
    return 0;
}

inline dirent* readdir(DIR* dir) {
    if (dir == nullptr || dir->exhausted) {
        return nullptr;
    }

    if (dir->first) {
        dir->first = false;
        fileseq_win_fill_entry(dir);
        return &dir->entry;
    }

    if (!FindNextFileA(dir->handle, &dir->data)) {
        dir->exhausted = true;
        return nullptr;
    }
    fileseq_win_fill_entry(dir);
    return &dir->entry;
}
