#include "LibraryService.h"

#include <cctype>
#include <charconv>
#include <filesystem>
#include <string_view>
#include <system_error>
#include <utility>

#include "core/model/Medium.h"
#include "core/model/Book.h"
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

// Books

ServiceError LibraryService::addBook(Book& book) {
    if (!isOpen()) return ServiceError::NotOpen;
    if (book.getId() != kUnassignedId) return ServiceError::InvalidData;

    std::uint32_t assignedId = kUnassignedId;
    const IoError error = books_.append(book.toRecord(), assignedId);
    if (error != IoError::None) return toServiceError(error);

    book.setId(assignedId);
    return ServiceError::None;
}

ServiceError LibraryService::getBook(std::uint32_t id, std::optional<Book>& out) {
    out.reset();
    if (!isOpen()) return ServiceError::NotOpen;
    
    BookRecord record{};
    const IoError error = books_.read(id, record);
    if (error != IoError::None) return toServiceError(error);

    const auto book = Book::fromRecord(record);
    if (!book) return ServiceError::Persistence;
    if (book->getStatus() == Status::DELETED) return ServiceError::NotFound;

    out = *book;
    return ServiceError::None;
}

ServiceError LibraryService::updateBook(const Book& book) {
    if (!isOpen()) return ServiceError::NotOpen;
    
    BookRecord record{};
    IoError error = books_.read(book.getId(), record);
    if (error != IoError::None) return toServiceError(error);

    auto existing = Book::fromRecord(record);
    if (!existing) return ServiceError::Persistence;
    if (existing->getStatus() == Status::DELETED) return ServiceError::NotFound;

    existing->update(
        book.getTitle(),
        book.getPublisher(),
        book.getGenre(),
        book.getPublishedOn(),
        book.getLocation(),
        book.getPrice(),
        book.getIsbn(),
        book.getEdition(),
        book.getAuthors(),
        book.getPageCount()
    );
    error = books_.write(existing->getId(), existing->toRecord());
    return toServiceError(error);
}

ServiceError LibraryService::deleteBook(std::uint32_t id) {
    if (!isOpen()) return ServiceError::NotOpen;

    BookRecord record{};
    IoError error = books_.read(id, record);
    if (error != IoError::None) return toServiceError(error);

    auto book = Book::fromRecord(record);
    if (!book) return ServiceError::Persistence;
    if (book->getStatus() == Status::DELETED) return ServiceError::NotFound;

    bool hasOpen = false;
    const ServiceError loanError = hasOpenMediumLoan(id, MediumType::BOOK, hasOpen);
    if (loanError != ServiceError::None) return loanError;
    if (hasOpen) return ServiceError::HasOpenLoans;

    book->setStatus(Status::DELETED);

    error = books_.write(id, book->toRecord());
    return toServiceError(error);
}

ServiceError LibraryService::findBooks(const std::string& query, std::vector<Book>& out) {
    out.clear();
    if (!isOpen()) return ServiceError::NotOpen;

    const std::string needle = toLowerAscii(query);

    std::uint32_t idQuery = 0;
    const bool hasIdQuery = parseId(query, idQuery);

    std::vector<BookRecord> records;
    const IoError error = books_.findAll<BookRecord>(
        [&](const BookRecord& record) {
            if (record.status == Status::DELETED) return false;
            if (hasIdQuery && record.id == idQuery) return true;
            if (needle.empty()) return true;

            return containsCaseInsensitive(fixedToString(record.title), needle)
                || containsCaseInsensitive(fixedToString(record.authors), needle)
                || containsCaseInsensitive(fixedToString(record.isbn), needle)
                || containsCaseInsensitive(fixedToString(record.publisher), needle);
        },
        records
    );
    if (error != IoError::None) return toServiceError(error);

    out.reserve(records.size());
    for (const BookRecord& record : records) {
        const auto book = Book::fromRecord(record);
        if (!book) return ServiceError::Persistence;
        out.push_back(*book);
    }

    return ServiceError::None;
}

