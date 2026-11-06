#pragma once

#include <cstdint>

namespace libster {

enum class Status : std::uint8_t { AVAILABLE = 0, LOANED = 1, DELETED = 2 };
static_assert(sizeof(Status) == 1, "muss 1 Byte im Record sein!");    

} // namespace libster
