#include <string>

#include "TestSupport.h"
#include "core/persistence/CStringUtil.h"

void runPersistenceTests(libster::test::TestContext& t) {
    char buf[8]{};
    char raw[8];
    for (char& c : raw) c = 'Z';

    libster::copyToFixed(buf, "");
    CHECK(t, libster::fixedToString(buf).empty());

    libster::copyToFixed(buf, "abcdefg");
    CHECK(t, libster::fixedToString(buf) == "abcdefg");

    libster::copyToFixed(buf, "abcdefghij");
    CHECK(t, libster::fixedToString(buf) == "abcdefg");

    libster::copyToFixed(buf, std::string(200, 'x'));
    CHECK(t, libster::fixedToString(buf) == std::string(7, 'x'));

    char unterminated[4] = {'A', 'B', 'C', 'D'};
    CHECK(t, libster::fixedToString(unterminated) == "ABCD");

    libster::copyToFixed(raw, "abcdefghij");
    CHECK(t, raw[7] == '\0');
    CHECK(t, raw[0] == 'a' && raw[6] == 'g');

    libster::copyToFixed(raw, "ab");
    CHECK(t, raw[2] == '\0' && raw[3] == '\0' && raw[7] == '\0');

    // Kein Test fuer Aliasing: src darf laut Vorbedingung nicht in dst zeigen.
    // `alias == ""` verglich ausserdem nur Array-Adressen, nicht den Inhalt.
}
