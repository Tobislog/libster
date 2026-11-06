#include "core/persistence/BinaryFile.h"

#include <array>
#include <cstring>
#include <filesystem>
#include <system_error>

namespace libster {

// 1. Lebenszyklus
BinaryFile::~BinaryFile() {
    close();
}

void BinaryFile::close() noexcept {
    if (stream_.is_open()) {
        stream_.flush();
        stream_.close();
    }
    stream_.clear();
    path_.clear();
    header_ = FileHeader{};
    recordSize_ = 0;
}

bool BinaryFile::isOpen() const noexcept {
    return stream_.is_open();
}

// 2. Zustand
std::uint32_t BinaryFile::nextId() const noexcept {
    return header_.nextId;
}

const FileHeader& BinaryFile::header() const noexcept {
    return header_;
}

// 3. Reine Berechnungen
std::uint64_t BinaryFile::recordOffset(std::uint32_t id) const noexcept {
    return static_cast<std::uint64_t>(sizeof(FileHeader))
        + static_cast<std::uint64_t>(id - 1) * static_cast<std::uint64_t>(recordSize_);
}

bool BinaryFile::isValidId(std::uint32_t id) const noexcept {
    return id >= kFirstId && id < header_.nextId;
}

// 4. Header-I/O
IoError BinaryFile::loadHeader() {
    if (!stream_.is_open()) return IoError::NotOpen;
    stream_.clear();
    stream_.seekg(0, std::ios::beg);

    if (!stream_) return IoError::CannotRead;

    std::array<char, sizeof(FileHeader)> buffer{};
    stream_.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    if (stream_.gcount() != static_cast<std::streamsize>(buffer.size())) return IoError::InvalidHeader;
    std::memcpy(&header_, buffer.data(), sizeof(FileHeader));

    return IoError::None;
}

IoError BinaryFile::persistHeader(const FileHeader& header) {
    if (!stream_.is_open()) return IoError::NotOpen;
    stream_.clear();
    stream_.seekp(0, std::ios::beg);
    if (!stream_) return IoError::CannotWrite;

    std::array<char, sizeof(FileHeader)> buffer{};
    std::memcpy(buffer.data(), &header, sizeof(FileHeader));

    stream_.write(buffer.data(), static_cast<std::streamsize>(sizeof(FileHeader)));
    stream_.flush();
    if (!stream_.good()) return IoError::CannotWrite;

    return IoError::None;
}

// 5. Record-I/O auf Byte-Ebene
IoError BinaryFile::readRecordBytes(std::uint64_t offset, void* dst, std::size_t size) {
    if (!stream_.is_open()) return IoError::NotOpen;
    stream_.clear();
    stream_.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if (!stream_) return IoError::CannotRead;

    stream_.read(static_cast<char*>(dst), static_cast<std::streamsize>(size));
    if (stream_.gcount() != static_cast<std::streamsize>(size)) return IoError::CannotRead;

    return IoError::None;

}

IoError BinaryFile::writeRecordBytes(std::uint64_t offset, const void* src, std::size_t size) {
    if (!stream_.is_open()) return IoError::NotOpen;
    stream_.clear();
    stream_.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
    if (!stream_) return IoError::CannotWrite;

    stream_.write(static_cast<const char*>(src), static_cast<std::streamsize>(size));
    stream_.flush();
    if (!stream_.good()) return IoError::CannotWrite;
    
    return IoError::None;
}

// 6. Öffnen / Initialisieren
IoError BinaryFile::openOrCreateImpl(const std::string& path, FileType expectedType, std::uint32_t recordSize) {
    close();
    if (recordSize == 0) return IoError::InvalidRecordSize;

    std::error_code ec;
    const bool exists = std::filesystem::exists(path, ec);
    if (ec) return IoError::CannotOpen;
    
    bool needsHeader = !exists;

    if (exists) {
        const bool regular = std::filesystem::is_regular_file(path, ec);
        if (ec || !regular) return IoError::CannotOpen;
        const auto size = std::filesystem::file_size(path, ec);
        if (ec) return IoError::CannotOpen;
        needsHeader = (size == 0);
    }

    const auto mode = std::ios::in | std::ios::out | std::ios::binary;
    stream_.open(path, needsHeader ? (mode | std::ios::trunc) : mode);
    if (!stream_.is_open()) {
        stream_.clear();
        return IoError::CannotOpen;
    }

    path_ = path;
    recordSize_ = recordSize;

    if (needsHeader) {
        const FileHeader header = makeHeader(expectedType, recordSize);
        const IoError error = persistHeader(header);
        if (error != IoError::None) {
            close();
            return error;
        }
        header_ = header;
        return IoError::None;
    }

    IoError error = loadHeader();
    if (error != IoError::None) {
        close();
        return error;
    }

    if (header_.magic != kFileMagic) {
        close();
        return IoError::InvalidMagic;
    }
    if (header_.version != kFileFormatVersion) {
        close();
        return IoError::InvalidVersion;
    }
    if (header_.type != expectedType) {
        close();
        return IoError::InvalidType;
    }
    if (header_.recordSize != recordSize) {
        close();
        return IoError::InvalidRecordSize;
    }
    if (header_.nextId < kFirstId) {
        close();
        return IoError::InvalidHeader;
    }
    return IoError::None;
}

} // namespace libster

