#include <cstdlib>

#include "FSHelpers.h"
#include "Log.h"

extern void ConditionalFree(void* buf);

RawFile::~RawFile() {
    if (mDoFree) {
        free(mRawData);
    }

    mRawData = nullptr;
}

bool RawFile::get(const char* path, bool doFree, bool isString) {
    mDoFree = doFree;

    if (!FileExists(path)) {
        return false;
    }

    nn::fs::FileHandle handle;
    nn::fs::OpenFile(&handle, path, nn::fs::OpenMode_Read);
    nn::fs::GetFileSize(&mRawDataSize, handle);

    if (mRawDataSize == 0) {
        nn::fs::CloseFile(handle);
        return false;
    }

    mRawData = static_cast<u8*>(malloc(mRawDataSize + static_cast<u8>(isString)));
    if (mRawData == nullptr) {
        ERROR_LOG("Failed to allocate space for file %s", path);
        nn::fs::CloseFile(handle);
        return false;
    }

    nn::fs::ReadFile(handle, 0, mRawData, mRawDataSize);
    
    if (isString) {
        mRawData[mRawDataSize] = 0;
    }

    nn::fs::CloseFile(handle);
    return true;
}
