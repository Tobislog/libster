#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "TestSupport.h"
#include "core/model/Borrower.h"
#include "core/model/Book.h"
#include "core/model/CD.h"
#include "core/model/LoanTransaction.h"
#include "core/model/MediumType.h"
#include "core/model/Status.h"
#include "core/persistence/BinaryFile.h"
#include "core/persistence/BorrowerRecord.h"
#include "core/persistence/BookRecord.h"
#include "core/persistence/CDRecord.h"
#include "core/persistence/CStringUtil.h"
#include "core/persistence/FileType.h"
#include "core/persistence/IoError.h"
#include "core/persistence/LoanTransactionRecord.h"
#include "core/services/LibraryService.h"

namespace {

using libster::BinaryFile;
using libster::Book;
using libster::BookRecord;
using libster::Borrower;
using libster::BorrowerRecord;
using libster::BorrowerStatus;
using libster::CD;
using libster::CDRecord;
using libster::FileType;
using libster::IoError;
using libster::LibraryService;
using libster::LoanTransaction;
using libster::LoanTransactionRecord;
using libster::MediumType;
using libster::ServiceError;
using libster::Status;
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

Book makeBook(
    const std::string& title = "Example Book",
    const std::string& authors = "Author One;Author Two",
    const std::string& isbn = "ISBN-A",
    const std::string& publisher = "Example Publisher"
) {
    return Book{
        libster::kUnassignedId,
        title,
        publisher,
        "Testing",
        "2020-01-01",
        "2026-11-07",
        "Shelf A1",
        1299,
        "Tester",
        isbn,
        1,
        authors,
        320
    };
}

CD makeCD(
    const std::string& title = "Example CD",
    const std::string& performer = "Example Performer",
    const std::string& publisher = "Example Label",
    const std::string& location = "Shelf B1"
) {
    return CD{
        libster::kUnassignedId,
        title,
        publisher,
        "Jazz",
        "2001-01-01",
        "2026-11-07",
        location,
        999,
        "Tester",
        performer,
        45
    };
}

std::uint32_t appendRawLoan(
    TestContext& t,
    const std::string& directory,
    std::uint32_t borrowerId,
    std::uint32_t mediumId,
    MediumType type
) {
    BinaryFile loans;
    CHECK(t, loans.openOrCreate<LoanTransactionRecord>(
        filePath(directory, "loan_transactions.bin"),
        FileType::LoanTransaction) == IoError::None);

    LoanTransaction loan{
        libster::kUnassignedId,
        borrowerId,
        mediumId,
        type,
        1760000000LL,
        0,
        "Tester"
    };

    std::uint32_t loanId = 0;
    CHECK(t, loans.append(loan.toRecord(), loanId) == IoError::None);
    return loanId;
}

void returnRawLoan(
    TestContext& t,
    const std::string& directory,
    std::uint32_t loanId,
    std::int64_t returnDate
) {
    BinaryFile loans;
    CHECK(t, loans.openOrCreate<LoanTransactionRecord>(
        filePath(directory, "loan_transactions.bin"),
        FileType::LoanTransaction) == IoError::None);

    LoanTransactionRecord record{};
    CHECK(t, loans.read(loanId, record) == IoError::None);

    record.returnDate = returnDate;
    CHECK(t, loans.write(loanId, record) == IoError::None);
}

// General

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

// Borrower
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

// Books
void testAddBookAssignsIdAndRoundtrip(TestContext& t) {
    TempDir dir("libster_service_book_add");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Book book = makeBook("Effective C++");
    CHECK(t, service.addBook(book) == ServiceError::None);
    CHECK(t, book.getId() == 1u);

    std::optional<Book> restored;
    CHECK(t, service.getBook(book.getId(), restored) == ServiceError::None);
    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, *restored == book);
    CHECK(t, restored->getStatus() == Status::AVAILABLE);
    CHECK(t, restored->getAddedOn() == "2026-11-07");
    CHECK(t, restored->getRecordedBy() == "Tester");
}

void testAddBookRejectsAssignedId(TestContext& t) {
    TempDir dir("libster_service_book_add_assigned");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Book book = makeBook();
    book.setId(7);

    CHECK(t, service.addBook(book) == ServiceError::InvalidData);
    CHECK(t, book.getId() == 7u);
}

void testGetBookNotFound(TestContext& t) {
    TempDir dir("libster_service_book_get_missing");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    std::optional<Book> out;
    CHECK(t, service.getBook(1, out) == ServiceError::NotFound);
    CHECK(t, service.getBook(99, out) == ServiceError::NotFound);
}

