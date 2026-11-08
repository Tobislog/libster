#include "TimeUtil.h"

#include <chrono>
#include <cstdio>
#include <ctime>

namespace libster {

namespace {

bool parseFixedInt(std::string_view text, std::size_t offset, std::size_t length, int& value) noexcept {
    int result = 0;
    for (std::size_t i = 0; i < length; ++i) {
        const char c = text[offset +  i];
        if (c < '0' || c > '9') return false;
        result = result * 10 + (c - '0');
    }
    value = result;
    return true;
}

std::tm toLocalTime(std::time_t value) {
    std::tm result{};
    #if defined(_WIN32)
        localtime_s(&result, &value);
    #else
        localtime_r(&value, &result);
    #endif
        return result;
}

} // namespace

std::int64_t nowEpochSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

bool isoDateToDayRange(std::string_view isoDate, std::int64_t& dayStart, std::int64_t& dayEnd) {
    dayStart = 0;
    dayEnd = 0;

    if (isoDate.size() != 10 || isoDate[4] != '-' || isoDate[7] != '-') return false;
    int year = 0;
    int month = 0;
    int day = 0;

    if (!parseFixedInt(isoDate, 0, 4, year) ||
        !parseFixedInt(isoDate, 5, 2, month) ||
        !parseFixedInt(isoDate, 8, 2, day)) {
            return false;
    }

    std::tm start{};
    start.tm_year = year -1900;
    start.tm_mon = month -1;
    start.tm_mday = day;
    start.tm_hour = 0;
    start.tm_min = 0;
    start.tm_sec = 0;
    start.tm_isdst = -1;

    const std::time_t startTime = std::mktime(&start);
    if (startTime == static_cast<std::time_t>(-1)) return false;

    // mktime normalisiert ungültige Daten. Ist nachfolgende Prüfung falsch, war das Datum falsch.
    if (start.tm_year != year - 1900 ||
        start.tm_mon != month -1 ||
        start.tm_mday != day) {
        return false;
    }

    // Tagesende über mktime statt "+86400", damit Sommer-/Winterzeit korrekt bleibt.
    std::tm next = start;
    next.tm_mday += 1;
    next.tm_isdst = -1;

    const std::time_t endTime = std::mktime(&next);
    if (endTime == static_cast<std::time_t>(-1)) return false;

    dayStart = static_cast<std::int64_t>(startTime);
    dayEnd = static_cast<std::int64_t>(endTime);
    return true;
}

std::string epochToIsoDate(std::int64_t epochSeconds) {
    const std::time_t value = static_cast<std::time_t>(epochSeconds);
    const std::tm local = toLocalTime(value);
    
    char buffer[16] = {};
    const int written = std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
    if (written < 0) return {};
    return std::string(buffer);
}

} // namespace libster
