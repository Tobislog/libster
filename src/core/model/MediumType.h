#pragma once

#include <cstdint>

namespace libster {
    
enum class MediumType : std::uint8_t {BOOK = 1, CD = 2 };
static_assert(sizeof(MediumType) == 1, "muss 1 Byte im Record sein!");

} // namespace libster
