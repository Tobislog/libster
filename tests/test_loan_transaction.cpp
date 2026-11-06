#include <cstdint>
#include <string>

#include "TestSupport.h"
#include "core/model/LoanTransaction.h"
#include "core/persistence/CStringUtil.h"
#include "core/persistence/LoanTransactionRecord.h"

namespace {

using libster::LoanTransaction;
using libster::LoanTransactionRecord;
using libster::MediumType;
using libster::test::TestContext;

LoanTransaction makeOpenLoan() {
    return LoanTransaction{
        1,
        42, 
        7, 
        MediumType::BOOK, 
        1760000000LL,
        0,
        "Tester"
    };
}

LoanTransaction makeReturnedLoan() {
    return LoanTransaction{
        2,
        42,
        9,
        MediumType::CD,
        1760000000LL,
        1760003600LL,
        "Tester"
    };
}

void testOpenLoanRoundtrip(TestContext& t) {
    const LoanTransaction original = makeOpenLoan();
    const auto restored = LoanTransaction::fromRecord(original.toRecord());

    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, *restored == original);
    CHECK(t, restored->getReturnDate() == 0);
    CHECK(t, restored->getType() == MediumType::BOOK);
}

void testReturnedLoanRoundtrip(TestContext& t) {
    const LoanTransaction original = makeReturnedLoan();
    const auto restored = LoanTransaction::fromRecord(original.toRecord());

    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, *restored == original);
    CHECK(t, restored->getReturnDate() == 1760003600LL);
    CHECK(t, restored->getType() == MediumType::CD);
}

void testToRecordMapsAllFields(TestContext& t) {
    const LoanTransaction original = makeReturnedLoan();
    const LoanTransactionRecord record = original.toRecord();

    CHECK(t, record.id == original.getId());
    CHECK(t, record.borrowerId == original.getBorrowerId());
    CHECK(t, record.mediumId == original.getMediumId());
    CHECK(t, record.type == original.getType());
    CHECK(t, record.loanDate == original.getLoanDate());
    CHECK(t, record.returnDate == original.getReturnDate());
    CHECK(t, libster::fixedToString(record.recordedBy) == original.getRecordedBy());
}

void testSetReturnDate(TestContext& t) {
    LoanTransaction loan = makeOpenLoan();
    CHECK(t, loan.getReturnDate() == 0);

    loan.setReturnDate(1760007200LL);
    CHECK(t, loan.getReturnDate() == 1760007200LL);

    const auto restored = LoanTransaction::fromRecord(loan.toRecord());
    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, restored->getReturnDate() == 1760007200LL);
}

void testInvalidTypeIsRejected(TestContext& t) {
    LoanTransactionRecord record = makeOpenLoan().toRecord();
    record.type = static_cast<MediumType>(3);

    CHECK(t, !LoanTransaction::fromRecord(record).has_value());
}

void testNegativeDatesAreRejected(TestContext& t) {
    LoanTransactionRecord record = makeOpenLoan().toRecord();
    record.loanDate = -1;
    CHECK(t, !LoanTransaction::fromRecord(record).has_value());

    record = makeOpenLoan().toRecord();
    record.returnDate = -1;
    CHECK(t, !LoanTransaction::fromRecord(record).has_value());
}

void testReturnBeforeLoanIsRejected(TestContext& t) {
    LoanTransactionRecord record = makeReturnedLoan().toRecord();
    record.returnDate = record.loanDate - 1;

    CHECK(t, !LoanTransaction::fromRecord(record).has_value());
}

void testOverlongRecordedByIsTruncated(TestContext& t) {
    const LoanTransaction original{
        3,
        42,
        7,
        MediumType::BOOK,
        1760000000LL,
        0,
        std::string(100, 'R')
    };

    const LoanTransactionRecord record = original.toRecord();
    CHECK(t, record.recordedBy[63] == '\0');

    const auto restored = LoanTransaction::fromRecord(record);
    CHECK(t, restored.has_value());
    if (!restored) return;

    CHECK(t, restored->getRecordedBy() == std::string(63, 'R'));
}

} // namespace

void runLoanTransactionTests(libster::test::TestContext& t) {
    testOpenLoanRoundtrip(t);
    testReturnedLoanRoundtrip(t);
    testToRecordMapsAllFields(t);
    testSetReturnDate(t);
    testInvalidTypeIsRejected(t);
    testNegativeDatesAreRejected(t);
    testReturnBeforeLoanIsRejected(t);
    testOverlongRecordedByIsTruncated(t);
}
