#define SEAD_VERSION 5

#define ZSTD_STATIC_LINKING_ONLY
#include <zstd.h>
#include "zstddeclib.c"

#include <heap/seadHeapMgr.h>

#include <ares/ResourceUnit.h>

#include <hk/diag/diag.h>
#include <hk/hook/Trampoline.h>

#include <cstring>
#include <cstdio>

#include "Config.h"
#include "FSHelpers.h"
#include "Misc.h"
#include "Log.h"

HkTrampoline nn_fs_OpenFile_hook = [](TrampolineStatic(), nn::fs::FileHandle* out, const char* filePath, int mode) -> nn::Result {
    static bool isSDCardMounted = false;
    static bool shouldTryToMountCard = true;
    static const char* configPath = "sd:/wonder-sdfs/config.cfg";

    if (!isSDCardMounted && shouldTryToMountCard) {
        if (nn::fs::MountSdCard("sd").IsSuccess()) {
            INFO_LOG("Mounted SD card");
            isSDCardMounted = true;
            
            if (FileExists(configPath)) {
                INFO_LOG("Found configuration file");

                RawFile res;
                if (res.get(configPath, false, true)) {
                    if (Config::instance().load(reinterpret_cast<const char*>(res.mRawData))) {
                        INFO_LOG("Configuration loading OK");

                        Config::printConfiguration();   
                    }
                }
            } else {
                ERROR_LOG("Configuration file does not exist");
            }
        } else {
            ERROR_LOG("Failed to mount SD card");
        }

        shouldTryToMountCard = false;
    }

    if (StringContains(filePath, "content:/")) {
        if (StringContains(filePath, "wonder-sdfs-assets")) {
            // load from SD card instead
            char path[1024] = { 0 };
            strcpy(path, "sd:/wonder-sdfs/");
            strcat(path, filePath + strlen("content:/"));
            INFO_LOG("[From SD]: Opening %s", path);
            return orig(out, path, mode);
        } else {
            VERBOSE_LOG("[From ROMFS]: Opening %s", filePath);
        }
    } else if (StringContains(filePath, "sd:/")) {
        INFO_LOG("[From SD]: Opening %s", filePath);
    }
    
    return orig(out, filePath, mode);
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

void ReloadCompressedData(MyDirectResource* resource, const char* path) {
    PathBuf filepath(path, true);

    RawFile compressed;

    if (!compressed.get(filepath.str(), true, false)) {
        return;
    }

    INFO_LOG("Decompressing");
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

    void* old = resource->swapRawData(decompressed);
    resource->setRawSize(decompSize);
    
    if (old != nullptr) {
        delete static_cast<u8*>(old);
    }

    INFO_LOG("Buffers swapped");
}

static void* sCurrentBGYML = nullptr;
static u32 sCurrentBGYMLSize = 0;

void ReadCurrentBGYML(const char* path) {
    if (sCurrentBGYML != nullptr) {
        return;
    }

    PathBuf filepath(path, false);
    RawFile bgyml;
    if (!bgyml.get(filepath.str(), false, false)) {
        return;
    }

    sCurrentBGYML = bgyml.data();
    sCurrentBGYMLSize = bgyml.size();
}

HkTrampoline ares_ResourceBinder_tryLoadSync_hook = [](TrampolineStatic(), ares::ResourceBinder* _this, const sead::SafeString& path, u64 va1, u64 va2, u64 va3) -> sead::DirectResource* {
    if (StringContains(path.cstr(), ".bgyml")) {
        ReadCurrentBGYML(path.cstr());
        sead::DirectResource* directResource = orig(_this, path, va1, va2, va3);

        if (directResource == nullptr) {
            return nullptr;
        }
        
        if (sCurrentBGYML != nullptr) {
            MyDirectResource* resource = static_cast<MyDirectResource*>(directResource);
    
            u8* old = resource->swapRawData(static_cast<u8*>(sCurrentBGYML));
            resource->setRawSize(sCurrentBGYMLSize);

            if (old != nullptr) {
                delete old;
            }
    
            sCurrentBGYML = nullptr;
            sCurrentBGYMLSize = 0;
        }
        return directResource;
    }
    
    sead::DirectResource* directResource = orig(_this, path, va1, va2, va3);

    if (directResource == nullptr || !Config::instance().isLoaded()) {
        return directResource;
    }
    
    const char* compressedPaths[] = {
        "BancMapUnit/",
    };
    
    for (const char* item : compressedPaths) {
        if (StringContains(path.cstr(), item)) {
            MyDirectResource* resource = static_cast<MyDirectResource*>(directResource);
            ReloadCompressedData(resource, path.cstr());
            return directResource;
        }
    }

    return directResource;
};

HkTrampoline engine_resource_GeneralResourceFormat_initialize_hook = [](TrampolineStatic(), u64 _this, void* rawData, u32 rawSize, sead::Heap* resourceHeap) -> bool {
    if (sCurrentBGYML != nullptr) {
        return orig(_this, sCurrentBGYML, sCurrentBGYMLSize, resourceHeap);
    }

    return orig(_this, rawData, sCurrentBGYMLSize, resourceHeap);
};

void BlankedLevelCollisionHook(sead::BufferedSafeString& out, const char* fmt, const char* levelName) {
    if (Config::instance().isLoaded() && Config::instance().useGlobalBlankBPHSC() && StringContains(levelName, "Course")) {
        out.format("wonder-sdfs-assets/BLANK.bphsc");
    } else {
        out.format(fmt, levelName);
    }
}

extern "C" void hkMain() {
    nn_fs_OpenFile_hook.installAtSym<"_ZN2nn2fs8OpenFileEPNS0_10FileHandleEPKci">();
    ares_ResourceBinder_tryLoadSync_hook.installAtSym<"_ZN4ares14ResourceBinder11tryLoadSyncERKN4sead14SafeStringBaseIcEEPvPv">();
    engine_resource_GeneralResourceFormat_initialize_hook.installAtSym<"_ZN6engine8resource21GeneralResourceFormat10initializeEPvjPN4sead4HeapE">();
    hk::hook::writeBranchLinkAtSym<"CreateStaticCompound_format_call">(BlankedLevelCollisionHook);

    INFO_LOG("Installed all hooks!");
}
