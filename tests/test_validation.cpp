#include <string>

#include "TestSupport.h"
#include "core/Validation.h"

namespace {

using libster::fitsField;
using libster::isNonBlank;
using libster::isValidEmail;
using libster::isValidIsbn;
using libster::isValidIsoDate;
using libster::normalizeIsbn;
using libster::test::TestContext;

void testIsNonBlank(TestContext& t) {
    CHECK(t, !isNonBlank(""));
    CHECK(t, !isNonBlank("   "));
    CHECK(t, !isNonBlank("\t\r\n"));
    CHECK(t, isNonBlank("a"));
    CHECK(t, isNonBlank(" a "));
}

void testFitsField(TestContext& t) {
    CHECK(t, fitsField("", 0));
    CHECK(t, fitsField("a", 1));
    CHECK(t, fitsField("abc", 3));
    CHECK(t, !fitsField("abcd", 3));
}

void testValidEmails(TestContext& t) {
    CHECK(t, isValidEmail("anna@example.org"));
    CHECK(t, isValidEmail("a.b+c@sub.example.org"));
    CHECK(t, isValidEmail("x@y.z"));
}

void testInvalidEmails(TestContext& t) {
    CHECK(t, !isValidEmail(""));
    CHECK(t, !isValidEmail("anna"));
    CHECK(t, !isValidEmail("anna@"));
    CHECK(t, !isValidEmail("@example.org"));
    CHECK(t, !isValidEmail("anna@example"));
    CHECK(t, !isValidEmail("anna@@example.org"));
    CHECK(t, !isValidEmail("anna example@example.org"));
    CHECK(t, !isValidEmail("anna@example."));
    CHECK(t, !isValidEmail("anna@.org"));
}

void testValidIsbn10(TestContext& t) {
    CHECK(t, isValidIsbn("0306406152"));
    CHECK(t, isValidIsbn("0-306-40615-2"));
    CHECK(t, isValidIsbn("097522980X"));
    CHECK(t, isValidIsbn("097522980x"));
}

void testInvalidIsbn10(TestContext& t) {
    CHECK(t, !isValidIsbn("0306406153"));
    CHECK(t, !isValidIsbn("X306406152"));
    CHECK(t, !isValidIsbn("030640615X"));
}

void testValidIsbn13(TestContext& t) {
    CHECK(t, isValidIsbn("9780306406157"));
    CHECK(t, isValidIsbn("978-0-306-40615-7"));
    CHECK(t, isValidIsbn("978 0 306 40615 7"));
}

void testInvalidIsbn13(TestContext& t) {
    CHECK(t, !isValidIsbn("9780306406158"));
    CHECK(t, !isValidIsbn("978030640615X"));
}

void testInvalidIsbnLengths(TestContext& t) {
    CHECK(t, !isValidIsbn(""));
    CHECK(t, !isValidIsbn("123"));
    CHECK(t, !isValidIsbn("ISBN-A"));
    CHECK(t, !isValidIsbn("12345678901"));
}

void testNormalizeIsbn(TestContext& t) {
    CHECK(t, normalizeIsbn("0-306-40615-2") == "0306406152");
    CHECK(t, normalizeIsbn("978 0 306 40615 7") == "9780306406157");
    CHECK(t, normalizeIsbn("097522980x") == "097522980X");
}

void testIsoDates(TestContext& t) {
    CHECK(t, isValidIsoDate("2026-11-09"));
    CHECK(t, isValidIsoDate("2024-02-29"));
    CHECK(t, !isValidIsoDate("2023-02-29"));
    CHECK(t, !isValidIsoDate("2026-13-01"));
    CHECK(t, !isValidIsoDate("2026-02-30"));
    CHECK(t, !isValidIsoDate("09.11.2026"));
    CHECK(t, !isValidIsoDate(""));
}

} // namespace

void runValidationTests(libster::test::TestContext& t) {
    testIsNonBlank(t);
    testFitsField(t);
    testValidEmails(t);
    testInvalidEmails(t);
    testValidIsbn10(t);
    testInvalidIsbn10(t);
    testValidIsbn13(t);
    testInvalidIsbn13(t);
    testInvalidIsbnLengths(t);
    testNormalizeIsbn(t);
    testIsoDates(t);
}
