#include <cstdint>
#include <string>
#include <type_traits>

#include "TestSupport.h"
#include "core/model/Book.h"
#include "core/model/CD.h"
#include "core/model/Medium.h"
#include "core/persistence/CStringUtil.h"

static_assert(std::is_abstract_v<libster::Medium>);
static_assert(!std::is_abstract_v<libster::Book>);
static_assert(!std::is_abstract_v<libster::CD>);
static_assert(std::has_virtual_destructor_v<libster::Medium>);

namespace {

using libster::Book;
using libster::BookRecord;
using libster::CD;
using libster::CDRecord;
using libster::Medium;
using libster::MediumType;
using libster::Status;
using libster::test::TestContext;

Book makeSampleBook(Status status = Status::AVAILABLE) {
    return Book{
        7,
        "ExampleBook",
        "ExampleBookPublisher",
        "Testing",
        "2005-12-24",
        "2026-11-06",
        "Shelf 24/5",
        1299,
        "Tester",
        "1234567890123",
        4,
        "ExampleAuthor;SecondAuthor",
        200,
        status
    };
}

CD makeSampleCD(Status status = Status::AVAILABLE) {
    return CD{
        9,
        "ExampleCD",
        "ExampleCDPublisher",
        "Hearing",
        "2012-10-24",
        "2026-11-06",
        "Shelf 12/14",
        2199,
        "Tester",
        "ExamplePerformer",
        490,
        status
    };
}

// Tests

void testConcreteTypes(TestContext& t) {
    const Book book = makeSampleBook();
    const CD cd = makeSampleCD();

    CHECK(t, book.getType() == MediumType::BOOK);
    CHECK(t, cd.getType() == MediumType::CD);

    const Medium& mediumBook = book;
    const Medium& mediumCD = cd;
    CHECK(t, mediumBook.getType() == MediumType::BOOK);
    CHECK(t, mediumCD.getType() == MediumType::CD);
}

void testBookRoundtrip(TestContext& t) {
    const Book original = makeSampleBook();
    const auto restored = Book::fromRecord(original.toRecord());

    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, *restored == original);
    CHECK(t, restored->getStatus() == Status::AVAILABLE);
    CHECK(t, restored->getAddedOn() == "2026-11-06");
    CHECK(t, restored->getRecordedBy() == "Tester");
}

void testCDRoundTrip(TestContext& t) {
    const CD original = makeSampleCD();
    const auto restored = CD::fromRecord(original.toRecord());

    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, *restored == original);
    CHECK(t, restored->getStatus() == Status::AVAILABLE);
    CHECK(t, restored->getAddedOn() == "2026-11-06");
    CHECK(t, restored->getRecordedBy() == "Tester");
}

void testDeletedStatusRoundtrip(TestContext& t) {
    const Book book = makeSampleBook(Status::DELETED);
    const auto restoredBook = Book::fromRecord(book.toRecord());
    CHECK(t, restoredBook.has_value());
    if (restoredBook) CHECK(t, restoredBook->getStatus() == Status::DELETED);

    const CD cd = makeSampleCD(Status::DELETED);
    const auto restoredCD = CD::fromRecord(cd.toRecord());
    CHECK(t, restoredCD.has_value());
    if (restoredCD) CHECK(t, restoredCD->getStatus() == Status::DELETED);
}

void testBookToRecordMapsAllFields(TestContext& t) {
    const Book original = makeSampleBook();
    const BookRecord record = original.toRecord();

    CHECK(t, record.id == original.getId());
    CHECK(t, libster::fixedToString(record.title) == original.getTitle());
    CHECK(t, libster::fixedToString(record.publisher) == original.getPublisher());
    CHECK(t, libster::fixedToString(record.genre) == original.getGenre());
    CHECK(t, libster::fixedToString(record.publishedOn) == original.getPublishedOn());
    CHECK(t, libster::fixedToString(record.addedOn) == original.getAddedOn());
    CHECK(t, libster::fixedToString(record.location) == original.getLocation());
    CHECK(t, record.price == original.getPrice());
    CHECK(t, libster::fixedToString(record.recordedBy) == original.getRecordedBy());
    CHECK(t, record.status == original.getStatus());
    CHECK(t, libster::fixedToString(record.isbn) == original.getIsbn());
    CHECK(t, record.edition == original.getEdition());
    CHECK(t, libster::fixedToString(record.authors) == original.getAuthors());
    CHECK(t, record.pageCount == original.getPageCount());
}

void testCDToRecordMapsAllFields(TestContext& t) {
    const CD original = makeSampleCD();
    const CDRecord record = original.toRecord();

    CHECK(t, record.id == original.getId());
    CHECK(t, libster::fixedToString(record.title) == original.getTitle());
    CHECK(t, libster::fixedToString(record.publisher) == original.getPublisher());
    CHECK(t, libster::fixedToString(record.genre) == original.getGenre());
    CHECK(t, libster::fixedToString(record.publishedOn) == original.getPublishedOn());
    CHECK(t, libster::fixedToString(record.addedOn) == original.getAddedOn());
    CHECK(t, libster::fixedToString(record.location) == original.getLocation());
    CHECK(t, record.price == original.getPrice());
    CHECK(t, libster::fixedToString(record.recordedBy) == original.getRecordedBy());
    CHECK(t, record.status == original.getStatus());
    CHECK(t, libster::fixedToString(record.performer) == original.getPerformer());
    CHECK(t, record.duration == original.getDuration());
}

void testBookEmptyStrings(TestContext& t) {
    const Book original{0, "", "", "", "", "", "", 0, "", "", 0, "", 0};
    const auto restored = Book::fromRecord(original.toRecord());

    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, *restored == original);
    CHECK(t, restored->getTitle().empty());
    CHECK(t, restored->getAuthors().empty());
    CHECK(t, restored->getIsbn().empty());
}

