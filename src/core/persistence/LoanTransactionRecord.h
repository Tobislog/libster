#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "core/model/MediumType.h"

namespace libster {

#pragma pack(push, 1)
struct LoanTransactionRecord {
    std::uint32_t    id;                 // 0
    std::uint32_t    borrowerId;         // 4
    std::uint32_t    mediumId;           // 8
    MediumType      type;               // 12
    std::int64_t    loanDate;           // 13   Epoch-Sekunden
    std::int64_t    returnDate;         // 21   0 = noch nicht zurückgegeben
    char            recordedBy[64];     // 29
    
};
#pragma pack(pop)

static_assert(sizeof(LoanTransactionRecord) == 93, "LoanTransactionRecord muss 93 Bytes sein (Dateiformat!)");
static_assert(alignof(LoanTransactionRecord) == 1, "pragma pack nicht aktiv");

static_assert(offsetof(LoanTransactionRecord, id)           == 0);
static_assert(offsetof(LoanTransactionRecord, borrowerId)   == 4);
static_assert(offsetof(LoanTransactionRecord, mediumId)     == 8);
static_assert(offsetof(LoanTransactionRecord, type)         == 12);
static_assert(offsetof(LoanTransactionRecord, loanDate)     == 13);
static_assert(offsetof(LoanTransactionRecord, returnDate)   == 21);
static_assert(offsetof(LoanTransactionRecord, recordedBy)   == 29);

static_assert(std::is_trivially_copyable_v<LoanTransactionRecord>);
static_assert(std::is_standard_layout_v<LoanTransactionRecord>);

} // namespace libster
