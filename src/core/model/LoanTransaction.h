#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "MediumType.h"
#include "core/persistence/LoanTransactionRecord.h"

namespace libster {

class LoanTransaction {
public: 
    LoanTransaction(
        std::uint32_t id,
        std::uint32_t borrowerId,
        std::uint32_t mediumId,
        MediumType type,
        std::int64_t loanDate,
        std::int64_t returnDate,    // 0 = noch nicht zurückgegeben
        std::string recordedBy
    );

    std::uint32_t getId() const noexcept;
    std::uint32_t getBorrowerId() const noexcept;
    std::uint32_t getMediumId() const noexcept;
    MediumType getType() const noexcept;
    std::int64_t getLoanDate() const noexcept;
    std::int64_t getReturnDate() const noexcept;
    const std::string& getRecordedBy() const noexcept;

    void setId(std::uint32_t id) noexcept;
    void setReturnDate(std::int64_t returnDate) noexcept;

    LoanTransactionRecord toRecord() const noexcept;
    static std::optional<LoanTransaction> fromRecord(const LoanTransactionRecord& record);

private:
    std::uint32_t id_;
    std::uint32_t borrowerId_;
    std::uint32_t mediumId_;
    MediumType type_;
    std::int64_t loanDate_;
    std::int64_t returnDate_;
    std::string recordedBy_;
};

bool operator==(const LoanTransaction& a, const LoanTransaction& b);
bool operator!=(const LoanTransaction& a, const LoanTransaction& b);

} // namespace libster