void testCDEmptyStrings(TestContext& t) {
    const CD original{0, "", "", "", "", "", "", 0, "", "", 0};
    const auto restored = CD::fromRecord(original.toRecord());

    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, *restored == original);
    CHECK(t, restored->getTitle().empty());
    CHECK(t, restored->getPerformer().empty());
}

void testBookOverlongStringsAreTruncated(TestContext& t) {
    const Book original{
        1,
        std::string(200, 'T'),
        std::string(100, 'P'),
        std::string(100, 'G'),
        "2026-11-06",
        "2026-11-06",
        std::string(100, 'L'),
        1234,
        std::string(100, 'R'),
        std::string(50, 'I'),
        1,
        std::string(300, 'A'),
        100
    };

    const BookRecord record = original.toRecord();
    CHECK(t, record.title[127] == '\0');
    CHECK(t, record.isbn[13] == '\0');
    CHECK(t, record.authors[255] == '\0');

    const auto restored = Book::fromRecord(record);
    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, restored->getTitle() == std::string(127, 'T'));
    CHECK(t, restored->getIsbn() == std::string(13, 'I'));
    CHECK(t, restored->getAuthors() == std::string(255, 'A'));
}

void testCDOverlongStringsAreTruncated(TestContext& t) {
    const CD original{
        1,
        std::string(200, 'T'),
        std::string(100, 'P'),
        std::string(100, 'G'),
        "2026-11-06",
        "2026-11-06",
        std::string(100, 'L'),
        1234,
        std::string(100, 'R'),
        std::string(100, 'X'),
        99
    };

    const CDRecord record = original.toRecord();
    CHECK(t, record.title[127] == '\0');
    CHECK(t, record.performer[63] == '\0');

    const auto restored = CD::fromRecord(record);
    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, restored->getTitle() == std::string(127, 'T'));
    CHECK(t, restored->getPerformer() == std::string(63, 'X'));
}

void testAuthorsKeepSemicolonSeparators(TestContext& t) {
    const Book original = makeSampleBook();
    CHECK(t, original.getAuthors() == "ExampleAuthor;SecondAuthor");

    const auto restored = Book::fromRecord(original.toRecord());
    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, restored->getAuthors() == original.getAuthors());
}

void testBookUpdateKeepsMetadata(TestContext& t) {
    Book book = makeSampleBook();
    book.update(
        "New Title",
        "New Publisher",
        "New Genre",
        "2020-01-01",
        "Shelf Z9",
        1234,
        "9780000000000",
        2,
        "A;B",
        99
    );

    CHECK(t, book.getTitle() == "New Title");
    CHECK(t, book.getPublisher() == "New Publisher");
    CHECK(t, book.getGenre() == "New Genre");
    CHECK(t, book.getPublishedOn() == "2020-01-01");
    CHECK(t, book.getLocation() == "Shelf Z9");
    CHECK(t, book.getPrice() == 1234);
    CHECK(t, book.getIsbn() == "9780000000000");
    CHECK(t, book.getEdition() == 2);
    CHECK(t, book.getAuthors() == "A;B");
    CHECK(t, book.getPageCount() == 99);

    CHECK(t, book.getId() == 7);
    CHECK(t, book.getAddedOn() == "2026-11-06");
    CHECK(t, book.getRecordedBy() == "Tester");
    CHECK(t, book.getStatus() == Status::AVAILABLE);
}

void testCDUpdateKeepsMetadata(TestContext& t) {
    CD cd = makeSampleCD();
    cd.update(
        "New Album",
        "New Label",
        "Rock",
        "2001-01-01",
        "Shelf C3",
        999,
        "New Performer",
        60
    );

    CHECK(t, cd.getTitle() == "New Album");
    CHECK(t, cd.getPublisher() == "New Label");
    CHECK(t, cd.getGenre() == "Rock");
    CHECK(t, cd.getPublishedOn() == "2001-01-01");
    CHECK(t, cd.getLocation() == "Shelf C3");
    CHECK(t, cd.getPrice() == 999);
    CHECK(t, cd.getPerformer() == "New Performer");
    CHECK(t, cd.getDuration() == 60);

    CHECK(t, cd.getId() == 9);
    CHECK(t, cd.getAddedOn() == "2026-11-06");
    CHECK(t, cd.getRecordedBy() == "Tester");
    CHECK(t, cd.getStatus() == Status::AVAILABLE);
}

void testInvalidStatusIsRejected(TestContext& t) {
    BookRecord bookRecord = makeSampleBook().toRecord();
    bookRecord.status = static_cast<Status>(3);
    CHECK(t, !Book::fromRecord(bookRecord).has_value());

    CDRecord cdRecord = makeSampleCD().toRecord();
    cdRecord.status = static_cast<Status>(3);
    CHECK(t, !CD::fromRecord(cdRecord).has_value());
}

} // namespace

void runModelTests(libster::test::TestContext& t) {
    testConcreteTypes(t);
    testBookRoundtrip(t);
    testCDRoundTrip(t);
    testDeletedStatusRoundtrip(t);
    testBookToRecordMapsAllFields(t);
    testCDToRecordMapsAllFields(t);
    testBookEmptyStrings(t);
    testCDEmptyStrings(t);
    testBookOverlongStringsAreTruncated(t);
    testCDOverlongStringsAreTruncated(t);
    testAuthorsKeepSemicolonSeparators(t);
    testBookUpdateKeepsMetadata(t);
    testCDUpdateKeepsMetadata(t);
    testInvalidStatusIsRejected(t);
}
