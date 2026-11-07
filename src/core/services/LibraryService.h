#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "core/model/Borrower.h"
#include "core/model/Book.h"
#include "core/model/CD.h"
#include "core/persistence/BinaryFile.h"
#include "core/persistence/IoError.h"
#include "core/services/ServiceError.h"

namespace libster {

class LibraryService {
public:
    LibraryService() = default;
    ~LibraryService();

    LibraryService( const LibraryService&) = delete;
    LibraryService& operator=(const LibraryService&) = delete;

    IoError open(const std::string& dataDir);
    void close() noexcept;
    bool isOpen() const noexcept;

    // Borrower
    ServiceError addBorrower(Borrower& borrower);
    ServiceError getBorrower(std::uint32_t id, std::optional<Borrower>& out);
    ServiceError updateBorrower(const Borrower& borrower);
    ServiceError deleteBorrower(std::uint32_t id);
    ServiceError findBorrowers(const std::string& query, std::vector<Borrower>& out);

    // Book
    ServiceError addBook(Book& book);
    ServiceError getBook(std::uint32_t id, std::optional<Book>& out);
    ServiceError updateBook(const Book& book);
    ServiceError deleteBook(std::uint32_t id);
    ServiceError findBooks(const std::string& query, std::vector<Book>& out);

    // CD
    ServiceError addCD(CD& cd);
    ServiceError getCD(std::uint32_t id, std::optional<CD>& out);
    ServiceError updateCD(const CD& cd);
    ServiceError deleteCD(std::uint32_t id);
    ServiceError findCDs(const std::string& query, std::vector<CD>& out);

private:
    ServiceError hasOpenLoans(std::uint32_t borrowerId, bool& result);
    ServiceError hasOpenMediumLoan(std::uint32_t mediumId, MediumType type, bool& result);

    BinaryFile borrowers_;
    BinaryFile books_;
    BinaryFile cds_;
    BinaryFile loans_;
};

} // namespace libster
