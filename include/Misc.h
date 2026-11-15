#pragma once

#include <cstring>

constexpr bool StringContains(const char* string, const char* query) {
    return string != nullptr && query != nullptr && std::strstr(string, query) != nullptr;
}

/*

extern "C" void _init();

u64 GetOffs(u64 addr) {
    u64 binaryStart = ((u64)&_init) - 0x30;
    return addr - binaryStart;
}

void PrintLR() {
    u64 lr = (u64)__builtin_return_address(0);
    u64 lrOffs = getOffs(lr);
    hk::diag::logLine("Addr: 0x%" PRIx64 "\n", lrOffs);
}
    
*/
