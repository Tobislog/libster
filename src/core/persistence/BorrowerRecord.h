#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "core/model/BorrowerStatus.h"

namespace libster {

#pragma pack(push, 1)
struct BorrowerRecord {
    std::uint32_t   id;             // 0
    char            lastName[64];   // 4
    char            firstName[64];  // 68
    char            email[128];     // 132
    char            address[128];   // 260
    char            recordedOn[11]; // 388  "YYYY-MM-DD" + '\0'
    char            recordedBy[64]; // 399
    BorrowerStatus  status;         // 463
};
#pragma pack(pop)

static_assert(sizeof(BorrowerRecord) == 464, "BorrowerRecord muss 464 Bytes sein (Dateiformat!)");
static_assert(offsetof(BorrowerRecord, id)          == 0);
static_assert(offsetof(BorrowerRecord, lastName)    == 4);
static_assert(offsetof(BorrowerRecord, firstName)   == 68);
static_assert(offsetof(BorrowerRecord, email)       == 132);
static_assert(offsetof(BorrowerRecord, address)     == 260);
static_assert(offsetof(BorrowerRecord, recordedOn)  == 388);
static_assert(offsetof(BorrowerRecord, recordedBy)  == 399);
static_assert(offsetof(BorrowerRecord, status)      == 463);
static_assert(alignof(BorrowerRecord) == 1);
static_assert(std::is_trivially_copyable_v<BorrowerRecord>);
static_assert(std::is_standard_layout_v<BorrowerRecord>);

} // namespace libster