ServiceError LibraryService::loanBook(std::uint32_t borrowerId, std::uint32_t bookId, std::int64_t loanDate, std::uint32_t& loanId) {
    return loanMedium(MediumType::BOOK, borrowerId, bookId, loanDate, loanId);
}


// CDs

ServiceError LibraryService::addCD(CD& cd) {
    if (!isOpen()) return ServiceError::NotOpen;
    if (cd.getId() != kUnassignedId) return ServiceError::InvalidData;

    std::uint32_t assignedId = kUnassignedId;
    const IoError error = cds_.append(cd.toRecord(), assignedId);
    if (error != IoError::None) return toServiceError(error);

    cd.setId(assignedId);
    return ServiceError::None;
}

ServiceError LibraryService::getCD(std::uint32_t id, std::optional<CD>& out) {
    out.reset();
    if (!isOpen()) return ServiceError::NotOpen;
    
    CDRecord record{};
    const IoError error = cds_.read(id, record);
    if (error != IoError::None) return toServiceError(error);

    const auto cd = CD::fromRecord(record);
    if (!cd) return ServiceError::Persistence;
    if (cd->getStatus() == Status::DELETED) return ServiceError::NotFound;

    out = *cd;
    return ServiceError::None;
}

ServiceError LibraryService::updateCD(const CD& cd) {
    if (!isOpen()) return ServiceError::NotOpen;
    
    CDRecord record{};
    IoError error = cds_.read(cd.getId(), record);
    if (error != IoError::None) return toServiceError(error);

    auto existing = CD::fromRecord(record);
    if (!existing) return ServiceError::Persistence;
    if (existing->getStatus() == Status::DELETED) return ServiceError::NotFound;

    existing->update(
        cd.getTitle(),
        cd.getPublisher(),
        cd.getGenre(),
        cd.getPublishedOn(),
        cd.getLocation(),
        cd.getPrice(),
        cd.getPerformer(),
        cd.getDuration()
        
    );
    error = cds_.write(existing->getId(), existing->toRecord());
    return toServiceError(error);
}

ServiceError LibraryService::deleteCD(std::uint32_t id) {
    if (!isOpen()) return ServiceError::NotOpen;

    CDRecord record{};
    IoError error = cds_.read(id, record);
    if (error != IoError::None) return toServiceError(error);

    auto cd = CD::fromRecord(record);
    if (!cd) return ServiceError::Persistence;
    if (cd->getStatus() == Status::DELETED) return ServiceError::NotFound;

    bool hasOpen = false;
    const ServiceError loanError = hasOpenMediumLoan(id, MediumType::CD, hasOpen);
    if (loanError != ServiceError::None) return loanError;
    if (hasOpen) return ServiceError::HasOpenLoans;

    cd->setStatus(Status::DELETED);

    error = cds_.write(id, cd->toRecord());
    return toServiceError(error);
}

ServiceError LibraryService::findCDs(const std::string& query, std::vector<CD>& out) {
    out.clear();
    if (!isOpen()) return ServiceError::NotOpen;

    const std::string needle = toLowerAscii(query);

    std::uint32_t idQuery = 0;
    const bool hasIdQuery = parseId(query, idQuery);

    std::vector<CDRecord> records;
    const IoError error = cds_.findAll<CDRecord>(
        [&](const CDRecord& record) {
            if (record.status == Status::DELETED) return false;
            if (hasIdQuery && record.id == idQuery) return true;
            if (needle.empty()) return true;

            return containsCaseInsensitive(fixedToString(record.title), needle)
                || containsCaseInsensitive(fixedToString(record.performer), needle)
                || containsCaseInsensitive(fixedToString(record.publisher), needle)
                || containsCaseInsensitive(fixedToString(record.location), needle);
        },
        records
    );
    if (error != IoError::None) return toServiceError(error);

    out.reserve(records.size());
    for (const CDRecord& record : records) {
        const auto cd = CD::fromRecord(record);
        if (!cd) return ServiceError::Persistence;
        out.push_back(*cd);
    }

    return ServiceError::None;
}

