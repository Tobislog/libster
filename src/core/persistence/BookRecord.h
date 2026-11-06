#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "core/model/Status.h"

namespace libster {

#pragma pack(push, 1)
struct BookRecord {
    // gemeinsamer Medium-Block, identisch zu CDRecord
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

    // Book-spezifisch
    char            isbn[14];           // 415
    std::uint16_t   edition;            // 429
    char            authors[256];       // 431
    std::uint16_t   pageCount;          // 687
};
#pragma pack(pop)

static_assert(sizeof(BookRecord) == 689, "BookRecord muss 689 Bytes groß sein (Dateiformat!)");
static_assert(alignof(BookRecord) == 1, "pragma pack nicht aktiv");

static_assert(offsetof(BookRecord, id)          == 0);
static_assert(offsetof(BookRecord, title)       == 4);
static_assert(offsetof(BookRecord, publisher)   == 132);
static_assert(offsetof(BookRecord, genre)       == 196);
static_assert(offsetof(BookRecord, publishedOn) == 260);
static_assert(offsetof(BookRecord, addedOn)     == 271);
static_assert(offsetof(BookRecord, location)    == 282);
static_assert(offsetof(BookRecord, price)       == 346);
static_assert(offsetof(BookRecord, recordedBy)  == 350);
static_assert(offsetof(BookRecord, status)      == 414);
static_assert(offsetof(BookRecord, isbn)        == 415);
static_assert(offsetof(BookRecord, edition)     == 429);
static_assert(offsetof(BookRecord, authors)     == 431);
static_assert(offsetof(BookRecord, pageCount)   == 687);

static_assert(std::is_trivially_copyable_v<BookRecord>);
static_assert(std::is_standard_layout_v<BookRecord>);

} // namespace libster
