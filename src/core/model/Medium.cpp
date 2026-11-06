#include "Medium.h"

#include <utility>

namespace libster {
    
Medium::Medium(
    std::uint32_t id,
    std::string title,
    std::string publisher,
    std::string genre,
    std::string publishedOn,
    std::string addedOn,
    std::string location,
    std::uint32_t price,
    std::string recordedBy,
    Status status
) : id_(id),
    title_(std::move(title)),
    publisher_(std::move(publisher)),
    genre_(std::move(genre)),
    publishedOn_(std::move(publishedOn)),
    addedOn_(std::move(addedOn)),
    location_(std::move(location)),
    price_(price),
    recordedBy_(std::move(recordedBy)),
    status_(status)
{}

std::uint32_t Medium::getId() const noexcept {
    return id_;
}

const std::string& Medium::getTitle() const noexcept {
    return title_;
}

const std::string& Medium::getPublisher() const noexcept {
    return publisher_;
}

const std::string& Medium::getGenre() const noexcept {
    return genre_;
}

const std::string& Medium::getPublishedOn() const noexcept {
    return publishedOn_;
}

const std::string& Medium::getAddedOn() const noexcept {
    return addedOn_;
}

const std::string& Medium::getLocation() const noexcept {
    return location_;
}

std::uint32_t Medium::getPrice() const noexcept {
    return price_;
}

const std::string& Medium::getRecordedBy() const noexcept {
    return recordedBy_;
}

Status Medium::getStatus() const noexcept {
    return status_;
}

void Medium::setId(std::uint32_t id) noexcept {
    id_ = id;
}

void Medium::setStatus(Status status) noexcept {
    status_ = status;
}

void Medium::updateCommon(
    std::string title,
    std::string publisher,
    std::string genre,
    std::string publishedOn,
    std::string location,
    std::uint32_t price
) {
    title_ = std::move(title);
    publisher_ = std::move(publisher);
    genre_ = std::move(genre);
    publishedOn_ = std::move(publishedOn);
    location_ = std::move(location);
    price_ = price;
}

bool operator==(const Medium& a, const Medium& b) {
    return a.getType() == b.getType()
        && a.getId() == b.getId()
        && a.getTitle() == b.getTitle()
        && a.getPublisher() == b.getPublisher()
        && a.getGenre() == b.getGenre()
        && a.getPublishedOn() == b.getPublishedOn()
        && a.getAddedOn() == b.getAddedOn()
        && a.getLocation() == b.getLocation()
        && a.getPrice() == b.getPrice()
        && a.getRecordedBy() == b.getRecordedBy()
        && a.getStatus() == b.getStatus();
}

bool operator!=(const Medium& a, const Medium& b) {
    return !(a == b);
}

} // namespace libster
