#pragma once

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <type_traits>

#include "core/persistence/FileHeader.h"
#include "core/persistence/FileType.h"
#include "core/persistence/IoError.h"

namespace libster {

class BinaryFile {
public:
    BinaryFile() = default;
    ~BinaryFile();

    BinaryFile(const BinaryFile&) = delete;
    BinaryFile& operator=(const BinaryFile&) = delete;

    template <typename T>
    IoError openOrCreate(const std::string& path, FileType expectedType) {
        static_assert(std::is_trivially_copyable_v<T>, "Records müssen trivially copyable sein!");
        static_assert(std::is_standard_layout_v<T>, "Records müssen standard layout sein");
        return openOrCreateImpl(path, expectedType, static_cast<std::uint32_t>(sizeof(T)));
    }

    void close() noexcept;
    bool isOpen() const noexcept;
    std::uint32_t nextId() const noexcept;
    const FileHeader& header() const noexcept;

    template <typename T>
    IoError append(const T& record, std::uint32_t& assignedId);

    template <typename T>
    IoError read(std::uint32_t id, T& out);

    template <typename T>
    IoError write(std::uint32_t id, const T& record);

private:
    IoError openOrCreateImpl(const std::string& path, FileType expectedType, std::uint32_t recordSize);

    IoError loadHeader();
    IoError persistHeader(const FileHeader& header);
    bool isValidId(std::uint32_t id) const noexcept;
    std::uint64_t recordOffset(std::uint32_t id) const noexcept;
    IoError readRecordBytes(std::uint64_t offset, void* dst, std::size_t size);
    IoError writeRecordBytes(std::uint64_t offset, const void* src, std::size_t size);
    std::fstream stream_;
    std::string path_;
    FileHeader header_{};
    std::uint32_t recordSize_ = 0;
};

template <typename T>
IoError BinaryFile::append(const T& record, std::uint32_t& assignedId) {
    static_assert(std::is_trivially_copyable_v<T>, "Records müssen trivially copyable sein");

    if (!isOpen()) return IoError::NotOpen;
    if (recordSize_ != static_cast<std::uint32_t>(sizeof(T))) return IoError::InvalidRecordSize;
    if (header_.nextId < kFirstId) return IoError::InvalidHeader;
    const std::uint32_t id = header_.nextId;

    T stored = record;
    stored.id = id;

    // Reihenfolge bewusst so: erst Record, dann Header.
    IoError error = writeRecordBytes(recordOffset(id), &stored, sizeof(T));
    if (error != IoError::None) return error;
    FileHeader updated = header_;
    updated.nextId = id + 1;
    error = persistHeader(updated);
    if (error != IoError::None) return error;
    header_ = updated;
    assignedId = id;
    return IoError::None;
}

template <typename T>
IoError BinaryFile::read(std::uint32_t id, T& out) {
    static_assert(std::is_trivially_copyable_v<T>, "Records müssen trivially copyable sein");
    static_assert(std::is_default_constructible_v<T>, "Records müssen default-konstruierbar sein.");

    if (!isOpen()) return IoError::NotOpen;
    if (recordSize_ != static_cast<std::uint32_t>(sizeof(T))) return IoError::InvalidRecordSize;
    if (!isValidId(id)) return IoError::InvalidId;
    T record {};
    const IoError error = readRecordBytes(recordOffset(id), &record, sizeof(T));
    if (error != IoError::None) return error;
    if (record.id != id) return IoError::RecordMismatch;

    out = record;
    return IoError::None;
}

template <typename T>
IoError BinaryFile::write(std::uint32_t id, const T& record) {
    static_assert(std::is_trivially_copyable_v<T>, "Records muessen trivially copyable sein");
    
    if (!isOpen()) return IoError::NotOpen;
    if (recordSize_ != static_cast<std::uint32_t>(sizeof(T))) return IoError::InvalidRecordSize;
    if (!isValidId(id)) return IoError::InvalidId;
    if (record.id != id) return IoError::RecordMismatch;

    return writeRecordBytes(recordOffset(id), &record, sizeof(T));
}
} // namespace libster