void testUpdateBookKeepsMetadata(TestContext& t) {
    TempDir dir("libster_service_book_update");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Book original = makeBook("Old Title");
    CHECK(t, service.addBook(original) == ServiceError::None);

    Book changed{
        original.getId(),
        "New Title",
        "New Publisher",
        "New Genre",
        "2020-01-01",
        "1999-01-01",   // addedOn darf nicht uebernommen werden
        "Shelf Z9",
        1234,
        "Hacker",       // recordedBy darf nicht uebernommen werden
        "ISBN-NEW",
        2,
        "New Author;Second Author",
        99
    };

    CHECK(t, service.updateBook(changed) == ServiceError::None);

    std::optional<Book> restored;
    CHECK(t, service.getBook(original.getId(), restored) == ServiceError::None);
    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, restored->getTitle() == "New Title");
    CHECK(t, restored->getPublisher() == "New Publisher");
    CHECK(t, restored->getGenre() == "New Genre");
    CHECK(t, restored->getPublishedOn() == "2020-01-01");
    CHECK(t, restored->getLocation() == "Shelf Z9");
    CHECK(t, restored->getPrice() == 1234u);
    CHECK(t, restored->getIsbn() == "ISBN-NEW");
    CHECK(t, restored->getEdition() == 2u);
    CHECK(t, restored->getAuthors() == "New Author;Second Author");
    CHECK(t, restored->getPageCount() == 99u);

    CHECK(t, restored->getId() == original.getId());
    CHECK(t, restored->getAddedOn() == "2026-11-07");
    CHECK(t, restored->getRecordedBy() == "Tester");
    CHECK(t, restored->getStatus() == Status::AVAILABLE);
}

void testUpdateBookNotFound(TestContext& t) {
    TempDir dir("libster_service_book_update_missing");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Book missing = makeBook();
    missing.setId(42);

    CHECK(t, service.updateBook(missing) == ServiceError::NotFound);
}

void testUpdateDeletedBookFails(TestContext& t) {
    TempDir dir("libster_service_book_update_deleted");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Book book = makeBook();
    CHECK(t, service.addBook(book) == ServiceError::None);
    CHECK(t, service.deleteBook(book.getId()) == ServiceError::None);

    CHECK(t, service.updateBook(book) == ServiceError::NotFound);
}

void testDeleteBookMarksDeleted(TestContext& t) {
    TempDir dir("libster_service_book_delete");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Book book = makeBook();
    CHECK(t, service.addBook(book) == ServiceError::None);

    CHECK(t, service.deleteBook(book.getId()) == ServiceError::None);

    std::optional<Book> out;
    CHECK(t, service.getBook(book.getId(), out) == ServiceError::NotFound);

    service.close();

    BinaryFile raw;
    CHECK(t, raw.openOrCreate<BookRecord>(
        filePath(dir.path(), "books.bin"), FileType::Book) == IoError::None);

    BookRecord record{};
    CHECK(t, raw.read(book.getId(), record) == IoError::None);
    CHECK(t, record.id == book.getId());
    CHECK(t, record.status == Status::DELETED);
    CHECK(t, libster::fixedToString(record.title) == "Example Book");
}

void testDeleteBookTwiceFails(TestContext& t) {
    TempDir dir("libster_service_book_delete_twice");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Book book = makeBook();
    CHECK(t, service.addBook(book) == ServiceError::None);

    CHECK(t, service.deleteBook(book.getId()) == ServiceError::None);
    CHECK(t, service.deleteBook(book.getId()) == ServiceError::NotFound);
}

void testDeleteBookWithOpenLoanRefused(TestContext& t) {
    TempDir dir("libster_service_book_loan");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Book book = makeBook();
    CHECK(t, service.addBook(book) == ServiceError::None);
    service.close();

    const std::uint32_t loanId = appendRawLoan(
        t, dir.path(), 1, book.getId(), MediumType::BOOK);
    CHECK(t, loanId != 0u);

    CHECK(t, service.open(dir.path()) == IoError::None);
    CHECK(t, service.deleteBook(book.getId()) == ServiceError::HasOpenLoans);

    std::optional<Book> restored;
    CHECK(t, service.getBook(book.getId(), restored) == ServiceError::None);
    CHECK(t, restored.has_value());
    if (restored) {
        CHECK(t, restored->getStatus() == Status::AVAILABLE);
    }
}

