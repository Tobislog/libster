#include <cstring>
#include <iostream>

#include "TestSupport.h"
#include "core/Version.h"

void runPersistenceTests(libster::test::TestContext& t);
void runBorrowerTests(libster::test::TestContext& t);
void runBinaryFileTests(libster::test::TestContext& t);
void runModelTests(libster::test::TestContext& t);

int main() {
    libster::test::TestContext t;

    CHECK(t, std::strcmp(libster::version(), "0.1.0") == 0);
    runPersistenceTests(t);
    runBorrowerTests(t);
    runBinaryFileTests(t);
    runModelTests(t);

    if (t.passed()) {
        std::cout << "Alle Tests bestanden.\n";
        return 0;
    }

    std::cerr << t.failures() << " Test(s) fehlgeschlagen.\n";
    return 1;
}
