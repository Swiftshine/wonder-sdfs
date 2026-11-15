#pragma once

#include <hk/types.h>
#include <string.h>
#include <stdlib.h>

class ConfigParser {
public:
    struct Entry {
        const char* key;
        const char* value;
    };

    static bool find(const char* contents, const char* key, Entry& entry) {
        if (contents == nullptr || key == nullptr) {
            return false;
        }

        const char* line = contents;

        while (*line != 0) {
            // skip whitespace
            while (
                *line == ' ' ||
                *line == '\t' ||
                *line == '\r' ||
                *line == '\n'
            ) {
                line++;
            }

            // skip comments
            if (*line == '#') {
                while (*line != 0 && *line != '\n') {
                    line++;
                }

                continue;
            }

            if (*line == 0) {
                break;
            }

            const char* separator = strchr(line, '=');

            if (separator == nullptr) {
                break;
            }

            // trim whitespace before '='
            const char* keyEnd = separator;

            while (keyEnd > line && (keyEnd[-1] == ' ' || keyEnd[-1] == '\t')) {
                keyEnd--;
            }

            u32 keyLen = static_cast<u32>(keyEnd - line);
            
            if (strlen(key) == keyLen && strncmp(line, key, keyLen) == 0) {
                const char* value = separator + 1;

                while (*value == ' ' || *value == '\t') {
                    value++;
                }

                bool quoted = *value == '"';

                if (quoted) {
                    value++;
                }

                const char* valueEnd = value;

                if (quoted) {
                    while (*valueEnd != 0 && *valueEnd != '"') {
                        valueEnd++;
                    }
                } else {
                    while (*valueEnd != 0 && *valueEnd != '\n' && *valueEnd != '\r' && *valueEnd != '#') {
                        valueEnd++;
                    }

                    while (valueEnd > value && (valueEnd[-1] == ' ' || valueEnd[-1] == '\t')) {
                        valueEnd--;
                    }
                }

                entry.key = line;
                entry.value = value;

                mValueLength = static_cast<u32>(valueEnd - value);

                return true;
            }
            
            // next line
            while (*line != 0 && *line != '\n') {
                line++;
            }
        }
        
        return false;
    }
        
    static bool getString(const Entry& entry, char* output, u32 outputSize) {
        if (output == nullptr || outputSize == 0) {
            return false;
        }

        if (mValueLength >= outputSize) {
            return false;
        }

        memcpy(output, entry.value, mValueLength);
        output[mValueLength] = '\0';

        return true;
    }

    // static bool getInt(const Entry& entry, s32& output) {
    //     if (entry.value == nullptr || *entry.value == 0) {
    //         return false;
    //     }

    //     char* end = nullptr;
    //     s32 value = strtol(entry.value, &end, 10);

    //     if (end == entry.value || *end != '\0') {
    //         return false;
    //     }

    //     output = value;
    //     return true;
    // }

    static bool getBool(const Entry& entry, bool& output) {
        if ((mValueLength == 4 && strncmp(entry.value, "true", 4) == 0) ||
            (mValueLength == 1 && strncmp(entry.value, "1", 1) == 0)) {
            output = true;
            return true;
        }

        if ((mValueLength == 5 && strncmp(entry.value, "false", 5) == 0) ||
            (mValueLength == 1 && strncmp(entry.value, "0", 1) == 0)) {
            output = false;
            return true;
        }

        return false;
    }

private:
    static inline u32 mValueLength = 0;
};
