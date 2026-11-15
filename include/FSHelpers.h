#pragma once
#include <cstdio>

#include <nn/fs.h>

#include "Config.h"

constexpr bool FileExists(const char* path) {
    nn::fs::DirectoryEntryType type;
    nn::fs::GetEntryType(&type, path);
    return type == nn::fs::DirectoryEntryType_File;
}

class RawFile {
public:
    u8* mRawData = nullptr;
    s64 mRawDataSize = 0;
    bool mDoFree = false;

    ~RawFile();

    bool get(const char* path, bool doFree, bool isString);

    void setDoFree(bool doFree) {
        mDoFree = doFree;
    }

    u8* data() const {
        return mRawData;
    }

    u32 size() const {
        return mRawDataSize;
    }
};

class PathBuf {
public:
    PathBuf(const char* path, bool isZS = false) {
        const char* fmt = isZS ? "sd:/wonder-sdfs/%s/%s.zs" : "sd:/wonder-sdfs/%s/%s";
        snprintf(mBuffer, sizeof(mBuffer), fmt, Config::instance().getFolderName(), path);
    }

    const char* str() const {
        return mBuffer;
    }
private:
    char mBuffer[1024] = { 0 };
};

// void CheckFile(const char* filepath) {
//     if (!fileExists(filepath)) {
//         hk::diag::logLine("File doesn't exist");
//         return;
//     }

//     hk::diag::logLine("File exists.");
//     nn::fs::FileHandle handle;
//     nn::Result result = nn::fs::OpenFile(&handle, filepath, nn::fs::OpenMode_Read);

//     if (result.IsSuccess()) {
//         hk::diag::logLine("Access OK!");
//         nn::fs::CloseFile(handle);
//     } else {
//         hk::diag::logLine("Cannot access file.");
//     }
// }

// void CheckTestFile() {
//     const char* filepath = "sd:/wonder-sdfs/test.txt";
//     checkFile(filepath);
// }