void testDeleteBookAfterReturnSucceeds(TestContext& t) {
    TempDir dir("libster_service_book_returned");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Book book = makeBook();
    CHECK(t, service.addBook(book) == ServiceError::None);
    service.close();

    const std::uint32_t loanId = appendRawLoan(
        t, dir.path(), 1, book.getId(), MediumType::BOOK);
    CHECK(t, loanId != 0u);

    returnRawLoan(t, dir.path(), loanId, 1760003600LL);

    CHECK(t, service.open(dir.path()) == IoError::None);
    CHECK(t, service.deleteBook(book.getId()) == ServiceError::None);
}

void testFindBooksEmptyQueryReturnsActive(TestContext& t) {
    TempDir dir("libster_service_book_find_empty");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Book alpha = makeBook("Alpha");
    Book beta = makeBook("Beta");
    CHECK(t, service.addBook(alpha) == ServiceError::None);
    CHECK(t, service.addBook(beta) == ServiceError::None);
    CHECK(t, service.deleteBook(beta.getId()) == ServiceError::None);

    std::vector<Book> hits(3, makeBook("Old"));
    CHECK(t, service.findBooks("", hits) == ServiceError::None);

    CHECK(t, hits.size() == 1u);
    if (hits.size() == 1u) {
        CHECK(t, hits[0].getTitle() == "Alpha");
    }
}

void testFindBooksSubstringCaseInsensitive(TestContext& t) {
    TempDir dir("libster_service_book_find_text");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Book first = makeBook(
        "Effective C++",
        "Scott Meyers;Andrei Alexandrescu",
        "ISBN-A",
        "Addison-Wesley");
    Book second = makeBook(
        "More Effective C++",
        "Scott Meyers",
        "ISBN-B",
        "Addison-Wesley");
    Book third = makeBook(
        "Clean Code",
        "Robert Martin",
        "ISBN-C",
        "Prentice Hall");

    CHECK(t, service.addBook(first) == ServiceError::None);
    CHECK(t, service.addBook(second) == ServiceError::None);
    CHECK(t, service.addBook(third) == ServiceError::None);

    std::vector<Book> hits;

    CHECK(t, service.findBooks("effective", hits) == ServiceError::None);
    CHECK(t, hits.size() == 2u);

    CHECK(t, service.findBooks("EFFECTIVE", hits) == ServiceError::None);
    CHECK(t, hits.size() == 2u);

    CHECK(t, service.findBooks("alexandrescu", hits) == ServiceError::None);
    CHECK(t, hits.size() == 1u);

    CHECK(t, service.findBooks("addison", hits) == ServiceError::None);
    CHECK(t, hits.size() == 2u);

    CHECK(t, service.findBooks("ISBN-B", hits) == ServiceError::None);
    CHECK(t, hits.size() == 1u);
    if (hits.size() == 1u) {
        CHECK(t, hits[0].getTitle() == "More Effective C++");
    }
}

void testFindBooksNumericQueryMatchesId(TestContext& t) {
    TempDir dir("libster_service_book_find_id");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Book first = makeBook("Alpha", "Author A", "ISBN-A", "Publisher A");
    Book second = makeBook("Beta", "Author B", "ISBN-B", "Publisher B");

    CHECK(t, service.addBook(first) == ServiceError::None);
    CHECK(t, service.addBook(second) == ServiceError::None);

    std::vector<Book> hits;
    CHECK(t, service.findBooks("2", hits) == ServiceError::None);

    CHECK(t, hits.size() == 1u);
    if (hits.size() == 1u) {
        CHECK(t, hits[0].getId() == 2u);
        CHECK(t, hits[0].getTitle() == "Beta");
    }

    CHECK(t, service.findBooks("99", hits) == ServiceError::None);
    CHECK(t, hits.empty());
}

void testFindBooksNoMatch(TestContext& t) {
    TempDir dir("libster_service_book_find_none");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Book book = makeBook("Alpha");
    CHECK(t, service.addBook(book) == ServiceError::None);

    std::vector<Book> hits(2, makeBook("Old"));
    CHECK(t, service.findBooks("zzz", hits) == ServiceError::None);
    CHECK(t, hits.empty());
}

// CDs
void testAddCDAssignsIdAndRoundtrip(TestContext& t) {
    TempDir dir("libster_service_cd_add");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    CD cd = makeCD("Kind of Blue");
    CHECK(t, service.addCD(cd) == ServiceError::None);
    CHECK(t, cd.getId() == 1u);

    std::optional<CD> restored;
    CHECK(t, service.getCD(cd.getId(), restored) == ServiceError::None);
    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, *restored == cd);
    CHECK(t, restored->getStatus() == Status::AVAILABLE);
    CHECK(t, restored->getAddedOn() == "2026-11-07");
    CHECK(t, restored->getRecordedBy() == "Tester");
}

