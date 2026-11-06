#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "Medium.h"
#include "core/persistence/CDRecord.h"

namespace libster {

class CD final : public Medium {
public:
    CD(
        std::uint32_t id,
        std::string title,
        std::string publisher,
        std::string genre,
        std::string publishedOn,
        std::string addedOn,
        std::string location,
        std::uint32_t price,
        std::string recordedBy,
        std::string performer,
        std::uint16_t duration,
        Status status = Status::AVAILABLE
    );

    MediumType getType() const noexcept override;

    const std::string& getPerformer() const noexcept;
    std::uint16_t getDuration() const noexcept;

    void update(
        std::string title,
        std::string publisher,
        std::string genre,
        std::string publishedOn,
        std::string location,
        std::uint32_t price,
        std::string performer,
        std::uint16_t duration
    );

    CDRecord toRecord() const noexcept;
    static std::optional<CD> fromRecord(const CDRecord& record);
private:
    std::string performer_;
    std::uint16_t duration_;
};

bool operator==(const CD& a, const CD& b);
bool operator!=(const CD& a, const CD& b);
    
} // namespace libster
