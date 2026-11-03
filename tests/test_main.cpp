#include <cstring>
#include <iostream>

#include "core/Version.h"

namespace {
int failures = 0;

void check(bool condition, const char* description) {
    if (condition) {
        std::cout << "[ OK ] " << description << '\n';
    } else {
        std::cout << "[ FAIL ] " << description << '\n';
        ++failures;
    }
}

#define CHECK(condition) check((condition), #condition)

} // namespace

int main() {
    CHECK(libster::version() != nullptr);
    CHECK(std::strcmp(libster::version(), "0.1.0") == 0);

    if (failures == 0) {
        std::cout << "Alle Tests bestanden.\n";
        return 0;
    }

    std::cerr << failures << " Test(s) fehlgeschlagen.\n";
    return 1;
}
