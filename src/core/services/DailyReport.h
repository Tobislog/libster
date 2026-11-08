#pragma once

#include <cstdint>

namespace libster {

struct DailyReport {
    std::uint32_t loanCount = 0;
    std::uint32_t returnCount = 0;
};    

} // namespace libster
