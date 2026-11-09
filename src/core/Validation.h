#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace libster {

bool isNonBlank(std::string_view value) noexcept;

// maxChars =  maximale Zeichenzahl ohne \0 (also Feldgröße - 1).
bool fitsField(std::string_view value, std::size_t maxChars) noexcept;

bool isValidEmail(std::string_view value) noexcept;

// Akzeptiert ISBN-10 und ISBN-13, optional mit Bindestrichen/Leerzeichen.
bool isValidIsbn(std::string_view value) noexcept;

bool isValidIsoDate(std::string_view value);

std::string normalizeIsbn(std::string_view value);

} // namespace libster
