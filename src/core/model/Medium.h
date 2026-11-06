#pragma once

#include <cstdint>
#include <string>

#include "MediumType.h"
#include "Status.h"

namespace libster {

class Medium {
public:
    virtual ~Medium() = default;

    virtual MediumType getType() const noexcept = 0; // macht Medium abstrakt

    std::uint32_t getId() const noexcept;
    const std::string& getTitle() const noexcept;
    const std::string& getPublisher() const noexcept;
    const std::string& getGenre() const noexcept;
    const std::string& getPublishedOn() const noexcept;
    const std::string& getAddedOn() const noexcept;
    const std::string& getLocation() const noexcept;
    std::uint32_t getPrice() const noexcept;
    const std::string& getRecordedBy() const noexcept;
    Status getStatus() const noexcept;

    void setId(std::uint32_t id) noexcept;
    void setStatus(Status status) noexcept;

protected:
    Medium(
        std::uint32_t id,
        std::string title,
        std::string publisher,
        std::string genre,
        std::string publishedOn,
        std::string addedOn,
        std::string location,
        std::uint32_t price,
        std::string recordedBy,
        Status status = Status::AVAILABLE
    );

    void updateCommon(
        std::string title,
        std::string publisher,
        std::string genre,
        std::string publishedOn,
        std::string location,
        std::uint32_t price
    );

private:
    std::uint32_t id_;
    std::string title_;
    std::string publisher_;
    std::string genre_;
    std::string publishedOn_;
    std::string addedOn_;
    std::string location_;
    std::uint32_t price_;
    std::string recordedBy_;
    Status status_;
};

bool operator==(const Medium& a, const Medium& b);
bool operator!=(const Medium& a, const Medium& b);

} // namespace libster
