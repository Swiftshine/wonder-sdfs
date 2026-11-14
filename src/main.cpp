#define SEAD_VERSION 5

#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>
#include "zstddeclib.c"

#include <ares/ResourceUnit.h>

#include <hk/diag/diag.h>
#include <hk/hook/Trampoline.h>

#include <nn/fs.h>

#include <cstring>
#include <cstdio>

#include "Config.h"

#define SHOW_VERBOSE_LOG false
#define SHOW_INFO_LOG true
#define SHOW_ERROR_LOG true

#if SHOW_VERBOSE_LOG
    #define VERBOSE_LOG(fmt, ...) hk::diag::logLine("wonder-sdfs - VERBOSE: " #fmt, ##__VA_ARGS__);
#else
    #define VERBOSE_LOG(...)
#endif

#if SHOW_INFO_LOG
    #define INFO_LOG(fmt, ...) hk::diag::logLine("wonder-sdfs - INFO: " #fmt, ##__VA_ARGS__);
#else
    #define INFO_LG(...)
#endif

#if SHOW_ERROR_LOG
    #define ERROR_LOG(fmt, ...) hk::diag::logLine("wonder-sdfs - ERROR: " #fmt, ##__VA_ARGS__)
#else
    #define ERROR_LOG(...)
#endif

bool fileExists(const char* path) {
    nn::fs::DirectoryEntryType type;
    nn::fs::GetEntryType(&type, path);
    return type == nn::fs::DirectoryEntryType_File;
}

bool stringContains(const char* string, const char* query) {
    return string != nullptr && query != nullptr && std::strstr(string, query) != nullptr;
}

class RawResource {
public:
    u8* mRawData = nullptr;
    s64 mRawDataSize = 0;

    ~RawResource() {
        if (mRawData != nullptr) {
            free(mRawData);
        }

        mRawData = nullptr;
    }

    bool get(const char* path, bool isString = false) {
        if (!fileExists(path)) {
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

};

HkTrampoline<nn::Result, nn::fs::FileHandle*, const char*, int> openFileHook = [](nn::fs::FileHandle* out, const char* filePath, int mode) -> nn::Result {
    static bool isSDCardMounted = false;
    static bool shouldTryToMountCard = true;
    static const char* configPath = "sd:/wonder-sdfs/config.cfg";

    if (!isSDCardMounted && shouldTryToMountCard) {
        if (nn::fs::MountSdCard("sd").IsSuccess()) {
            INFO_LOG("Mounted SD card!");
            isSDCardMounted = true;
            
            if (fileExists(configPath)) {
                INFO_LOG("Found configuration file");

                RawResource res;
                if (res.get(configPath, true)) {
                    if (Config::instance().load(reinterpret_cast<const char*>(res.mRawData))) {
                        INFO_LOG("Configuration loading OK!");

                        INFO_LOG("Folder name: %s", Config::instance().getFolderName());
                    }
                }
            } else {
                ERROR_LOG("Configuration file does not exist.");
            }
        } else {
            ERROR_LOG("Failed to mount SD card.");
        }

        shouldTryToMountCard = false;
    }

    if (stringContains(filePath, "content:/")) {
        VERBOSE_LOG("[From ROMFS]: Opening %s", filePath);
    } else if (stringContains(filePath, "sd:/")) {
        INFO_LOG("[From SD]: Opening %s", filePath);
    }

    return openFileHook.orig(out, filePath, mode);
};

// void checkFile(const char* filepath) {
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

// void checkTestFile() {
//     const char* filepath = "sd:/wonder-sdfs/test.txt";
//     checkFile(filepath);
// }


sead::DirectResource* tryForceLevelReload(ares::ResourceBinder* binder, const sead::SafeString& path, u64 va1, u64 va2) {
    // normally, mixing and matching malloc/free and operator new/delete is not recommended due to UB
    // it appears to be fine here, however. it's also necessary; using either pair causes decompression/memory deallocation to fail
    // todo: maybe look into using any default sead::Heap?
    
    sead::DirectResource* directResource = binder->tryLoadSync(path, va1, va2);

    if (!stringContains(path.cstr(), "Course")) {
        return directResource;
    }
    
    // checkTestFile();
    // hk::diag::logLine("Accessing level file: %s", path.cstr());

    if (directResource != nullptr && Config::instance().isLoaded()) {
        INFO_LOG("Attempting to re-read level file: %s", path.cstr());
        char filepath[1024] = { 0 };
        snprintf(filepath, sizeof(filepath), "sd:/wonder-sdfs/%s/%s.zs", Config::instance().getFolderName(), path.cstr());

        INFO_LOG("Full path: %s", filepath);

        RawResource rawRes;
        if (rawRes.get(filepath)) {
            INFO_LOG("Decompressing...");
            
            unsigned long long decompSize = ZSTD_getFrameContentSize(rawRes.mRawData, rawRes.mRawDataSize);

            if (decompSize == ZSTD_CONTENTSIZE_ERROR) {
                ERROR_LOG("Invalid ZSTD frame");
            } else if (decompSize == ZSTD_CONTENTSIZE_UNKNOWN) {
                ERROR_LOG("ZSTD frame does not specify decompressed size");
            } else {
                u8* decompressed = static_cast<u8*>(malloc(decompSize));

                if (decompressed != nullptr) {
                    size_t result = ZSTD_decompress(decompressed, decompSize, rawRes.mRawData, rawRes.mRawDataSize);

                    if (ZSTD_isError(result)) {
                        ERROR_LOG("Decompression failed: %s", ZSTD_getErrorName(result));
                    } else {
                        INFO_LOG("Decompression OK!");

                        struct MyResource : public sead::DirectResource {
                            void setRawData(u8* data) {
                                mRawData = data;
                            }

                            void setRawSize(u32 size) {
                                mRawSize = size;
                            }
                        };

                        MyResource* resource = static_cast<MyResource*>(directResource);

                        u8* oldBuf = resource->getRawData();
                        resource->setRawData(&decompressed[0]);

                        u32 oldSize = resource->getRawSize();
                        resource->setRawSize(decompSize);

                        if (oldBuf != nullptr) {
                            delete oldBuf;
                        }
                    }
                }
            }
        }
    }

    return directResource;
}

extern "C" void hkMain() {
    openFileHook.installAtSym<"_ZN2nn2fs8OpenFileEPNS0_10FileHandleEPKci">();
    hk::hook::writeBranchLinkAtMainOffset(0x001A6BE8, tryForceLevelReload);
    INFO_LOG("Installed all hooks!");
}
