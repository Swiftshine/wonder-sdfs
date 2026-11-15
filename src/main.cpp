#define SEAD_VERSION 5

#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>
#include "zstddeclib.c"

#include <ares/ResourceUnit.h>

#include <hk/diag/diag.h>
#include <hk/hook/Trampoline.h>

#include <nn/fs.h>
#include <nn/svc.h>

#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <span>

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
    bool mDoFree = false;

    ~RawResource() {
        if (mRawData != nullptr && mDoFree) {
            free(mRawData);
        }

        mRawData = nullptr;
    }

    bool get(const char* path, bool doFree, bool isString) {
        mDoFree = doFree;

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
                if (res.get(configPath, false, true)) {
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

struct MyDirectResource : public sead::DirectResource {
    u8* swapRawData(u8* newData) {
        u8* old = mRawData;
        mRawData = newData;
        return old;
    }

    void setRawData(u8* newData) {
        mRawData = newData;
    }

    void setRawSize(u32 size) {
        mRawSize = size;
    }
};

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
        
        PathBuf filepath(path.cstr(), true);
        // char filepath[1024] = { 0 };
        // snprintf(filepath, sizeof(filepath), "sd:/wonder-sdfs/%s/%s.zs", Config::instance().getFolderName(), path.cstr());

        INFO_LOG("Full path: %s", filepath.str());

        RawResource rawRes;
        if (rawRes.get(filepath.str(), true, false)) {
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

                        MyDirectResource* resource = static_cast<MyDirectResource*>(directResource);

                        u8* oldBuf = resource->swapRawData(&decompressed[0]);
                        resource->setRawSize(decompSize);

                        // u32 oldSize = resource->getRawSize();

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

sead::DirectResource* tryForceBGYMLReload(ares::ResourceBinder* binder, const sead::SafeString& path, u64 va1, u64 va2) {
    sead::DirectResource* directResource = binder->tryLoadSync(path, va1, va2);

    const char* allowedFolders[] = {
        "Gyml/Gfx/EnvPaletteParam/",
        "Stage/AreaParam/",
        "Stage/StageLoadInfo/",
        "Stage/StageParam/",
    };

    bool allowed = false;
    for (const char* item : allowedFolders) {
        if (stringContains(path.cstr(), item)) {
            allowed = true;
            break;
        }
    }

    if (!allowed) {
        return directResource;
    }

    if (directResource != nullptr && Config::instance().isLoaded()) {
        PathBuf filepath(path.cstr());
        RawResource rawRes;
        if (rawRes.get(filepath.str(), false, false)) {
            INFO_LOG("Swapping %s", filepath.str());
            MyDirectResource* resource = static_cast<MyDirectResource*>(directResource);

            u8* oldBuf = resource->swapRawData(rawRes.mRawData);
            resource->setRawSize(rawRes.mRawDataSize);

            if (oldBuf != nullptr) {
                delete oldBuf;
            }

            INFO_LOG("Swapping done");
        }
    }

    return directResource;
}

void ReloadCompressedData(MyDirectResource* resource, const char* path) {
    PathBuf filepath(path, true);

    RawResource compressed;

    if (!compressed.get(filepath.str(), true, false)) {
        return;
    }

    INFO_LOG("Decompressing...");
    unsigned long long decompSize = ZSTD_getFrameContentSize(compressed.data(), compressed.size());

    if (decompSize == ZSTD_CONTENTSIZE_ERROR) {
        ERROR_LOG("Invalid ZSTD frame");
        return;
    } else if (decompSize == ZSTD_CONTENTSIZE_UNKNOWN) {
        ERROR_LOG("ZSTD frame does not specify decompressed size");
        return;
    }
    
    u8* decompressed = static_cast<u8*>(malloc(decompSize));

    if (decompressed == nullptr) {
        ERROR_LOG("Failed to allocate space for decompressed data");
        return;
    }

    size_t result = ZSTD_decompress(decompressed, decompSize, compressed.data(), compressed.size());

    if (ZSTD_isError(result)) {
        ERROR_LOG("Decompression failed: %s", ZSTD_getErrorName(result));
        free(decompressed);
        return;
    }

    INFO_LOG("Decompression OK");

    u8* oldBuf = resource->swapRawData(decompressed);
    resource->setRawSize(decompSize);

    if (oldBuf != nullptr) {
        delete oldBuf;
    }
}

static void* sCurrentBGYML = nullptr;

void readCurrentBGYML(RawResource& bgyml, const char* path) {
    if (sCurrentBGYML != nullptr) {
        return;
    }

    PathBuf filepath(path, false);

    if (!bgyml.get(filepath.str(), true, false)) {
        return;
    }

    sCurrentBGYML = bgyml.data();
}

void ReloadBGYMLData(MyDirectResource* resource, const char* path) {
    PathBuf filepath(path, false);

    RawResource bgyml;

    if (!bgyml.get(filepath.str(), false, false)) {
        return;
    }

    u32 oldSize = resource->getRawSize();
    u8* oldBuf = resource->getRawData();
    // u8* oldBuf = resource->swapRawData(bgyml.data());
    // resource->setRawSize(bgyml.size());

    // if (oldSize != bgyml.size()) {
    //     INFO_LOG("Sizes [old] %d and [new] %d are not the same", oldSize, bgyml.size());
    // }

    // bool eq = memcmp(oldBuf, bgyml.data(), MIN(oldSize, bgyml.size())) == 0;

    // if (!eq) {
    //     INFO_LOG("Files are different");
    //     INFO_LOG("Swapping...");
    //     resource->setRawData(bgyml.data());
    //     resource->setRawSize(bgyml.size());

    //     if (oldBuf != nullptr) {
    //         delete oldBuf;
    //     }
    //     INFO_LOG("Swapping done.");
    // } else {
    //     bgyml.setDoFree(true);
    // }
    
    // it looks like the data is set somewhere else
}

extern "C" void _init();

u64 getOffs(u64 addr) {
    u64 binaryStart = ((u64)&_init) - 0x30;
    return addr - binaryStart;
}

void printLR() {
    u64 lr = (u64)__builtin_return_address(0);
    u64 lrOffs = getOffs(lr);
    hk::diag::logLine("Addr: 0x%" PRIx64 "\n", lrOffs);
}

HkTrampoline<sead::DirectResource*, ares::ResourceBinder*, const sead::SafeString&, u64, u64, u64> resourceTryLoadSyncHook = [](ares::ResourceBinder* _this, const sead::SafeString& path, u64 va1, u64 va2, u64 va3) {
    RawResource bgyml;

    const char* compressedPaths[] = {
        "BancMapUnit/",
        "Phive/StaticCompoundBody/"
    };

    const char* bgymlPaths[] = {
        "Stage/",
        "Gyml/Gfx/EnvPaletteParam/"
    };

    for (const char* item : bgymlPaths) {
        if (stringContains(path.cstr(), item)) {
            readCurrentBGYML(bgyml, path.cstr());
            break;
        }
    }

    sead::DirectResource* directResource = resourceTryLoadSyncHook.orig(_this, path, va1, va2, va3);

    if (directResource == nullptr || !Config::instance().isLoaded()) {
        return directResource;
    }
    
    MyDirectResource* resource = static_cast<MyDirectResource*>(directResource);

    for (const char* item : compressedPaths) {
        if (stringContains(path.cstr(), item)) {
            ReloadCompressedData(resource, path.cstr());
            break;
        }
    }
    return directResource;
};

HkTrampoline<bool, u64, void*, u64, u64> generalResourceFormat_initializeHook = [](u64 _this, void* rawData, u64 va1, u64 va2) {
    if (sCurrentBGYML != nullptr) {
        bool result = generalResourceFormat_initializeHook.orig(_this, sCurrentBGYML, va1, va2);
        delete (u8*)sCurrentBGYML;
        sCurrentBGYML = nullptr;
        return result;
    }

    return generalResourceFormat_initializeHook.orig(_this, rawData, va1, va2);
};

extern "C" void hkMain() {
    openFileHook.installAtSym<"_ZN2nn2fs8OpenFileEPNS0_10FileHandleEPKci">();
    // hk::hook::writeBranchLinkAtMainOffset(0x001A6BE8, tryForceLevelReload);
    // hk::hook::writeBranchLinkAtMainOffset(0x00076E1C, tryForceBGYMLReload);
    // hk::hook::writeBranchLinkAtMainOffset(0x00076ECC, tryForceBGYMLReload);

    // engine_resource_loadSyncHook.installAtMainOffset(0x00076DC4);
    resourceTryLoadSyncHook.installAtMainOffset(0x0003B750);
    generalResourceFormat_initializeHook.installAtMainOffset(0x00244D44);
    // getResourceHook.installAtMainOffset(0x00257FE0);
    // getResourceHook.installAtMainOffset(0x0037CD70);
    INFO_LOG("Installed all hooks!");
}
