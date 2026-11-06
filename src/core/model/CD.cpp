#include "CD.h"

#include <utility>

#include "core/persistence/CStringUtil.h"

namespace libster {

CD::CD(
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
        Status status
) : Medium(
    id,
    std::move(title),
    std::move(publisher),
    std::move(genre),
    std::move(publishedOn),
    std::move(addedOn),
    std::move(location),
    price,
    std::move(recordedBy),
    status
),  performer_(std::move(performer)),
    duration_(duration)
{}

MediumType CD::getType() const noexcept {
    return MediumType::CD;
}

const std::string& CD::getPerformer() const noexcept {
    return performer_;
}

std::uint16_t CD::getDuration() const noexcept {
    return duration_;
}

void CD::update(
    std::string title,
        std::string publisher,
        std::string genre,
        std::string publishedOn,
        std::string location,
        std::uint32_t price,
        std::string performer,
        std::uint16_t duration
) {
    updateCommon(
        std::move(title),
        std::move(publisher),
        std::move(genre),
        std::move(publishedOn),
        std::move(location),
        price 
    );

    performer_ = std::move(performer);
    duration_ = duration;
}

CDRecord CD::toRecord() const noexcept {
    CDRecord record{};

    record.id = getId();
    copyToFixed(record.title, getTitle());
    copyToFixed(record.publisher, getPublisher());
    copyToFixed(record.genre, getGenre());
    copyToFixed(record.publishedOn, getPublishedOn());
    copyToFixed(record.addedOn, getAddedOn());
    copyToFixed(record.location, getLocation());
    record.price = getPrice();
    copyToFixed(record.recordedBy, getRecordedBy());
    record.status = getStatus();

    copyToFixed(record.performer, getPerformer());
    record.duration = getDuration();

    return record;
}

std::optional<CD> CD::fromRecord(const CDRecord& record) {
    const auto raw = static_cast<std::uint8_t>(record.status);
    if (raw != static_cast<std::uint8_t>(Status::AVAILABLE) &&
        raw != static_cast<std::uint8_t>(Status::LOANED) &&
        raw != static_cast<std::uint8_t>(Status::DELETED)) {
            return std::nullopt;
        }
    return CD {
        record.id,
        fixedToString(record.title),
        fixedToString(record.publisher),
        fixedToString(record.genre),
        fixedToString(record.publishedOn),
        fixedToString(record.addedOn),
        fixedToString(record.location),
        record.price,
        fixedToString(record.recordedBy),
        fixedToString(record.performer),
        record.duration,
        record.status
    };
}

bool operator==(const CD& a, const CD& b) {
    return static_cast<const Medium&>(a) == static_cast<const Medium&>(b)
        && a.getPerformer() == b.getPerformer()
        && a.getDuration() == b.getDuration();
}
bool operator!=(const CD& a, const CD& b) {
    return !(a == b);
}

} // namespace libster
