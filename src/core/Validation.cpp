#include "core/Validation.h"

#include <cstdint>

#include "core/TimeUtil.h"

namespace libster {

bool isNonBlank(std::string_view value) noexcept {
    for (char c : value) {
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') return true;
    }
    return false;
}

bool fitsField(std::string_view value, std::size_t maxChars) noexcept {
    return value.size() <= maxChars;
}

bool isValidEmail(std::string_view value) noexcept {
    if (value.empty()) return false;
    for (char c : value) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            return false;
        }
    }

    const std::size_t at = value.find('@');
    if (at == std::string_view::npos || at == 0) return false;
    if (value.find('@', at + 1) != std::string_view::npos) return false;

    const std::string_view domain = value.substr(at + 1);
    const std::size_t dot = domain.rfind('.');
    if (dot == std::string_view::npos || dot == 0) return false;

    return dot + 1 < domain.size();
}

std::string normalizeIsbn(std::string_view value) {
    std::string result;
    result.reserve(value.size());

    for (char c : value) {
        if (c == '-' || c == ' ') continue;
        if (c == 'x' || c == 'X') {
            result.push_back('X');
            continue;
        }
        result.push_back(c);
    }
    return result;
}

bool isValidIsbn(std::string_view value) noexcept {
    const std::string digits = normalizeIsbn(value);

    if (digits.size() == 10) {
        int sum = 0;

        for (std::size_t i = 0;i < digits.size(); ++i) {
            const char c = digits[i];
            int digit = 0;

            if (c == 'X') {
                if (i != 9) return false;
                digit = 10;
            } else if (c >= '0' && c <= '9') {
                digit = c - '0';
            } else {
                return false;
            }
            sum += digit * static_cast<int>(10 - i);
        }
        return sum % 11 == 0;
    }

    if (digits.size() == 13) {
        int sum = 0;
        for (std::size_t i = 0; i < digits.size(); ++i) {
            const char c =  digits[i];
            if (c < '0' || c > '9') return false;

            const int digit = c - '0';
            sum += (i % 2 == 0) ? digit : digit * 3;
        }

        return sum % 10 == 0;
    }
    return false;
}

bool isValidIsoDate(std::string_view value) {
    std::int64_t dayStart = 0;
    std::int64_t dayEnd = 0;
    return isoDateToDayRange(value, dayStart, dayEnd);
}

} // namespace libster
