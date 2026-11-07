#pragma once

#include <cstdint>

namespace libster {

enum class ServiceError : std::uint8_t {
    None = 0,
    NotOpen,
    NotFound,
    AlreadyDeleted,
    HasOpenLoans,
    NotAvailable,
    InvalidData,
    Persistence
};

inline const char* toString(ServiceError error) noexcept {
    switch (error) {
        case ServiceError::None:            return "None";
        case ServiceError::NotOpen:         return "NotOpen";
        case ServiceError::NotFound:        return "NotFound";
        case ServiceError::AlreadyDeleted:  return "AlreadyDeleted";
        case ServiceError::HasOpenLoans:    return "HasOpenLoans";
        case ServiceError::NotAvailable:    return "NotAvailable";
        case ServiceError::InvalidData:     return "InvalidData";
        case ServiceError::Persistence:     return "Persistence";
    }
    return "Unknown";
}

} // namespace libster
