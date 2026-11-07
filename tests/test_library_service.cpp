#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "TestSupport.h"
#include "core/model/Borrower.h"
#include "core/persistence/BinaryFile.h"
#include "core/persistence/BorrowerRecord.h"
#include "core/persistence/CStringUtil.h"
#include "core/persistence/FileType.h"
#include "core/persistence/IoError.h"
#include "core/services/LibraryService.h"

namespace {

using libster::BinaryFile;
using libster::Borrower;
using libster::BorrowerRecord;
using libster::BorrowerStatus;
using libster::FileType;
using libster::IoError;
using libster::LibraryService;
using libster::ServiceError;
using libster::test::TempDir;
using libster::test::TestContext;

Borrower makeBorrower(
    const std::string& lastName,
    const std::string& firstName = "Anna",
    const std::string& email = "anna@example.org",
    const std::string& address = "Hauptweg"
) {
    return Borrower{
        libster::kUnassignedId,
        lastName,
        firstName,
        email,
        address,
        "2026-11-07",
        "Tester"
    };
}

std::string filePath(const std::string& directory, const char* fileName) {
    return (std::filesystem::path(directory) / fileName).string();
}

void testOpenCreatesFilesAndCloses(TestContext& t) {
    TempDir dir("libster_service_open");
    LibraryService service;

    CHECK(t, service.open(dir.path()) == IoError::None);
    CHECK(t, service.isOpen());
    CHECK(t, std::filesystem::exists(filePath(dir.path(), "borrowers.bin")));
    CHECK(t, std::filesystem::exists(filePath(dir.path(), "books.bin")));
    CHECK(t, std::filesystem::exists(filePath(dir.path(), "cds.bin")));
    CHECK(t, std::filesystem::exists(filePath(dir.path(), "loan_transactions.bin")));

    service.close();
    CHECK(t, !service.isOpen());
}

void testOpenRejectsWrongFileType(TestContext& t) {
    TempDir dir("libster_service_badtype");

    {
        BinaryFile wrong;
        CHECK(t, wrong.openOrCreate<BorrowerRecord>(
            filePath(dir.path(), "books.bin"), FileType::Borrower) == IoError::None);
    }

    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::InvalidType);
    CHECK(t, !service.isOpen());
}

void testOpenPreservesData(TestContext& t) {
    TempDir dir("libster_service_reopen");

    {
        LibraryService service;
        CHECK(t, service.open(dir.path()) == IoError::None);

        Borrower borrower = makeBorrower("Mustermann");
        CHECK(t, service.addBorrower(borrower) == ServiceError::None);
        CHECK(t, borrower.getId() == 1u);
    }

    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    std::optional<Borrower> restored;
    CHECK(t, service.getBorrower(1, restored) == ServiceError::None);
    CHECK(t, restored.has_value());
    if (restored) {
        CHECK(t, restored->getLastName() == "Mustermann");
    }
}

void testAddBorrowerAssignsIds(TestContext& t) {
    TempDir dir("libster_service_add");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Borrower first = makeBorrower("Alpha", "Anna");
    Borrower second = makeBorrower("Beta", "Bert");

    CHECK(t, service.addBorrower(first) == ServiceError::None);
    CHECK(t, service.addBorrower(second) == ServiceError::None);

    CHECK(t, first.getId() == 1u);
    CHECK(t, second.getId() == 2u);

    std::optional<Borrower> restored;
    CHECK(t, service.getBorrower(2, restored) == ServiceError::None);
    CHECK(t, restored.has_value());
    if (restored) {
        CHECK(t, restored->getLastName() == "Beta");
        CHECK(t, restored->getFirstName() == "Bert");
    }
}

void testAddBorrowerRejectsAssignedId(TestContext& t) {
    TempDir dir("libster_service_add_assigned");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Borrower borrower = makeBorrower("Alpha");
    borrower.setId(7);

    CHECK(t, service.addBorrower(borrower) == ServiceError::InvalidData);
    CHECK(t, borrower.getId() == 7u);
}

void testClosedServiceRejectsOperations(TestContext& t) {
    LibraryService service;

    Borrower borrower = makeBorrower("Alpha");
    std::optional<Borrower> out;
    std::vector<Borrower> hits;

    CHECK(t, service.addBorrower(borrower) == ServiceError::NotOpen);
    CHECK(t, service.getBorrower(1, out) == ServiceError::NotOpen);
    CHECK(t, service.updateBorrower(borrower) == ServiceError::NotOpen);
    CHECK(t, service.deleteBorrower(1) == ServiceError::NotOpen);
    CHECK(t, service.findBorrowers("", hits) == ServiceError::NotOpen);
}

