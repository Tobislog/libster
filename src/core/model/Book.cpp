#include "Book.h"

#include <utility>

#include "core/persistence/CStringUtil.h"

namespace libster {
    
Book::Book(
    std::uint32_t id,
        std::string title,
        std::string publisher,
        std::string genre,
        std::string publishedOn,
        std::string addedOn,
        std::string location,
        std::uint32_t price,
        std::string recordedBy,
        std::string isbn,
        std::uint16_t edition,
        std::string authors,
        std::uint16_t pageCount,
        Status status
) : Medium(
    id, 
    std::move(title),
    std::move(publisher),
    std::move(genre),
    std::move(publishedOn),
    std::move(addedOn),
    std::move(location),
    price,
    std::move(recordedBy),
    status
),  isbn_(std::move(isbn)),
    edition_(edition),
    authors_(std::move(authors)),
    pageCount_(pageCount)
{}

MediumType Book::getType() const noexcept {
    return MediumType::BOOK;
}

const std::string& Book::getIsbn() const noexcept {
    return isbn_;
}

std::uint16_t Book::getEdition() const noexcept {
    return edition_;
}

const std::string& Book::getAuthors() const noexcept {
    return authors_;
}

std::uint16_t Book::getPageCount() const noexcept {
    return pageCount_;
}

void Book::update(
    std::string title,
    std::string publisher,
    std::string genre,
    std::string publishedOn,
    std::string location,
    std::uint32_t price,
    std::string isbn,
    std::uint16_t edition,
    std::string authors,
    std::uint16_t pageCount
) {
    updateCommon(
        std::move(title),
        std::move(publisher),
        std::move(genre),
        std::move(publishedOn),
        std::move(location),
        price
    );

    isbn_ = std::move(isbn);
    edition_ = edition;
    authors_ = std::move(authors);
    pageCount_ = pageCount;
}

BookRecord Book::toRecord() const noexcept {
    BookRecord record{};

    record.id = getId();
    copyToFixed(record.title, getTitle());
    copyToFixed(record.publisher, getPublisher());
    copyToFixed(record.genre, getGenre());
    copyToFixed(record.publishedOn, getPublishedOn());
    copyToFixed(record.addedOn, getAddedOn());
    copyToFixed(record.location, getLocation());
    record.price = getPrice();
    copyToFixed(record.recordedBy, getRecordedBy());
    record.status = getStatus();

    copyToFixed(record.isbn, isbn_);
    record.edition = edition_;
    copyToFixed(record.authors, authors_);
    record.pageCount = pageCount_;

    return record;
}

std::optional<Book> Book::fromRecord(const BookRecord& record) {
    const auto raw = static_cast<std::uint8_t>(record.status);
    if (raw != static_cast<std::uint8_t>(Status::AVAILABLE) &&
        raw != static_cast<std::uint8_t>(Status::LOANED) &&
        raw != static_cast<std::uint8_t>(Status::DELETED)) {
            return std::nullopt;
        }
    return Book{
        record.id,
        fixedToString(record.title),
        fixedToString(record.publisher),
        fixedToString(record.genre),
        fixedToString(record.publishedOn),
        fixedToString(record.addedOn),
        fixedToString(record.location),
        record.price,
        fixedToString(record.recordedBy),
        fixedToString(record.isbn),
        record.edition,
        fixedToString(record.authors),
        record.pageCount,
        record.status
    };
}

bool operator==(const Book& a, const Book& b) {
    return static_cast<const Medium&>(a) == static_cast<const Medium&>(b)
        && a.getIsbn() == b.getIsbn()
        && a.getEdition() == b.getEdition()
        && a.getAuthors() == b.getAuthors()
        && a.getPageCount() == b.getPageCount();
}

bool operator!=(const Book& a, const Book& b) {
    return !(a == b);
}

} // namespace libster
