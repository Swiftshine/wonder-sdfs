#pragma once

#include <hk/types.h>
#include <prim/seadSafeString.h>
#include <resource/seadResource.h>

namespace ares {

class ResourceMgr;
class ResourceUnit;

// size unk
class ResourceBinder {
public:
    sead::DirectResource* getResource() const;
    sead::DirectResource* tryLoadSync(const sead::SafeString& path, ...);
    void finalize();
    bool isResourceReady() const;
public:
    u8 getCacheFlags() const {
        return mCacheFlags;
    }
    
    void setCacheFlags(u8 flags) {
        mCacheFlags = flags;
    }

    ResourceUnit* getResourceUnit() const {
        return mResourceUnit;
    }

    void setResourceUnit(ResourceUnit* unit) {
        mResourceUnit = unit;
    }
private:
    u8 mCacheFlags;
    u32 mStatus;
    ResourceUnit* mResourceUnit;
};
// size unk
class ResourceUnit {
public:
    struct InitArg {
        InitArg() = default;

        bool _0 = false;
        bool _1 = false;
        bool _2 = false;
        bool _3 = false;
        bool _4 = false;
        void* handle = nullptr;
        void* cache = nullptr;
        void* _18 = nullptr;
        void* _20 = nullptr;
        void* heap = nullptr;
        void* load_req = nullptr;
        void* arena = nullptr;
        u64 alloc_size = 0;
        sead::SafeString path = "";
    };
    // static_assert(sizeof(InitArg) == 0x50);
public:
    void adjustRefCount(s32 amt);
    void tryBusyLoad(const sead::SafeString& path);
    // Returns if the resource unit was removed
    bool removeIfOutOfReference();
    sead::DirectResource* tryLoadFile(const sead::SafeString& path);
    void finalize();

    // void loadFile(void* arg2, const char* path, void* arg4, void* arg5, void* arg6, void* arg7, void* arg8);

    bool queueLoad();
    bool initializeResource(void*);
    bool loadFile();
    void setFile(void* raw);
public:
    void setCacheFlags(u8 flags) {
        mCacheFlags = 0;
    }

    u32 getStatusFlags() const {
        return mStatusFlags;
    }

    sead::DirectResource* getResource() const {
        return mResource;
    }

    void setResource(sead::DirectResource* resource) {
        mResource = resource;
    }

    const char* getResourcePath() const {
        return mResourcePath;
    }

    sead::FileDevice* getFileDevice() const {
        return mFileDevice;
    }

    u32 getRefCount() const {
        return mRefCount;
    }

    void setRefCount(u32 count) {
        mRefCount = count;
    }

    s32 getAlignment() const {
        return mAlignment;
    }

    sead::Heap* getResourceHeap() const {
        return mResourceHeap;
    }

private:
    u8 _0;
    bool mIsInited;
    u8 _2;
    u8 mCacheFlags;
    u16 mFlags;
    u32 mStatusFlags;
    u32 mStatus;
    void* mArena;
    void* mArean1;
    void* mArena2;
    sead::DirectResource* mResource;
    u8 _30[0x60 - 0x30];
    ResourceMgr* mResourceMgr;
    u8 _68[0x78 - 0x68];
    sead::FileDevice* mFileDevice;
    u8 _80[8];
    u32 mRefCount;
    u8 _8c[8];
    s32 mAlignment;
    u8 _98[0x2D8 - 0x98];
    const char* mResourcePath;
    u32 mResourcePathLength;
    u8 _2e4[0x388 - 0x2E4];
    sead::Heap* mResourceHeap;
};
}
