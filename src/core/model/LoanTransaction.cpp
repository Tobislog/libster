#include "LoanTransaction.h"

#include <utility>

#include "core/persistence/CStringUtil.h"

namespace libster {

LoanTransaction::LoanTransaction (
    std::uint32_t id,
    std::uint32_t borrowerId,
    std::uint32_t mediumId,
    MediumType type,
    std::int64_t loanDate,
    std::int64_t returnDate,    // 0 = noch nicht zurückgegeben
    std::string recordedBy
) : id_(id),
    borrowerId_(borrowerId),
    mediumId_(mediumId),
    type_(type),
    loanDate_(loanDate),
    returnDate_(returnDate),
    recordedBy_(std::move(recordedBy))
{}

std::uint32_t LoanTransaction::getId() const noexcept {
    return id_;
}

std::uint32_t LoanTransaction::getBorrowerId() const noexcept {
    return borrowerId_;
}

std::uint32_t LoanTransaction::getMediumId() const noexcept {
    return mediumId_;
}

MediumType LoanTransaction::getType() const noexcept {
    return type_;
}

std::int64_t LoanTransaction::getLoanDate() const noexcept {
    return loanDate_;
}

std::int64_t LoanTransaction::getReturnDate() const noexcept {
    return returnDate_;
}

const std::string& LoanTransaction::getRecordedBy() const noexcept {
    return recordedBy_;
}

void LoanTransaction::setId(std::uint32_t id) noexcept {
    id_ = id;
}

void LoanTransaction::setReturnDate(std::int64_t returnDate) noexcept {
    returnDate_ = returnDate;
}

LoanTransactionRecord LoanTransaction::toRecord() const noexcept {
    LoanTransactionRecord record{};

    record.id = id_;
    record.borrowerId = borrowerId_;
    record.mediumId = mediumId_;
    record.type = type_;
    record.loanDate = loanDate_;
    record.returnDate = returnDate_;
    copyToFixed(record.recordedBy, recordedBy_);

    return record;
}

std::optional<LoanTransaction> LoanTransaction::fromRecord(const LoanTransactionRecord& record) {
    const auto rawType = static_cast<std::uint8_t>(record.type);
    if (rawType != static_cast<std::uint8_t>(MediumType::BOOK) &&
        rawType != static_cast<std::uint8_t>(MediumType::CD)) {
        return std::nullopt;
    }

    if (record.loanDate < 0 || record.returnDate < 0) {
        return std::nullopt;
    }

    // 0 bedeutet noch offen
    // Ein vorhandenes Rückgabedatum darf nicht vor dem Ausleihdatum liegen
    if (record.returnDate != 0 && record.returnDate < record.loanDate) {
        return std::nullopt;
    }

    return LoanTransaction{
        record.id,
        record.borrowerId,
        record.mediumId,
        record.type,
        record.loanDate,
        record.returnDate,
        fixedToString(record.recordedBy)
    };
}

bool operator==(const LoanTransaction& a, const LoanTransaction& b) {
    return a.getId() == b.getId()
        && a.getBorrowerId() == b.getBorrowerId()
        && a.getMediumId() == b.getMediumId()
        && a.getType() == b.getType()
        && a.getLoanDate() == b.getLoanDate()
        && a.getReturnDate() == b.getReturnDate()
        && a.getRecordedBy() == b.getRecordedBy();
}

bool operator!=(const LoanTransaction& a, const LoanTransaction& b) {
    return !(a == b);
}

} // namespace libster