ServiceError LibraryService::loanCD(std::uint32_t borrowerId, std::uint32_t cdId, std::int64_t loanDate, std::uint32_t& loanId) {
    return loanMedium(MediumType::CD, borrowerId, cdId, loanDate, loanId);
}

// General

ServiceError LibraryService::returnLoan(std::uint32_t loanId, std::int64_t returnDate) {
    if (!isOpen()) return ServiceError::NotOpen;
    if (returnDate <= 0) return ServiceError::InvalidData;

    LoanTransactionRecord record{};
    IoError error = loans_.read(loanId, record);
    if (error != IoError::None) return toServiceError(error);
    
    auto loan = LoanTransaction::fromRecord(record);
    if (!loan) return ServiceError::Persistence;
    if (loan->getReturnDate() != 0) return ServiceError::AlreadyReturned;
    if (returnDate < loan->getLoanDate()) return ServiceError::InvalidData;

    loan->setReturnDate(returnDate);

    error = loans_.write(loanId, loan->toRecord());
    if (error != IoError::None) return toServiceError(error);

    const ServiceError statusError = setMediumStatus(loan->getType(), loan->getMediumId(), Status::AVAILABLE);
    if (statusError != ServiceError::None) return ServiceError::PartialWrite;

    return ServiceError::None;
}

ServiceError LibraryService::getLoan(std::uint32_t loanId, std::optional<LoanTransaction>& out){
    out.reset();
    if (!isOpen()) return ServiceError::NotOpen;

    LoanTransactionRecord record{};
    IoError error = loans_.read(loanId, record);
    if (error != IoError::None) return toServiceError(error);

    const auto loan = LoanTransaction::fromRecord(record);
    if (!loan) return ServiceError::Persistence;

    out = *loan;
    return ServiceError::None;
}

ServiceError LibraryService::findOpenLoans(std::uint32_t borrowerId, std::vector<LoanTransaction>& out){
    out.clear();
    if (!isOpen()) return ServiceError::NotOpen;

    std::vector<LoanTransactionRecord> records;
    const IoError error = loans_.findAll<LoanTransactionRecord>(
        [borrowerId](const LoanTransactionRecord& loan) {
            return loan.borrowerId == borrowerId && loan.returnDate == 0;
        }, records
    );
    if (error != IoError::None) return toServiceError(error);

    out.reserve(records.size());
    for (const LoanTransactionRecord& record : records) {
        const auto loan = LoanTransaction::fromRecord(record);
        if (!loan) return ServiceError::Persistence;
        out.push_back(*loan);
    }

    return ServiceError::None;
}

ServiceError LibraryService::dailyReport(std::int64_t dayStart, std::int64_t dayEnd, DailyReport& out) {
    out = DailyReport{};
    if (!isOpen()) return ServiceError::NotOpen;
    if (dayStart < 0 || dayEnd <= dayStart) return ServiceError::InvalidData;

    // Nur vorgänge laden, die im Zeitraum relevant sind
    const auto relevant = [dayStart, dayEnd](const LoanTransactionRecord& loan) {
        const bool borrowed = loan.loanDate >= dayStart && loan.loanDate < dayEnd;
        const bool returned = loan.returnDate != 0 && loan.returnDate >= dayStart && loan.returnDate < dayEnd;
        return borrowed || returned;
    };
    std::vector<LoanTransactionRecord> records;
    const IoError error = loans_.findAll<LoanTransactionRecord>(relevant, records);
    if (error != IoError::None) return toServiceError(error);

    for (const LoanTransactionRecord& loan : records) {
        if (loan.loanDate >= dayStart && loan.loanDate < dayEnd) {
            ++out.loanCount;
        }
        if (loan.returnDate != 0 && loan.returnDate >= dayStart && loan.returnDate < dayEnd) {
            ++out.returnCount;
        }
    }

    return ServiceError::None;
}

