#pragma once

#include "ConfigParser.h"
#include "Log.h"

class Config {
    static constexpr u32 FOLDER_NAME_LEN = 256;

    static constexpr inline const char* bool_to_str(bool val) {
        return val ? "true" : "false";
    }
public:
    static Config& instance() {
        static Config inst;
        return inst;
    }

    static void printConfiguration() {
        if (!Config::instance().isLoaded()) {
            INFO_LOG("No configs.");
            return;
        }
        
        INFO_LOG("Config: PROJECT_ROMFS_FOLDER=%s", Config::instance().getFolderName());
        INFO_LOG("Config: USE_UNIVERSAL_BLANK_BPHSC=%s", bool_to_str(Config::instance().mUseGlobalBlankBPHSC));
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

        if (ConfigParser::find(contents, "USE_UNIVERSAL_BLANK_BPHSC", entry)) {
            if (!ConfigParser::getBool(entry, mUseGlobalBlankBPHSC)) {
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

    bool useGlobalBlankBPHSC() const {
        return mUseGlobalBlankBPHSC;
    }
private:
    bool mIsLoaded = false;
    char mFolderName[FOLDER_NAME_LEN] = { 0 };
    bool mUseGlobalBlankBPHSC = false;
};
