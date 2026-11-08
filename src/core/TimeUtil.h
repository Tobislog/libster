#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace libster {
// Aktuelle Zeit als sekunden seit der Epoche
std::int64_t nowEpochSeconds();

// IsoDate -> halboffenes Tagesintervall [dayStart, dayEnd]
bool isoDateToDayRange(std::string_view isoDate, std::int64_t& dayStart, std::int64_t& dayEnd);


std::string epochToIsoDate(std::int64_t epochSeconds);

} // namespace libster