void testGetBorrowerNotFound(TestContext& t) {
    TempDir dir("libster_service_get_missing");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    std::optional<Borrower> out;
    CHECK(t, service.getBorrower(1, out) == ServiceError::NotFound);
    CHECK(t, service.getBorrower(99, out) == ServiceError::NotFound);
}

void testUpdateBorrowerKeepsMetadata(TestContext& t) {
    TempDir dir("libster_service_update");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Borrower borrower = makeBorrower("Mustermann", "Max", "max@example.org", "Musterweg");
    CHECK(t, service.addBorrower(borrower) == ServiceError::None);

    Borrower changed{
        borrower.getId(),
        "Neumann",
        "Nina",
        "nina@example.org",
        "Neue Strasse",
        "1999-01-01",
        "Hacker"
    };

    CHECK(t, service.updateBorrower(changed) == ServiceError::None);

    std::optional<Borrower> restored;
    CHECK(t, service.getBorrower(borrower.getId(), restored) == ServiceError::None);
    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, restored->getLastName() == "Neumann");
    CHECK(t, restored->getFirstName() == "Nina");
    CHECK(t, restored->getEmail() == "nina@example.org");
    CHECK(t, restored->getAddress() == "Neue Strasse");

    CHECK(t, restored->getId() == borrower.getId());
    CHECK(t, restored->getRecordedOn() == "2026-11-07");
    CHECK(t, restored->getRecordedBy() == "Tester");
    CHECK(t, restored->getStatus() == BorrowerStatus::ACTIVE);
}

void testUpdateBorrowerNotFound(TestContext& t) {
    TempDir dir("libster_service_update_missing");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Borrower missing = makeBorrower("Missing");
    missing.setId(42);

    CHECK(t, service.updateBorrower(missing) == ServiceError::NotFound);
}

void testDeleteBorrowerAnonymizesRecord(TestContext& t) {
    TempDir dir("libster_service_delete");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Borrower borrower = makeBorrower("Mustermann", "Max", "max@example.org", "Musterweg");
    CHECK(t, service.addBorrower(borrower) == ServiceError::None);

    const std::uint32_t id = borrower.getId();

    CHECK(t, service.deleteBorrower(id) == ServiceError::None);

    std::optional<Borrower> out;
    CHECK(t, service.getBorrower(id, out) == ServiceError::NotFound);

    service.close();

    BinaryFile raw;
    CHECK(t, raw.openOrCreate<BorrowerRecord>(
        filePath(dir.path(), "borrowers.bin"), FileType::Borrower) == IoError::None);

    BorrowerRecord record{};
    CHECK(t, raw.read(id, record) == IoError::None);

    CHECK(t, record.id == id);
    CHECK(t, record.status == BorrowerStatus::DELETED);
    CHECK(t, libster::fixedToString(record.lastName).empty());
    CHECK(t, libster::fixedToString(record.firstName).empty());
    CHECK(t, libster::fixedToString(record.email).empty());
    CHECK(t, libster::fixedToString(record.address).empty());
    CHECK(t, libster::fixedToString(record.recordedOn) == "2026-11-07");
    CHECK(t, libster::fixedToString(record.recordedBy) == "Tester");
}

void testDeleteBorrowerTwiceFails(TestContext& t) {
    TempDir dir("libster_service_delete_twice");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Borrower borrower = makeBorrower("Mustermann");
    CHECK(t, service.addBorrower(borrower) == ServiceError::None);

    CHECK(t, service.deleteBorrower(borrower.getId()) == ServiceError::None);
    CHECK(t, service.deleteBorrower(borrower.getId()) == ServiceError::NotFound);
}

void testDeleteBorrowerNotFound(TestContext& t) {
    TempDir dir("libster_service_delete_missing");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    CHECK(t, service.deleteBorrower(1) == ServiceError::NotFound);
}

void testUpdateDeletedBorrowerFails(TestContext& t) {
    TempDir dir("libster_service_update_deleted");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Borrower borrower = makeBorrower("Mustermann");
    CHECK(t, service.addBorrower(borrower) == ServiceError::None);
    CHECK(t, service.deleteBorrower(borrower.getId()) == ServiceError::None);

    CHECK(t, service.updateBorrower(borrower) == ServiceError::NotFound);
}

void testIdsAreNotReusedAfterDelete(TestContext& t) {
    TempDir dir("libster_service_ids");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Borrower first = makeBorrower("Alpha");
    CHECK(t, service.addBorrower(first) == ServiceError::None);
    CHECK(t, first.getId() == 1u);

    CHECK(t, service.deleteBorrower(first.getId()) == ServiceError::None);

    Borrower second = makeBorrower("Beta");
    CHECK(t, service.addBorrower(second) == ServiceError::None);
    CHECK(t, second.getId() == 2u);
}

