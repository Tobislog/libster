#pragma once

#include <cstdint>

namespace libster {

enum class BorrowerStatus : std::uint8_t {
    ACTIVE = 0,
    DELETED = 1,
};

static_assert(sizeof(BorrowerStatus) == 1, "muss 1 Byte im Record sein!");

} // namespace libster
