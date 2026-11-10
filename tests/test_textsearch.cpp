#include <string>

#include "TestSupport.h"
#include "core/TextSearch.h"

namespace {

using libster::containsFolded;
using libster::foldForSearch;
using libster::test::TestContext;

void testAsciiFolding(TestContext& t) {
    CHECK(t, foldForSearch("Hello") == "hello");
    CHECK(t, foldForSearch("ABC xyz 123") == "abc xyz 123");
    CHECK(t, foldForSearch("") == "");
    CHECK(t, foldForSearch("no-change_42") == "no-change_42");
}

void testGermanUmlauts(TestContext& t) {
    CHECK(t, foldForSearch("Müller") == "muller");
    CHECK(t, foldForSearch("MÜLLER") == "muller");
    CHECK(t, foldForSearch("Bäcker") == "backer");
    CHECK(t, foldForSearch("BÄCKER") == "backer");
    CHECK(t, foldForSearch("König") == "konig");
    CHECK(t, foldForSearch("KÖNIG") == "konig");
}

void testSharpS(TestContext& t) {
    CHECK(t, foldForSearch("Straße") == "strasse");
    CHECK(t, foldForSearch("STRASSE") == "strasse");
    // Großes ẞ (U+1E9E) wird ebenfalls zu "ss".
    CHECK(t, foldForSearch("STRAẞE") == "strasse");
}

void testAccents(TestContext& t) {
    CHECK(t, foldForSearch("Crème brûlée") == "creme brulee");
    CHECK(t, foldForSearch("ÉCOLE") == "ecole");
    CHECK(t, foldForSearch("Ångström") == "angstrom");
}

void testInvalidUtf8(TestContext& t) {
    // Ungültige Bytes bleiben erhalten und dürfen nicht zum Absturz führen.
    const std::string broken = "\xFF\xFE";
    CHECK(t, foldForSearch(broken).size() == 2u);
    CHECK(t, foldForSearch(broken) == broken);

    // Abgeschnittene Sequenz (nur Lead-Byte).
    const std::string truncated = "A\xC3";
    CHECK(t, foldForSearch(truncated) == "a\xC3");
}

void testContainsFolded(TestContext& t) {
    CHECK(t, containsFolded("Müller", foldForSearch("muller")));
    CHECK(t, containsFolded("Müller", foldForSearch("müller")));
    CHECK(t, containsFolded("Müller", foldForSearch("MÜLLER")));
    CHECK(t, containsFolded("Müller", foldForSearch("ülle")));
    CHECK(t, containsFolded("Straße", foldForSearch("strasse")));
    CHECK(t, containsFolded("Straße", foldForSearch("STRASSE")));
    CHECK(t, !containsFolded("Müller", foldForSearch("schmidt")));
    CHECK(t, !containsFolded("", foldForSearch("a")));
}

} // namespace

void runTextSearchTests(libster::test::TestContext& t) {
    testAsciiFolding(t);
    testGermanUmlauts(t);
    testSharpS(t);
    testAccents(t);
    testInvalidUtf8(t);
    testContainsFolded(t);
}