void testFindBorrowersEmptyQueryReturnsActive(TestContext& t) {
    TempDir dir("libster_service_find_empty");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Borrower alpha = makeBorrower("Alpha");
    Borrower beta = makeBorrower("Beta");
    CHECK(t, service.addBorrower(alpha) == ServiceError::None);
    CHECK(t, service.addBorrower(beta) == ServiceError::None);
    CHECK(t, service.deleteBorrower(beta.getId()) == ServiceError::None);

    std::vector<Borrower> hits(3, makeBorrower("Old"));
    CHECK(t, service.findBorrowers("", hits) == ServiceError::None);

    CHECK(t, hits.size() == 1u);
    if (hits.size() == 1u) {
        CHECK(t, hits[0].getLastName() == "Alpha");
    }
}

void testFindBorrowersSubstringCaseInsensitive(TestContext& t) {
    TempDir dir("libster_service_find_text");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Borrower first = makeBorrower("Mustermann");
    Borrower second = makeBorrower("Musterfrau", "Mia");
    Borrower third = makeBorrower("Anders", "Bert");

    CHECK(t, service.addBorrower(first) == ServiceError::None);
    CHECK(t, service.addBorrower(second) == ServiceError::None);
    CHECK(t, service.addBorrower(third) == ServiceError::None);

    std::vector<Borrower> hits;
    CHECK(t, service.findBorrowers("muster", hits) == ServiceError::None);
    CHECK(t, hits.size() == 2u);

    CHECK(t, service.findBorrowers("MUSTER", hits) == ServiceError::None);
    CHECK(t, hits.size() == 2u);

    CHECK(t, service.findBorrowers("frau", hits) == ServiceError::None);
    CHECK(t, hits.size() == 1u);
    if (hits.size() == 1u) {
        CHECK(t, hits[0].getLastName() == "Musterfrau");
    }
}

void testFindBorrowersSearchesEmail(TestContext& t) {
    TempDir dir("libster_service_find_email");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Borrower first = makeBorrower("Mustermann", "Max", "max@example.org");
    Borrower second = makeBorrower("Anders", "Anna", "anna@other.test");

    CHECK(t, service.addBorrower(first) == ServiceError::None);
    CHECK(t, service.addBorrower(second) == ServiceError::None);

    std::vector<Borrower> hits;
    CHECK(t, service.findBorrowers("EXAMPLE.ORG", hits) == ServiceError::None);

    CHECK(t, hits.size() == 1u);
    if (hits.size() == 1u) {
        CHECK(t, hits[0].getLastName() == "Mustermann");
    }
}

void testFindBorrowersNumericQueryMatchesId(TestContext& t) {
    TempDir dir("libster_service_find_id");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Borrower first = makeBorrower("Alpha");
    Borrower second = makeBorrower("Beta");

    CHECK(t, service.addBorrower(first) == ServiceError::None);
    CHECK(t, service.addBorrower(second) == ServiceError::None);

    std::vector<Borrower> hits;
    CHECK(t, service.findBorrowers("2", hits) == ServiceError::None);

    CHECK(t, hits.size() == 1u);
    if (hits.size() == 1u) {
        CHECK(t, hits[0].getId() == 2u);
        CHECK(t, hits[0].getLastName() == "Beta");
    }

    CHECK(t, service.findBorrowers("99", hits) == ServiceError::None);
    CHECK(t, hits.empty());
}

void testFindBorrowersNoMatch(TestContext& t) {
    TempDir dir("libster_service_find_none");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Borrower borrower = makeBorrower("Alpha");
    CHECK(t, service.addBorrower(borrower) == ServiceError::None);

    std::vector<Borrower> hits(2, makeBorrower("Old"));
    CHECK(t, service.findBorrowers("zzz", hits) == ServiceError::None);
    CHECK(t, hits.empty());
}

} // namespace

void runLibraryServiceTests(libster::test::TestContext& t) {
    testOpenCreatesFilesAndCloses(t);
    testOpenRejectsWrongFileType(t);
    testOpenPreservesData(t);
    testAddBorrowerAssignsIds(t);
    testAddBorrowerRejectsAssignedId(t);
    testClosedServiceRejectsOperations(t);
    testGetBorrowerNotFound(t);
    testUpdateBorrowerKeepsMetadata(t);
    testUpdateBorrowerNotFound(t);
    testDeleteBorrowerAnonymizesRecord(t);
    testDeleteBorrowerTwiceFails(t);
    testDeleteBorrowerNotFound(t);
    testUpdateDeletedBorrowerFails(t);
    testIdsAreNotReusedAfterDelete(t);
    testFindBorrowersEmptyQueryReturnsActive(t);
    testFindBorrowersSubstringCaseInsensitive(t);
    testFindBorrowersSearchesEmail(t);
    testFindBorrowersNumericQueryMatchesId(t);
    testFindBorrowersNoMatch(t);
}
