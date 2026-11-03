#include <cstring>
#include <iostream>

#include "TestSupport.h"
#include "core/Version.h"

void runPersistenceTests(libster::test::TestContext& t); // Definition in test_persistence.cpp

int main() {
    libster::test::TestContext t;

    CHECK(t, std::strcmp(libster::version(), "0.1.0") == 0);
    runPersistenceTests(t);

    if (t.passed()) {
        std::cout << "Alle Tests bestanden.\n";
        return 0;
    }

    std::cerr << t.failures() << " Test(s) fehlgeschlagen.\n";
    return 1;
}
