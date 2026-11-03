#pragma once

#include <cstdint>
#include <cstddef>      // offsetof
#include <type_traits>  // is_trivially_copyable, is_standard_layout

#include "core/persistence/FileType.h"

namespace libster {

inline constexpr std::uint32_t kFileMagic = 0x4C425354; // "LBST"
inline constexpr std::uint16_t kFileFormatVersion = 1;
inline constexpr std::uint32_t kFirstId = 1;

#pragma pack(push, 1)
struct FileHeader {
    std::uint32_t magic;         // 0 kFileMagic
    std::uint16_t version;       // 4 kFileFormatVersion
    FileType type;               // 6 welche Entity
    std::uint32_t recordSize;    // 8 sizeof(T)
    std::uint32_t nextId;        // 12 nächste freie ID, Start 1
};
#pragma pack(pop)

static_assert(sizeof(FileHeader) == 16, "FileHeader muss 16 Bytes sein");
static_assert(alignof(FileHeader) == 1, "pragma pack nicht aktiv");
static_assert(offsetof(FileHeader, magic)       == 0);
static_assert(offsetof(FileHeader, version)     == 4);
static_assert(offsetof(FileHeader, type)        == 6);
static_assert(offsetof(FileHeader, recordSize)  == 8);
static_assert(offsetof(FileHeader, nextId)      == 12);
static_assert(std::is_trivially_copyable_v<FileHeader>);
static_assert(std::is_standard_layout_v<FileHeader>);

constexpr FileHeader makeHeader(FileType type, std::uint32_t recordSize) {
    FileHeader h{};
    h.magic = kFileMagic;
    h.version = kFileFormatVersion;
    h.type = type;
    h.recordSize = recordSize;
    h.nextId = kFirstId; // Start 1 nicht 0
    return h;
}

} // namespace libster
