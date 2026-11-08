#include <cstdint>
#include <string>

#include "TestSupport.h"
#include "core/TimeUtil.h"

namespace {

using libster::epochToIsoDate;
using libster::isoDateToDayRange;
using libster::nowEpochSeconds;
using libster::test::TestContext;

void testNowEpochSeconds(TestContext& t) {
    CHECK(t, nowEpochSeconds() > 0);
}

void testIsoDateRoundtrip(TestContext& t) {
    std::int64_t dayStart = 0;
    std::int64_t dayEnd = 0;

    CHECK(t, isoDateToDayRange("2026-11-08", dayStart, dayEnd));
    CHECK(t, dayStart > 0);
    CHECK(t, dayEnd > dayStart);

    CHECK(t, epochToIsoDate(dayStart) == "2026-11-08");
    CHECK(t, epochToIsoDate(dayEnd - 1) == "2026-11-08");
    CHECK(t, epochToIsoDate(dayEnd) != "2026-11-08");
}

void testLeapDay(TestContext& t) {
    std::int64_t dayStart = 0;
    std::int64_t dayEnd = 0;

    CHECK(t, isoDateToDayRange("2024-02-29", dayStart, dayEnd));
    CHECK(t, epochToIsoDate(dayStart) == "2024-02-29");
    CHECK(t, epochToIsoDate(dayEnd - 1) == "2024-02-29");
}

void testInvalidIsoDates(TestContext& t) {
    const char* invalid[] = {
        "",
        "2026-11",
        "2026-11-8",
        "2026/11/08",
        "abcd-ef-gh",
        "2026-00-01",
        "2026-13-01",
        "2026-11-00",
        "2026-02-30",
        "2023-02-29"
    };

    for (const char* value : invalid) {
        std::int64_t dayStart = 42;
        std::int64_t dayEnd = 43;
        CHECK(t, !isoDateToDayRange(value, dayStart, dayEnd));
        CHECK(t, dayStart == 0);
        CHECK(t, dayEnd == 0);
    }
}

} // namespace

void runTimeUtilTests(libster::test::TestContext& t) {
    testNowEpochSeconds(t);
    testIsoDateRoundtrip(t);
    testLeapDay(t);
    testInvalidIsoDates(t);
}