// Session

void LibraryService::setRecordedBy(std::string recordedBy){
    recordedBy_ = std::move(recordedBy);
}

const std::string& LibraryService::getRecordedBy() const noexcept{
    return recordedBy_;
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

ServiceError LibraryService::hasOpenMediumLoan(std::uint32_t mediumId, MediumType type, bool& result) {
    result = false;

    std::vector<LoanTransactionRecord> openLoans;
    const IoError error = loans_.findAll<LoanTransactionRecord>(
        [mediumId, type](const LoanTransactionRecord& loan) {
            return loan.mediumId == mediumId && loan.type == type && loan.returnDate == 0;
        },
        openLoans
    );
    if (error != IoError::None) return toServiceError(error);

    result = !openLoans.empty();
    return ServiceError::None;
}

ServiceError LibraryService::loanMedium(MediumType type, std::uint32_t borrowerId, uint32_t mediumId, std::int64_t loanDate, std::uint32_t& loanId){
    loanId = 0;
    if (!isOpen()) return ServiceError::NotOpen;
    if (loanDate < 0) return ServiceError::InvalidData;

    std::optional<Borrower> borrower;
    const ServiceError borrowerError = getBorrower(borrowerId, borrower);
    if (borrowerError != ServiceError::None) return borrowerError;

    Status mediumStatus = Status::AVAILABLE;
    if (type == MediumType::BOOK) {
        BookRecord record{};
        const IoError error = books_.read(mediumId, record);
        if (error != IoError::None) return toServiceError(error);
        
        const auto book = Book::fromRecord(record);
        if (!book) return ServiceError::Persistence;
        if (book->getStatus() == Status::DELETED) return ServiceError::NotFound;

        mediumStatus = book->getStatus();
    } else if (type == MediumType::CD) {
        CDRecord record{};
        const IoError error = cds_.read(mediumId, record);
        if (error != IoError::None) return toServiceError(error);

        const auto cd = CD::fromRecord(record);
        if (!cd) return ServiceError::Persistence;
        if (cd->getStatus() == Status::DELETED) return ServiceError::NotFound;

        mediumStatus = cd->getStatus();
    } else {
        return ServiceError::InvalidData;
    }

    if (mediumStatus != Status::AVAILABLE) return ServiceError::NotAvailable;

    LoanTransaction loan {
        kUnassignedId,
        borrowerId,
        mediumId,
        type,
        loanDate,
        0,
        recordedBy_
    };

    std::uint32_t assignedId = kUnassignedId;
    const IoError appendError = loans_.append(loan.toRecord(), assignedId);
    if (appendError != IoError::None) return toServiceError(appendError);

    loanId = assignedId;

    const ServiceError statusError = setMediumStatus(type, mediumId, Status::LOANED);
    if (statusError != ServiceError::None) return ServiceError::PartialWrite;

    return ServiceError::None;
}

ServiceError LibraryService::setMediumStatus(MediumType type, std::uint32_t mediumId, Status status){
    if (!isOpen()) return ServiceError::NotOpen;

    if (type == MediumType::BOOK) {
        BookRecord record{};
        IoError error = books_.read(mediumId, record);
        if (error != IoError::None) return toServiceError(error);

        auto book = Book::fromRecord(record);
        if (!book) return ServiceError::Persistence;

        book->setStatus(status);
        error = books_.write(mediumId, book->toRecord());
        return toServiceError(error);
    }

    if (type == MediumType::CD) {
        CDRecord record{};
        IoError error = cds_.read(mediumId, record);
        if (error != IoError::None) return toServiceError(error);

        auto cd = CD::fromRecord(record);
        if (!cd) return ServiceError::Persistence;

        cd->setStatus(status);
        error = cds_.write(mediumId, cd->toRecord());
        return toServiceError(error);
    }

    return ServiceError::InvalidData;
}

} // namespace libster
