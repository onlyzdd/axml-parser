#include <axml/string_pool.h>
#include <codecvt>
#include <locale>
#include <climits>

using namespace axml;

size_t StringPool::decodeLength(const uint8_t* t, size_t len, uint8_t & size) {
    assertSize(len >= 1);
    size_t ret = t[0];
    size = 1;
    if (ret & 0x80) {
        uint8_t u16len_fix = t[1], u8len = t[2];
        size += 2;
        if (u8len & 0x80) {
            ret = u8len;
        }
        else {
            uint8_t u8len_fix = t[3];
            size += 1;
            ret = ((u8len & 0x7F) << 8) | u8len_fix;
        }
    } else {
        uint8_t u8len = t[1];
        size += 1;
        if (u8len & 0x80) {
            uint8_t u8len_fix = t[2];
            size += 1;
            ret = ((u8len & 0x7F) << 8) | u8len_fix;
        } else {
            ret = u8len;
        }
    }
    return ret;
}

size_t StringPool::decodeLength(const uint16_t* t, size_t len, uint8_t & size) {
    assertSize(len >= 1 * sizeof(uint16_t));
    size_t ret = t[0];
    size = 1;
    if (ret & 0x8000) {
        uint8_t u16len_fix = t[1];
        size += 1;
        assertSize(len >= 2 * sizeof(uint16_t));
        ret = ((ret & 0x7FFF) << 16) | u16len_fix;
    }
    return ret;
}

std::string StringPool::getStringAtOffset(size_t offset) const {
    assertHasHeader();
    assertSize(offset < header->size);
    if (utf8) {
        uint8_t* ptr = (uint8_t*) ((size_t) (void*) header + header->stringsStart + offset);
        uint8_t lenSize;
        size_t size = decodeLength(ptr, header->size - offset, lenSize);
        assertSize(offset + size + lenSize <= header->size);
        return std::string((char*) &ptr[lenSize], size);
    } else {
        uint16_t* ptr = (uint16_t*) ((size_t) (void*) header + header->stringsStart + offset);
        uint8_t lenSize;
        size_t size = decodeLength(ptr, header->size - offset, lenSize);
        assertSize(offset + (size + lenSize) * sizeof(uint16_t) <= header->size);
        std::u16string str((char16_t*) &ptr[lenSize], size);
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> cvt;
        return cvt.to_bytes(str);
    }
}

std::string StringPool::getString(size_t index) const {
    assertHasHeader();
    if (index == UINT_MAX)
        return std::string();
    if (index >= header->stringCount)
        throw std::out_of_range("Index out of bounds");
    uint32_t* entries = (uint32_t*) ((size_t) (void*) header + header->headerSize);
    assertSize((index + 1) * sizeof(uint64_t) <= header->size);
    return getStringAtOffset(entries[index]);
}