void testAddCDRejectsAssignedId(TestContext& t) {
    TempDir dir("libster_service_cd_add_assigned");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    CD cd = makeCD();
    cd.setId(7);

    CHECK(t, service.addCD(cd) == ServiceError::InvalidData);
    CHECK(t, cd.getId() == 7u);
}

void testGetCDNotFound(TestContext& t) {
    TempDir dir("libster_service_cd_get_missing");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    std::optional<CD> out;
    CHECK(t, service.getCD(1, out) == ServiceError::NotFound);
    CHECK(t, service.getCD(99, out) == ServiceError::NotFound);
}

void testUpdateCDKeepsMetadata(TestContext& t) {
    TempDir dir("libster_service_cd_update");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    CD original = makeCD("Old Album");
    CHECK(t, service.addCD(original) == ServiceError::None);

    CD changed{
        original.getId(),
        "New Album",
        "New Label",
        "Rock",
        "2005-05-05",
        "1999-01-01",   // addedOn darf nicht uebernommen werden
        "Shelf Z9",
        1234,
        "Hacker",       // recordedBy darf nicht uebernommen werden
        "New Performer",
        60
    };

    CHECK(t, service.updateCD(changed) == ServiceError::None);

    std::optional<CD> restored;
    CHECK(t, service.getCD(original.getId(), restored) == ServiceError::None);
    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, restored->getTitle() == "New Album");
    CHECK(t, restored->getPublisher() == "New Label");
    CHECK(t, restored->getGenre() == "Rock");
    CHECK(t, restored->getPublishedOn() == "2005-05-05");
    CHECK(t, restored->getLocation() == "Shelf Z9");
    CHECK(t, restored->getPrice() == 1234u);
    CHECK(t, restored->getPerformer() == "New Performer");
    CHECK(t, restored->getDuration() == 60u);

    CHECK(t, restored->getId() == original.getId());
    CHECK(t, restored->getAddedOn() == "2026-11-07");
    CHECK(t, restored->getRecordedBy() == "Tester");
    CHECK(t, restored->getStatus() == Status::AVAILABLE);
}

void testUpdateDeletedCDFails(TestContext& t) {
    TempDir dir("libster_service_cd_update_deleted");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    CD cd = makeCD();
    CHECK(t, service.addCD(cd) == ServiceError::None);
    CHECK(t, service.deleteCD(cd.getId()) == ServiceError::None);

    CHECK(t, service.updateCD(cd) == ServiceError::NotFound);
}

void testDeleteCDMarksDeleted(TestContext& t) {
    TempDir dir("libster_service_cd_delete");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    CD cd = makeCD();
    CHECK(t, service.addCD(cd) == ServiceError::None);

    CHECK(t, service.deleteCD(cd.getId()) == ServiceError::None);

    std::optional<CD> out;
    CHECK(t, service.getCD(cd.getId(), out) == ServiceError::NotFound);

    service.close();

    BinaryFile raw;
    CHECK(t, raw.openOrCreate<CDRecord>(
        filePath(dir.path(), "cds.bin"), FileType::CD) == IoError::None);

    CDRecord record{};
    CHECK(t, raw.read(cd.getId(), record) == IoError::None);
    CHECK(t, record.id == cd.getId());
    CHECK(t, record.status == Status::DELETED);
    CHECK(t, libster::fixedToString(record.title) == "Example CD");
}

void testDeleteCDWithOpenLoanRefused(TestContext& t) {
    TempDir dir("libster_service_cd_loan");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    CD cd = makeCD();
    CHECK(t, service.addCD(cd) == ServiceError::None);
    service.close();

    const std::uint32_t loanId = appendRawLoan(
        t, dir.path(), 1, cd.getId(), MediumType::CD);
    CHECK(t, loanId != 0u);

    CHECK(t, service.open(dir.path()) == IoError::None);
    CHECK(t, service.deleteCD(cd.getId()) == ServiceError::HasOpenLoans);
}

void testFindCDsSubstringAndFields(TestContext& t) {
    TempDir dir("libster_service_cd_find_text");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    CD first = makeCD("Kind of Blue", "Miles Davis", "Columbia", "Shelf B2");
    CD second = makeCD("Bitches Brew", "Miles Davis", "Columbia", "Shelf B3");
    CD third = makeCD("Abbey Road", "The Beatles", "Apple", "Shelf C1");

    CHECK(t, service.addCD(first) == ServiceError::None);
    CHECK(t, service.addCD(second) == ServiceError::None);
    CHECK(t, service.addCD(third) == ServiceError::None);

    std::vector<CD> hits;

    CHECK(t, service.findCDs("miles", hits) == ServiceError::None);
    CHECK(t, hits.size() == 2u);

    CHECK(t, service.findCDs("abbey", hits) == ServiceError::None);
    CHECK(t, hits.size() == 1u);

    CHECK(t, service.findCDs("columbia", hits) == ServiceError::None);
    CHECK(t, hits.size() == 2u);

    CHECK(t, service.findCDs("shelf c1", hits) == ServiceError::None);
    CHECK(t, hits.size() == 1u);
    if (hits.size() == 1u) {
        CHECK(t, hits[0].getTitle() == "Abbey Road");
    }
}

