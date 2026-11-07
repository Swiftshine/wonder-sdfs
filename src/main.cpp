#include <hk/diag/diag.h>
#include <hk/hook/Trampoline.h>

#include <nn/fs.h>

#include <cstring>

static bool sIsCardMounted = false;
const char* WONDER_SD_CARD_PATH = "sd:/wonder-sdfs/";

#define VERBOSE true

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

HkTrampoline<nn::Result, nn::fs::FileHandle*, const char*, int> openFileHook = [](nn::fs::FileHandle* out, const char* filePath, int mode) -> nn::Result {
    if (!sIsCardMounted) {
        if (nn::fs::MountSdCard("sd").IsSuccess()) {
            sIsCardMounted = true;
            hk::diag::logLine("Mounted SD card!");
        } else {
            hk::diag::logLine("Failed to mount SD card.");
        }
    }

    if (sIsCardMounted) {
        if (std::strncmp(filePath, "content:/", 9) == 0) {
            char path[1024] { 0 };
            strcat(path, WONDER_SD_CARD_PATH);
            strcat(path, filePath + 9);
            
            if (fileExists(path)) {
                hk::diag::logLine("[From SD]: Opening %s", path);
                return openFileHook.orig(out, path, mode);
            }
        }
    }

    VERBOSE_LOG("[From ROMFS]: Opening %s", filePath);
    return openFileHook.orig(out, filePath, mode);
};

extern "C" void hkMain() {
    hk::diag::logLine("Installing FS hacks...");
    openFileHook.installAtSym<"_ZN2nn2fs8OpenFileEPNS0_10FileHandleEPKci">();
    hk::diag::logLine("FS hacks installed.");
}
