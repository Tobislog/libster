#pragma once

#include <cstdint>

namespace libster {

enum class IoError : std::uint8_t {
    None = 0,
    CannotOpen,
    NotOpen,
    CannotRead,
    CannotWrite,
    InvalidHeader,      // Datei zu kurz oder nextId ungueltig
    InvalidMagic,
    InvalidVersion,
    InvalidType,
    InvalidRecordSize,
    InvalidId,
    RecordMismatch      // Record-ID passt nicht zur Position
};

inline const char* toString(IoError error) noexcept {
    switch (error) {
        case IoError::None:             return "None";
        case IoError::CannotOpen:       return "CannotOpen";
        case IoError::NotOpen:          return "NotOpen";
        case IoError::CannotRead:       return "CannotRead";
        case IoError::CannotWrite:      return "CannotWrite";
        case IoError::InvalidHeader:    return "InvalidHeader";
        case IoError::InvalidMagic:     return "InvalidMagic";
        case IoError::InvalidVersion:   return "InvalidVersion";
        case IoError::InvalidType:      return "InvalidType";
        case IoError::InvalidRecordSize:return "InvalidRecordSize";
        case IoError::InvalidId:        return "InvalidId";
        case IoError::RecordMismatch:   return "RecordMismatch";
    }
    return "Unknown";
}
} // namespace libster
