#define SEAD_VERSION 5

#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>
#include "zstddeclib.c"

#include <ares/ResourceUnit.h>

#include <hk/diag/diag.h>
#include <hk/hook/Trampoline.h>

#include <nn/fs.h>

#include <cstring>

static bool sIsCardMounted = false;
const char* WONDER_FOLDER_PATH = "sd:/wonder-sdfs/";

#define VERBOSE false

#if VERBOSE
    #define VERBOSE_LOG(fmt, ...) hk::diag::logLine(fmt, ##__VA_ARGS__);
#else
    #define VERBOSE_LOG(...)
#endif

bool fileExists(const char* path) {
    nn::fs::DirectoryEntryType type;
    nn::fs::GetEntryType(&type, path);
    return type == nn::fs::DirectoryEntryType_File;
}

bool stringContains(const char* string, const char* query) {
    return string != nullptr && query != nullptr && std::strstr(string, query) != nullptr;
}

HkTrampoline<nn::Result, nn::fs::FileHandle*, const char*, int> openFileHook = [](nn::fs::FileHandle* out, const char* filePath, int mode) -> nn::Result {
    if (!sIsCardMounted) {
        if (nn::fs::MountSdCard("sd").IsSuccess()) {
            sIsCardMounted = true;
            hk::diag::logLine("Mounted SD card!");
        } else {
            hk::diag::logLine("Failed to mount SD card.");
        }
    }

    // if (sIsCardMounted) {
    //     if (std::strncmp(filePath, "content:/", 9) == 0) {
    //         char path[1024] { 0 };
    //         strcat(path, WONDER_SD_CARD_PATH);
    //         strcat(path, filePath + 9);
            
    //         if (fileExists(path)) {
    //             hk::diag::logLine("[From SD]: Opening %s", path);
    //             return openFileHook.orig(out, path, mode);
    //         }
    //     }
    // }
    
    if (stringContains(filePath, "content:/")) {
        VERBOSE_LOG("[From ROMFS]: Opening %s", filePath);
    } else if (stringContains(filePath, "sd:/")) {
        hk::diag::logLine("[From SD]: Opening %s", filePath);
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

struct RawResource {
    void* raw = nullptr;
    s64 size = 0;

    ~RawResource() {
        if (raw != nullptr) {
            free(raw);
        }

        raw = nullptr;
    }
};

bool getRawResource(RawResource& out, const char* path) {
    if (!fileExists(path)) {
        return false;
    }

    nn::fs::FileHandle handle;
    nn::fs::OpenFile(&handle, path, nn::fs::OpenMode_Read);

    nn::fs::GetFileSize(&out.size, handle);

    if (out.size == 0) {
        nn::fs::CloseFile(handle);
        return false;
    }

    out.raw = malloc(out.size);

    if (out.raw == nullptr) {
        nn::fs::CloseFile(handle);
        return false;
    }
    
    nn::fs::ReadFile(handle, 0, out.raw, out.size);    
    nn::fs::CloseFile(handle);
    return true;
}

sead::DirectResource* tryForceLevelReload(ares::ResourceBinder* binder, const sead::SafeString& path, u64 va1, u64 va2) {
    sead::DirectResource* directResource = binder->tryLoadSync(path, va1, va2);

    if (!stringContains(path.cstr(), "Course")) {
        return directResource;
    }
    
    // checkTestFile();
    // hk::diag::logLine("Accessing level file: %s", path.cstr());

    if (directResource != nullptr) {
        hk::diag::logLine("Attempting to re-read level file: %s", path.cstr());
        char filepath[1024] = { 0 };
        strcpy(filepath, WONDER_FOLDER_PATH);
        strcat(filepath, path.cstr());
        strcat(filepath, ".zs");

        VERBOSE_LOG("Full path: %s", filepath);

        RawResource rawRes;
        if (getRawResource(rawRes, filepath)) {
            hk::diag::logLine("Decompressing");
            
            unsigned long long decompSize = ZSTD_getFrameContentSize(rawRes.raw, rawRes.size);

            if (decompSize == ZSTD_CONTENTSIZE_ERROR) {
                hk::diag::logLine("Invalid ZSTD frame");
            } else if (decompSize == ZSTD_CONTENTSIZE_UNKNOWN) {
                hk::diag::logLine("ZSTD frame does not specify decompressed size");
            } else {
                void* decompressed = malloc(decompSize);

                if (decompressed != nullptr) {
                    size_t result = ZSTD_decompress(decompressed, decompSize, rawRes.raw, rawRes.size);

                    if (ZSTD_isError(result)) {
                        hk::diag::logLine("Decompression failed: %s", ZSTD_getErrorName(result));
                    } else {
                        hk::diag::logLine("Decompression OK!");

                        struct MyResource : public sead::DirectResource {
                            void setRawData(u8* data) {
                                mRawData = data;
                            }

                            void setRawSize(u32 size) {
                                mRawSize = size;
                            }
                        };

                        MyResource* resource = static_cast<MyResource*>(directResource);

                        hk::diag::logLine("Setting raw data");
                        void* oldBuf = resource->getRawData();
                        resource->setRawData(static_cast<u8*>(decompressed));
                        
                        hk::diag::logLine("Setting raw size");
                        u32 oldSize = resource->getRawSize();
                        resource->setRawSize(decompSize);

                        free(oldBuf);
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
}
