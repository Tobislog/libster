#include <string>

#include "TestSupport.h"
#include "core/persistence/CStringUtil.h"

void runPersistenceTests(libster::test::TestContext& t) {
    char buf[8]{};

    libster::copyToFixed(buf, "");
    CHECK(t, libster::fixedToString(buf).empty());

    libster::copyToFixed(buf, "abcdefg");
    CHECK(t, libster::fixedToString(buf) == "abcdefg");

    libster::copyToFixed(buf, "abcdefghij");
    CHECK(t, libster::fixedToString(buf) == "abcdefg");

    libster::copyToFixed(buf, std::string(200, 'x'));
    CHECK(t ,libster::fixedToString(buf) == std::string(7, 'x'));

    char unterminated[4] = {'A', 'B', 'C', 'D'};
    CHECK(t ,libster::fixedToString(unterminated) == "ABCD");


}
