#pragma once

#include <cstdint>

namespace libster {
enum class FileType : std::uint16_t {
    Book = 1,
    CD = 2,
    Borrower = 3,
    LoanTransaction = 4
};

} // namespace libster

