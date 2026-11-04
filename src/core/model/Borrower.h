#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "BorrowerStatus.h"
#include "core/persistence/BorrowerRecord.h"

namespace libster {

inline constexpr std::uint32_t kUnassignedId = 0; // noch nicht persistiert

class Borrower {
public:
    Borrower(
        std::uint32_t id,
        std::string lastName,
        std::string firstName,
        std::string email,
        std::string address,
        std::string recordedOn,
        std::string recordedBy,
        BorrowerStatus status = BorrowerStatus::ACTIVE
    );

    std::uint32_t getId() const noexcept;
    const std::string& getLastName() const noexcept;
    const std::string& getFirstName() const noexcept;
    const std::string& getEmail() const noexcept;
    const std::string& getAddress() const noexcept;
    const std::string& getRecordedOn() const noexcept;
    const std::string& getRecordedBy() const noexcept;
    BorrowerStatus getStatus() const noexcept;

    void setId(std::uint32_t id) noexcept;          // nur nach Append durch Persistenz/Service
    void update(
        std::string lastName, 
        std::string firstName,
        std::string email,
        std::string address
    ); // recordedOn/By bleiben unveränderliche Metadaten
    
    BorrowerRecord toRecord() const noexcept;
    static std::optional<Borrower> fromRecord(const BorrowerRecord& record);

private:
    std::uint32_t id_;
    std::string lastName_, firstName_, email_, address_;
    std::string recordedOn_, recordedBy_;
    BorrowerStatus status_;
};

bool operator==(const Borrower& a, const Borrower& b);
bool operator!=(const Borrower& a, const Borrower& b);


} // namespace libster
