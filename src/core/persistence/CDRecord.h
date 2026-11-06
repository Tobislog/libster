#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "core/model/Status.h"

namespace libster {

#pragma pack(push, 1)
struct CDRecord {
    // gemeinsamer Medium-Block, identisch zu BookRecord
    std::uint32_t   id;                 // 0
    char            title[128];         // 4
    char            publisher[64];      // 132
    char            genre[64];          // 196
    char            publishedOn[11];    // 260
    char            addedOn[11];        // 271
    char            location[64];       // 282
    std::uint32_t   price;              // 346
    char            recordedBy[64];     // 350
    Status          status;             // 414

    // CD-spezifisch
    char            performer[64];      // 415
    std::uint16_t   duration;           // 479
};
#pragma pack(pop)

static_assert(sizeof(CDRecord) == 481, "CDRecord muss genau 481 Bytes groß sein (Dateiformat!)");
static_assert(alignof(CDRecord) == 1, "pragma pack nicht aktiv");

static_assert(offsetof(CDRecord, id)          == 0);
static_assert(offsetof(CDRecord, title)       == 4);
static_assert(offsetof(CDRecord, publisher)   == 132);
static_assert(offsetof(CDRecord, genre)       == 196);
static_assert(offsetof(CDRecord, publishedOn) == 260);
static_assert(offsetof(CDRecord, addedOn)     == 271);
static_assert(offsetof(CDRecord, location)    == 282);
static_assert(offsetof(CDRecord, price)       == 346);
static_assert(offsetof(CDRecord, recordedBy)  == 350);
static_assert(offsetof(CDRecord, status)      == 414);
static_assert(offsetof(CDRecord, performer)   == 415);
static_assert(offsetof(CDRecord, duration)    == 479);

static_assert(std::is_trivially_copyable_v<CDRecord>);
static_assert(std::is_standard_layout_v<CDRecord>);

} // namespace libster
