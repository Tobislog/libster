#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "Medium.h"
#include "core/persistence/BookRecord.h"

namespace libster {

class Book final : public Medium {
public:
    Book(
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
        Status status = Status::AVAILABLE
    );
    
    MediumType getType() const noexcept override;

    const std::string& getIsbn() const noexcept;
    std::uint16_t getEdition() const noexcept;
    const std::string& getAuthors() const noexcept;
    std::uint16_t getPageCount() const noexcept;

    void update(
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
    );

    BookRecord toRecord() const noexcept;
    static std::optional<Book> fromRecord(const BookRecord& record);
private:
    std::string isbn_;
    std::uint16_t edition_;
    std::string authors_;
    std::uint16_t pageCount_;
};

bool operator==(const Book& a, const Book& b);
bool operator!=(const Book& a, const Book& b);

} // namespace libster
