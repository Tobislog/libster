#pragma once

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>

namespace libster {

// Vorbedingung: src darf nicht in dst hineinzeigen (Aliasing wuerde die Quelle
// durch das vorherige Nullen zerstoeren).
template <std::size_t N>
void copyToFixed(char (&dst)[N], std::string_view src) noexcept {
    static_assert(N > 0, "Array darf nicht leer sein");
    const std::size_t count = std::min(src.size(), N - 1);
    std::fill(dst, dst + N, '\0');
    std::copy_n(src.begin(), count, dst);
}

template <std::size_t N>
std::string fixedToString(const char (&src)[N]) {
    const char* end = std::find(src, src + N, '\0');
    return std::string(src, static_cast<std::size_t>(end - src));
}

} // namespace libster

