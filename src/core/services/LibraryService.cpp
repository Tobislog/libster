#include "LibraryService.h"

#include <cctype>
#include <charconv>
#include <filesystem>
#include <string_view>
#include <system_error>

#include "core/persistence/BorrowerRecord.h"
#include "core/persistence/BookRecord.h"
#include "core/persistence/CDRecord.h"
#include "core/persistence/LoanTransactionRecord.h"
#include "core/persistence/FileType.h"
#include "core/persistence/CStringUtil.h"

namespace libster {

namespace {

std::string joinPath(const std::string& directory, const char* fileName) {
    return (std::filesystem::path(directory) / fileName).string();
}

ServiceError toServiceError(IoError error) noexcept {
    switch (error) {
        case IoError::None:         return ServiceError::None;
        case IoError::NotOpen:      return ServiceError::NotOpen;
        case IoError::InvalidId:    return ServiceError::NotFound;
        default:                    return ServiceError::Persistence;
    }
}

std::string toLowerAscii(std::string_view text) {
    std::string lowered;
    lowered.reserve(text.size());

    for (unsigned char c : text) {
        lowered.push_back(static_cast<char>(std::tolower(c)));
    }

    return lowered;
}

bool containsCaseInsensitive(std::string_view haystack, const std::string& loweredNeedle) {
    return toLowerAscii(haystack).find(loweredNeedle) != std::string::npos;
}

bool parseId(std::string_view text, std::uint32_t& id) {
    if (text.empty()) return false;

    std::uint32_t value = 0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const auto result = std::from_chars(begin, end, value);

    if (result.ec != std::errc{} || result.ptr != end) {
        return false;
    }

    id = value;
    return true;
}

} // namespace

LibraryService::~LibraryService() {
    close();
}

IoError LibraryService::open(const std::string& dataDir) {
    close();

    std::error_code ec;
    std::filesystem::create_directories(dataDir, ec);
    if (ec) return IoError::CannotOpen;

    IoError error = borrowers_.openOrCreate<BorrowerRecord>(joinPath(dataDir, "borrowers.bin"), FileType::Borrower);
    if (error != IoError::None) {
        close();
        return error;
    }
    error = books_.openOrCreate<BookRecord>(joinPath(dataDir, "books.bin"), FileType::Book);
    if (error != IoError::None) {
        close();
        return error;
    }
    error = cds_.openOrCreate<CDRecord>(joinPath(dataDir, "cds.bin"), FileType::CD);
    if (error != IoError::None) {
        close();
        return error;
    }
    error = loans_.openOrCreate<LoanTransactionRecord>(joinPath(dataDir, "loan_transactions.bin"), FileType::LoanTransaction);
    if (error != IoError::None) {
        close();
        return error;
    }

    return IoError::None;
}

void LibraryService::close() noexcept {
    borrowers_.close();
    books_.close();
    cds_.close();
    loans_.close();
}

bool LibraryService::isOpen() const noexcept {
    return borrowers_.isOpen()
        && books_.isOpen()
        && cds_.isOpen()
        && loans_.isOpen();
}

// Borrower

ServiceError LibraryService::addBorrower(Borrower& borrower) {
    if (!isOpen()) return ServiceError::NotOpen;
    if (borrower.getId() != kUnassignedId) return ServiceError::InvalidData;

    std::uint32_t assignedId = kUnassignedId;
    const IoError error = borrowers_.append(borrower.toRecord(), assignedId);
    if (error != IoError::None) return toServiceError(error);

    borrower.setId(assignedId);
    return ServiceError::None;
}

ServiceError LibraryService::getBorrower(std::uint32_t id, std::optional<Borrower>& out) {
    out.reset();
    if (!isOpen()) return ServiceError::NotOpen;
    
    BorrowerRecord record{};
    const IoError error = borrowers_.read(id, record);
    if (error != IoError::None) return toServiceError(error);

    const auto borrower = Borrower::fromRecord(record);
    if (!borrower) return ServiceError::Persistence;
    if (borrower->getStatus() == BorrowerStatus::DELETED) return ServiceError::NotFound;

    out = *borrower;
    return ServiceError::None;
}

ServiceError LibraryService::updateBorrower(const Borrower& borrower) {
    if (!isOpen()) return ServiceError::NotOpen;
    
    BorrowerRecord record{};
    IoError error = borrowers_.read(borrower.getId(), record);
    if (error != IoError::None) return toServiceError(error);

    auto existing = Borrower::fromRecord(record);
    if (!existing) return ServiceError::Persistence;
    if (existing->getStatus() == BorrowerStatus::DELETED) return ServiceError::NotFound;

    existing->update(
        borrower.getLastName(),
        borrower.getFirstName(),
        borrower.getEmail(),
        borrower.getAddress()
    );
    error = borrowers_.write(existing->getId(), existing->toRecord());
    return toServiceError(error);
}

ServiceError LibraryService::deleteBorrower(std::uint32_t id) {
    if (!isOpen()) return ServiceError::NotOpen;

    BorrowerRecord record{};
    IoError error = borrowers_.read(id, record);
    if (error != IoError::None) return toServiceError(error);

    auto borrower = Borrower::fromRecord(record);
    if (!borrower) return ServiceError::Persistence;
    if (borrower->getStatus() == BorrowerStatus::DELETED) return ServiceError::NotFound;

    bool hasOpen = false;
    const ServiceError loanError = hasOpenLoans(id, hasOpen);
    if (loanError != ServiceError::None) return loanError;
    if (hasOpen) return ServiceError::HasOpenLoans;

    borrower->markDeleted();

    error = borrowers_.write(id, borrower->toRecord());
    return toServiceError(error);
}

ServiceError LibraryService::findBorrowers(const std::string& query, std::vector<Borrower>& out) {
    out.clear();
    if (!isOpen()) return ServiceError::NotOpen;

    const std::string needle = toLowerAscii(query);

    std::uint32_t idQuery = 0;
    const bool hasIdQuery = parseId(query, idQuery);

    std::vector<BorrowerRecord> records;
    const IoError error = borrowers_.findAll<BorrowerRecord>(
        [&](const BorrowerRecord& record) {
            if (record.status == BorrowerStatus::DELETED) return false;
            if (hasIdQuery && record.id == idQuery) return true;
            if (needle.empty()) return true;

            return containsCaseInsensitive(fixedToString(record.lastName), needle)
                || containsCaseInsensitive(fixedToString(record.firstName), needle)
                || containsCaseInsensitive(fixedToString(record.email), needle);
        },
        records
    );
    if (error != IoError::None) return toServiceError(error);

    out.reserve(records.size());
    for (const BorrowerRecord& record : records) {
        const auto borrower = Borrower::fromRecord(record);
        if (!borrower) return ServiceError::Persistence;
        out.push_back(*borrower);
    }

    return ServiceError::None;
}

ServiceError LibraryService::hasOpenLoans(std::uint32_t borrowerId, bool& result) {
    result = false;

    std::vector<LoanTransactionRecord> openLoans;
    const IoError error = loans_.findAll<LoanTransactionRecord>(
        [borrowerId](const LoanTransactionRecord& loan) {
            return loan.borrowerId == borrowerId && loan.returnDate == 0;
        },
        openLoans
    );
    if (error != IoError::None) return toServiceError(error);

    result = !openLoans.empty();
    return ServiceError::None;
}
} // namespace libster
