#include "Borrower.h"

#include <utility> // std::move

#include "core/persistence/CStringUtil.h"

namespace libster {

Borrower::Borrower(
        std::uint32_t id,
        std::string lastName,
        std::string firstName,
        std::string email,
        std::string address,
        std::string recordedOn,
        std::string recordedBy,
        BorrowerStatus status
    ) : id_(id),
        lastName_(std::move(lastName)),
        firstName_(std::move(firstName)),
        email_(std::move(email)),
        address_(std::move(address)),
        recordedOn_(std::move(recordedOn)),
        recordedBy_(std::move(recordedBy)),
        status_(status)
{}

std::uint32_t Borrower::getId() const noexcept {
    return id_;
}
const std::string& Borrower::getLastName() const noexcept {
    return lastName_;
}
const std::string& Borrower::getFirstName() const noexcept {
    return firstName_;
}
const std::string& Borrower::getEmail() const noexcept {
    return email_;
}
const std::string& Borrower::getAddress() const noexcept {
    return address_;
}
const std::string& Borrower::getRecordedOn() const noexcept {
    return recordedOn_;
}
const std::string& Borrower::getRecordedBy() const noexcept {
    return recordedBy_;
}
BorrowerStatus Borrower::getStatus() const noexcept {
    return status_;
}

void Borrower::setId(std::uint32_t id) noexcept { // nur nach Append durch Persistenz/Service
    id_ = id;
}

void Borrower::update(std::string lastName, std::string firstName, std::string email, std::string address) { // recordedOn/By bleiben unveränderliche Metadaten
    lastName_ = std::move(lastName);
    firstName_ = std::move(firstName);
    email_ = std::move(email);
    address_ = std::move(address);
}

BorrowerRecord Borrower::toRecord() const noexcept {
    BorrowerRecord record {};
    record.id = id_;
    copyToFixed(record.lastName, lastName_);
    copyToFixed(record.firstName, firstName_);
    copyToFixed(record.email, email_);
    copyToFixed(record.address, address_);
    copyToFixed(record.recordedOn, recordedOn_);
    copyToFixed(record.recordedBy, recordedBy_);
    record.status = status_;

    return record;
}

std::optional<Borrower> Borrower::fromRecord(const BorrowerRecord& record) {
    const auto raw = static_cast<std::uint8_t>(record.status);
    if (raw != static_cast<std::uint8_t>(BorrowerStatus::ACTIVE) &&
        raw != static_cast<std::uint8_t>(BorrowerStatus::DELETED)) {
            return std::nullopt;
        }
    return Borrower{
        record.id,
        fixedToString(record.lastName),
        fixedToString(record.firstName),
        fixedToString(record.email),
        fixedToString(record.address),
        fixedToString(record.recordedOn),
        fixedToString(record.recordedBy),
        record.status
    };
}

bool operator==(const Borrower& a, const Borrower& b) {
    return a.getId() == b.getId()
        && a.getLastName() == b.getLastName()
        && a.getFirstName() == b.getFirstName()
        && a.getEmail() == b.getEmail()
        && a.getAddress() == b.getAddress()
        && a.getRecordedOn() == b.getRecordedOn()
        && a.getRecordedBy() == b.getRecordedBy()
        && a.getStatus() == b.getStatus();
}

bool operator!=(const Borrower& a, const Borrower& b) {
    return !(a == b);
}

} // namespace libster
