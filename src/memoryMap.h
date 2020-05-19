#pragma once

#include <cstdint>
#include <wtypes.h>
#include <aclapi.h>
#include <string>

const uint32_t AGENT_MAX_MSGLEN = 8192;

class MemoryMap {
public:

    MemoryMap(const std::string& inName)
        : filemap (INVALID_HANDLE_VALUE)
        , mLength (AGENT_MAX_MSGLEN)
        , name (inName)
        , pos (0)
    {

    }

    void open() {
        std::wstring stemp = std::wstring(name.begin(), name.end());
        filemap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, mLength, stemp.c_str());

        if (filemap == INVALID_HANDLE_VALUE) {
            return;
        }

        view = MapViewOfFile(filemap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    }

    void seek(int inPos) {
        pos = inPos;
    }

    void write(uint8_t* inData, uint32_t len) {
        if (!inData) {
            return;
        }

        if (pos + len >= mLength) {
            return;
        }

        uint8_t* dest = (uint8_t*)view + pos;
        size_t length = len;
        RtlMoveMemory(dest, inData, length);

        pos += len;
    }

    uint8_t* read(uint32_t len) {
        uint8_t* out = (uint8_t*)calloc(len, sizeof(uint8_t));

        uint8_t* source = (uint8_t*)view + pos;
        size_t length = len;
        RtlMoveMemory(out, source, length);

        pos += len;
        return out;
    }

    void close() {
        UnmapViewOfFile(view);
        CloseHandle(filemap);
    }

    std::string getName() {
        return name;
    }

private:
    HANDLE filemap;
    size_t mLength = AGENT_MAX_MSGLEN;
    std::string name;
    int pos;

    LPVOID view;
};