void testFindCDsNumericQueryMatchesId(TestContext& t) {
    TempDir dir("libster_service_cd_find_id");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    CD first = makeCD("Alpha", "Performer A", "Label A", "Shelf A");
    CD second = makeCD("Beta", "Performer B", "Label B", "Shelf B");

    CHECK(t, service.addCD(first) == ServiceError::None);
    CHECK(t, service.addCD(second) == ServiceError::None);

    std::vector<CD> hits;
    CHECK(t, service.findCDs("2", hits) == ServiceError::None);

    CHECK(t, hits.size() == 1u);
    if (hits.size() == 1u) {
        CHECK(t, hits[0].getId() == 2u);
        CHECK(t, hits[0].getTitle() == "Beta");
    }

    CHECK(t, service.findCDs("99", hits) == ServiceError::None);
    CHECK(t, hits.empty());
}

// Book & CD

void testBookAndCDIdsAreIndependent(TestContext& t) {
    TempDir dir("libster_service_medium_ids");
    LibraryService service;
    CHECK(t, service.open(dir.path()) == IoError::None);

    Book book = makeBook();
    CD cd = makeCD();

    CHECK(t, service.addBook(book) == ServiceError::None);
    CHECK(t, service.addCD(cd) == ServiceError::None);

    CHECK(t, book.getId() == 1u);
    CHECK(t, cd.getId() == 1u);

    CHECK(t, service.deleteBook(book.getId()) == ServiceError::None);

    std::optional<CD> cdOut;
    CHECK(t, service.getCD(cd.getId(), cdOut) == ServiceError::None);
    CHECK(t, cdOut.has_value());
    if (cdOut) {
        CHECK(t, cdOut->getTitle() == cd.getTitle());
    }
}

void testClosedServiceRejectsBookAndCDOperations(TestContext& t) {
    LibraryService service;

    Book book = makeBook();
    CD cd = makeCD();
    std::optional<Book> bookOut;
    std::optional<CD> cdOut;
    std::vector<Book> books;
    std::vector<CD> cds;

    CHECK(t, service.addBook(book) == ServiceError::NotOpen);
    CHECK(t, service.getBook(1, bookOut) == ServiceError::NotOpen);
    CHECK(t, service.updateBook(book) == ServiceError::NotOpen);
    CHECK(t, service.deleteBook(1) == ServiceError::NotOpen);
    CHECK(t, service.findBooks("", books) == ServiceError::NotOpen);

    CHECK(t, service.addCD(cd) == ServiceError::NotOpen);
    CHECK(t, service.getCD(1, cdOut) == ServiceError::NotOpen);
    CHECK(t, service.updateCD(cd) == ServiceError::NotOpen);
    CHECK(t, service.deleteCD(1) == ServiceError::NotOpen);
    CHECK(t, service.findCDs("", cds) == ServiceError::NotOpen);
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

    testAddBookAssignsIdAndRoundtrip(t);
    testAddBookRejectsAssignedId(t);
    testGetBookNotFound(t);
    testUpdateBookKeepsMetadata(t);
    testUpdateBookNotFound(t);
    testUpdateDeletedBookFails(t);
    testDeleteBookMarksDeleted(t);
    testDeleteBookTwiceFails(t);
    testDeleteBookWithOpenLoanRefused(t);
    testDeleteBookAfterReturnSucceeds(t);
    testFindBooksEmptyQueryReturnsActive(t);
    testFindBooksSubstringCaseInsensitive(t);
    testFindBooksNumericQueryMatchesId(t);
    testFindBooksNoMatch(t);

    testAddCDAssignsIdAndRoundtrip(t);
    testAddCDRejectsAssignedId(t);
    testGetCDNotFound(t);
    testUpdateCDKeepsMetadata(t);
    testUpdateDeletedCDFails(t);
    testDeleteCDMarksDeleted(t);
    testDeleteCDWithOpenLoanRefused(t);
    testFindCDsSubstringAndFields(t);
    testFindCDsNumericQueryMatchesId(t);

    testBookAndCDIdsAreIndependent(t);
    testClosedServiceRejectsBookAndCDOperations(t);
}
