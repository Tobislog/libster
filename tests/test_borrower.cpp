#include <string>

#include "TestSupport.h"
#include "core/model/Borrower.h"
#include "core/persistence/CStringUtil.h"

namespace {

using libster::Borrower;
using libster::BorrowerRecord;
using libster::BorrowerStatus;
using libster::kUnassignedId;

Borrower makeSampleBorrower(BorrowerStatus status = BorrowerStatus::ACTIVE) {
    return Borrower{
        7, 
        "Max", 
        "Mustermann", 
        "m@example.com", 
        "Musterstraße 7", 
        "2026-11-04", 
        "Admin", 
        status
    };
}

void testActiveRoundtrip(libster::test::TestContext& t) {
    const Borrower original = makeSampleBorrower();
    const auto record = original.toRecord();
    const auto restored = Borrower::fromRecord(record);

    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, *restored == original);
    CHECK(t, restored->getStatus() == BorrowerStatus::ACTIVE);
}

void testDeletedRoundtrip(libster::test::TestContext& t) {
    const Borrower original = makeSampleBorrower(BorrowerStatus::DELETED);
    const auto restored = Borrower::fromRecord(original.toRecord());

    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, *restored == original);
    CHECK(t, restored->getStatus() == BorrowerStatus::DELETED);
}

void testEmptyStrings(libster::test::TestContext& t) {
    const Borrower original{kUnassignedId, "", "", "", "", "", ""};
    const auto restored = Borrower::fromRecord(original.toRecord());

    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, *restored == original);
    CHECK(t, restored->getLastName().empty());
    CHECK(t, restored->getRecordedBy().empty());
}

void testMaxLengthStrings(libster::test::TestContext& t) {
    const Borrower original{
        1,
        std::string(63, 'L'),   // char[64]
        std::string(63, 'F'),   // char[64]
        std::string(127, 'E'),  // char[128]
        std::string(127, 'A'),  // char[128]
        "2026-11-04",           // char[11], 10 Nutzzeichen
        std::string(63, 'B')    // char[64]
    };

    const auto record = original.toRecord();
    const auto restored = Borrower::fromRecord(record);

    CHECK(t, record.lastName[63] == '\0');
    CHECK(t, record.email[127] == '\0');
    CHECK(t, record.recordedOn[10] == '\0');

    CHECK(t, restored.has_value());
    if (!restored) return;
    CHECK(t, *restored == original);
}

void testOverlongStringsAreTruncated(libster::test::TestContext& t) {
    const Borrower original{
        1,
        std::string(100, 'x'),
        std::string(100, 'y'),
        std::string(200, 'e'),
        std::string(200, 'a'),
        "2026-11-04",
        std::string(100, 'r')
    };

    const auto restored = Borrower::fromRecord(original.toRecord());

    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, restored->getLastName() == std::string(63, 'x'));
    CHECK(t, restored->getFirstName() == std::string(63, 'y'));
    CHECK(t, restored->getEmail() == std::string(127, 'e'));
    CHECK(t, restored->getAddress() == std::string(127, 'a'));
    CHECK(t, restored->getRecordedBy() == std::string(63, 'r'));
}

void testUnterminatedRecordIsReadFully(libster::test::TestContext& t) {
    BorrowerRecord record{};
    record.id = 1;
    record.status = BorrowerStatus::ACTIVE;
    for (char& c : record.lastName) c = 'A'; // kein '\0' in den 64 Bytes

    const auto restored = Borrower::fromRecord(record);

    CHECK(t, restored.has_value());
    if (!restored) return;
    CHECK(t, restored->getLastName() == std::string(64, 'A'));
    CHECK(t, restored->getFirstName().empty());
}

void testInvalidStatusIsRejected(libster::test::TestContext& t) {
    auto record = makeSampleBorrower().toRecord();
    record.status = static_cast<BorrowerStatus>(2);

    CHECK(t, !Borrower::fromRecord(record).has_value());
}

void testUnassignedIdPassesThrough(libster::test::TestContext& t) {
    auto record = makeSampleBorrower().toRecord();
    CHECK(t, record.id == 7);

    record.id = kUnassignedId;
    const auto restored = Borrower::fromRecord(record);

    CHECK(t, restored.has_value());
    if (!restored) return;
    CHECK(t, restored->getId() == kUnassignedId);
}

void testToRecordMapsAllFields(libster::test::TestContext& t) {
    const Borrower original = makeSampleBorrower();
    const auto record = original.toRecord();

    CHECK(t, record.id == original.getId());
    CHECK(t, record.status == original.getStatus());
    CHECK(t, libster::fixedToString(record.lastName) == original.getLastName());
    CHECK(t, libster::fixedToString(record.firstName) == original.getFirstName());
    CHECK(t, libster::fixedToString(record.email) == original.getEmail());
    CHECK(t, libster::fixedToString(record.address) == original.getAddress());
    CHECK(t, libster::fixedToString(record.recordedOn) == original.getRecordedOn());
    CHECK(t, libster::fixedToString(record.recordedBy) == original.getRecordedBy());
}

void testUpdateKeepsMetadata(libster::test::TestContext& t) {
    Borrower borrower = makeSampleBorrower();
    borrower.update("Neu", "Name", "neu@example.com", "Neue Strasse 2");

    CHECK(t, borrower.getLastName() == "Neu");
    CHECK(t, borrower.getFirstName() == "Name");
    CHECK(t, borrower.getEmail() == "neu@example.com");
    CHECK(t, borrower.getAddress() == "Neue Strasse 2");

    CHECK(t, borrower.getId() == 7);
    CHECK(t, borrower.getRecordedOn() == "2026-11-04");
    CHECK(t, borrower.getRecordedBy() == "Admin");
    CHECK(t, borrower.getStatus() == BorrowerStatus::ACTIVE);
}


} // namespace

void runBorrowerTests(libster::test::TestContext& t) {
    testActiveRoundtrip(t);
    testDeletedRoundtrip(t);
    testEmptyStrings(t);
    testMaxLengthStrings(t);
    testOverlongStringsAreTruncated(t);
    testUnterminatedRecordIsReadFully(t);
    testInvalidStatusIsRejected(t);
    testUnassignedIdPassesThrough(t);
    testToRecordMapsAllFields(t);
    testUpdateKeepsMetadata(t);
}
