#pragma once

#include "ConfigParser.h"

class Config {
    static constexpr u32 FOLDER_NAME_LEN = 256;
public:
    static Config& instance() {
        static Config inst;
        return inst;
    }

    bool load(const char* contents) {
        if (mIsLoaded) {
            return true;
        }

        ConfigParser::Entry entry;

        if (ConfigParser::find(contents, "PROJECT_ROMFS_FOLDER", entry)) {
            if (!ConfigParser::getString(entry, mFolderName, FOLDER_NAME_LEN)) {
                return false;
            }
        }

        mIsLoaded = true;
        return true;
    }

    bool isLoaded() const {
        return mIsLoaded;
    }

    const char* getFolderName() const {
        return mFolderName;
    }
private:
    bool mIsLoaded = false;
    char mFolderName[FOLDER_NAME_LEN] = { 0 };
};
