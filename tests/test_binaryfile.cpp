#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "TestSupport.h"
#include "core/persistence/BinaryFile.h"
#include "core/persistence/BorrowerRecord.h"
#include "core/persistence/CStringUtil.h"
#include "core/persistence/FileHeader.h"
#include "core/persistence/FileType.h"
#include "core/persistence/IoError.h"

namespace {

using libster::BinaryFile;
using libster::BorrowerRecord;
using libster::BorrowerStatus;
using libster::FileType;
using libster::IoError;

class TempFile {
public: 
    explicit TempFile(std::string_view prefix) {
        static std::uint64_t counter = 0;

        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();

        path_ = std::filesystem::temp_directory_path() / (std::string(prefix) + "_" + std::to_string(stamp) + "_" + std::to_string(counter++) + ".bin");

        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }

    ~TempFile() {
        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;

    std::string path() const {
        return path_.string();
    }

    const std::filesystem::path& fsPath() const {
        return path_;
    }

private:
    std::filesystem::path path_;
};

BorrowerRecord makeRecord(std::uint32_t id, std::string_view lastName) {
    BorrowerRecord record{};
    record.id = id;
    libster::copyToFixed(record.lastName, lastName);
    libster::copyToFixed(record.firstName, "First");
    libster::copyToFixed(record.email, "first@example.com");
    libster::copyToFixed(record.address, "Example Street 1");
    libster::copyToFixed(record.recordedOn, "2026-11-04");
    libster::copyToFixed(record.recordedBy, "Tester");
    record.status = BorrowerStatus::ACTIVE;
    return record;
}

bool sameRecord(const BorrowerRecord& a, const BorrowerRecord& b) {
    return a.id == b.id
        && libster::fixedToString(a.lastName) == libster::fixedToString(b.lastName)
        && libster::fixedToString(a.firstName) == libster::fixedToString(b.firstName)
        && libster::fixedToString(a.email) == libster::fixedToString(b.email)
        && libster::fixedToString(a.address) == libster::fixedToString(b.address)
        && libster::fixedToString(a.recordedOn) == libster::fixedToString(b.recordedOn)
        && libster::fixedToString(a.recordedBy) == libster::fixedToString(b.recordedBy)
        && a.status == b.status;
}

// Tests

void testCreateInitializesHeader(libster::test::TestContext& t) {
    TempFile file("libster_create");
    BinaryFile bf;

    CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower) == IoError::None);
    CHECK(t, bf.isOpen());
    CHECK(t, bf.nextId() == 1u);
    CHECK(t, bf.header().magic == libster::kFileMagic);
    CHECK(t, bf.header().version == libster::kFileFormatVersion);
    CHECK(t, bf.header().type == FileType::Borrower);
    CHECK(t, bf.header().recordSize == static_cast<std::uint32_t>(sizeof(BorrowerRecord)));
    CHECK(t, bf.header().nextId == 1u);
}

void testAppendAssignsSequentialIds(libster::test::TestContext& t) {
    TempFile file("libster_append");
    BinaryFile bf;
    CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower) == IoError::None);

    std::uint32_t firstId = 0;
    std::uint32_t secondId = 0;

    CHECK(t, bf.append(makeRecord(0u, "Alpha"), firstId) == IoError::None);
    CHECK(t, firstId == 1u);
    CHECK(t, bf.nextId() == 2u);

    CHECK(t, bf.append(makeRecord(0u, "Beta"), secondId) == IoError::None);
    CHECK(t, secondId == 2u);
    CHECK(t, bf.nextId() == 3u);
    CHECK(t, bf.header().nextId == 3u);
}

void testReadReturnsWrittenRecords(libster::test::TestContext& t) {
    TempFile file("libster_read");
    BinaryFile bf;
    CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower) == IoError::None);

    std::uint32_t id = 0;
    CHECK(t, bf.append(makeRecord(0u, "Alpha"), id) == IoError::None);
    CHECK(t, bf.append(makeRecord(0u, "Beta"), id) == IoError::None);

    const BorrowerRecord expectedFirst = makeRecord(1u, "Alpha");
    const BorrowerRecord expectedSecond = makeRecord(2u, "Beta");

    BorrowerRecord out{};
    CHECK(t, bf.read(1u, out) == IoError::None);
    CHECK(t, sameRecord(out, expectedFirst));

    CHECK(t, bf.read(2u, out) == IoError::None);
    CHECK(t, sameRecord(out, expectedSecond));
}

void testReadRejectsInvalidIds(libster::test::TestContext& t) {
    TempFile file("libster_bad_id");
    BinaryFile bf;
    CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower) == IoError::None);

    std::uint32_t id = 0;
    CHECK(t, bf.append(makeRecord(0u, "Alpha"), id) == IoError::None);

    BorrowerRecord out = makeRecord(123u, "Sentinel");
    CHECK(t, bf.read(0u, out) == IoError::InvalidId);
    CHECK(t, bf.read(99u, out) == IoError::InvalidId);
    CHECK(t, out.id == 123u); // out bleibt unveraendert
}

void testReopenKeepsDataAndNextId(libster::test::TestContext& t) {
    TempFile file("libster_reopen");

    {
        BinaryFile bf;
        CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower) == IoError::None);

        std::uint32_t id = 0;
        CHECK(t, bf.append(makeRecord(0u, "Persist"), id) == IoError::None);
        CHECK(t, id == 1u);
    }

    BinaryFile bf;
    CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower) == IoError::None);
    CHECK(t, bf.nextId() == 2u);

    BorrowerRecord out{};
    CHECK(t, bf.read(1u, out) == IoError::None);
    CHECK(t, libster::fixedToString(out.lastName) == "Persist");

    std::uint32_t secondId = 0;
    CHECK(t, bf.append(makeRecord(0u, "AfterReopen"), secondId) == IoError::None);
    CHECK(t, secondId == 2u);
}

void testRejectsWrongFileType(libster::test::TestContext& t) {
    TempFile file("libster_wrong_type");

    {
        BinaryFile bf;
        CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower) == IoError::None);
    }

    BinaryFile bf;
    CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Book) == IoError::InvalidType);
    CHECK(t, !bf.isOpen());
}

void testRejectsWrongRecordSize(libster::test::TestContext& t) {
    TempFile file("libster_wrong_size");

    {
        BinaryFile bf;
        CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower) == IoError::None);
    }

    BinaryFile bf;
    CHECK(t, bf.openOrCreate<std::uint64_t>(file.path(), FileType::Borrower)
        == IoError::InvalidRecordSize);
}

void testRejectsInvalidMagic(libster::test::TestContext& t) {
    TempFile file("libster_bad_magic");

    {
        BinaryFile bf;
        CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower) == IoError::None);
    }

    {
        std::fstream patch(file.path(), std::ios::in | std::ios::out | std::ios::binary);
        CHECK(t, patch.is_open());

        const std::uint32_t badMagic = 0xDEADBEEFu;
        patch.write(reinterpret_cast<const char*>(&badMagic), sizeof(badMagic));
    }

    BinaryFile bf;
    CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower)
        == IoError::InvalidMagic);
}

void testRejectsTruncatedHeader(libster::test::TestContext& t) {
    TempFile file("libster_truncated");

    {
        std::ofstream out(file.path(), std::ios::binary | std::ios::trunc);
        out.write("abc", 3);
    }

    BinaryFile bf;
    CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower)
        == IoError::InvalidHeader);
}

void testInitializesEmptyExistingFile(libster::test::TestContext& t) {
    TempFile file("libster_empty");

    {
        std::ofstream out(file.path(), std::ios::binary | std::ios::trunc);
    }

    BinaryFile bf;
    CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower) == IoError::None);
    CHECK(t, bf.nextId() == 1u);
}

void testMissingParentDirectoryFails(libster::test::TestContext& t) {
    const TempFile base("libster_missing");

    const auto directory = base.fsPath().parent_path()
        / ("libster_missing_dir_"
            + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));

    std::error_code ec;
    std::filesystem::remove_all(directory, ec);

    BinaryFile bf;
    CHECK(t, bf.openOrCreate<BorrowerRecord>(
        (directory / "borrowers.bin").string(),
        FileType::Borrower) == IoError::CannotOpen);
    CHECK(t, !bf.isOpen());
}

void testOverwriteKeepsNextId(libster::test::TestContext& t) {
    TempFile file("libster_overwrite");
    BinaryFile bf;
    CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower) == IoError::None);

    std::uint32_t id = 0;
    CHECK(t, bf.append(makeRecord(0u, "Original"), id) == IoError::None);
    CHECK(t, id == 1u);

    CHECK(t, bf.write(1u, makeRecord(1u, "Changed")) == IoError::None);
    CHECK(t, bf.nextId() == 2u);

    BorrowerRecord out{};
    CHECK(t, bf.read(1u, out) == IoError::None);
    CHECK(t, libster::fixedToString(out.lastName) == "Changed");
}

void testWriteRejectsInvalidInput(libster::test::TestContext& t) {
    TempFile file("libster_write_checks");
    BinaryFile bf;
    CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower) == IoError::None);

    std::uint32_t id = 0;
    CHECK(t, bf.append(makeRecord(0u, "Alpha"), id) == IoError::None);

    CHECK(t, bf.write(1u, makeRecord(2u, "Mismatch")) == IoError::RecordMismatch);
    CHECK(t, bf.write(0u, makeRecord(0u, "NoSlot")) == IoError::InvalidId);
    CHECK(t, bf.write(99u, makeRecord(99u, "NoSlot")) == IoError::InvalidId);
}

void testFindAllReturnsMatchingRecords(libster::test::TestContext& t) {
    TempFile file("libster_find_all");
    BinaryFile bf;
    CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower) == IoError::None);

    std::uint32_t id = 0;
    CHECK(t, bf.append(makeRecord(0u, "Alpha"), id) == IoError::None);
    CHECK(t, bf.append(makeRecord(0u, "Beta"), id) == IoError::None);
    CHECK(t, bf.append(makeRecord(0u, "Alphabet"), id) == IoError::None);

    std::vector<BorrowerRecord> hits;
    const IoError error = bf.findAll<BorrowerRecord>(
        [](const BorrowerRecord& record) {
            return libster::fixedToString(record.lastName).find("Alpha") == 0;
        },
        hits
    );

    CHECK(t, error == IoError::None);
    CHECK(t, hits.size() == 2u);
    if (hits.size() == 2u) {
        CHECK(t, libster::fixedToString(hits[0].lastName) == "Alpha");
        CHECK(t, libster::fixedToString(hits[1].lastName) == "Alphabet");
    }
}

void testFindAllCanMatchById(libster::test::TestContext& t) {
    TempFile file("libster_find_by_id");
    BinaryFile bf;
    CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower) == IoError::None);

    std::uint32_t id = 0;
    CHECK(t, bf.append(makeRecord(0u, "Alpha"), id) == IoError::None);
    CHECK(t, bf.append(makeRecord(0u, "Beta"), id) == IoError::None);

    std::vector<BorrowerRecord> hits;
    CHECK(t, bf.findAll<BorrowerRecord>(
        [](const BorrowerRecord& record) {
            return record.id == 2u;
        },
        hits
    ) == IoError::None);

    CHECK(t, hits.size() == 1u);
    if (hits.size() == 1u) {
        CHECK(t, hits[0].id == 2u);
        CHECK(t, libster::fixedToString(hits[0].lastName) == "Beta");
    }
}

void testFindAllReturnsEmptyWhenNothingMatches(libster::test::TestContext& t) {
    TempFile file("libster_find_empty");
    BinaryFile bf;
    CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower) == IoError::None);

    std::uint32_t id = 0;
    CHECK(t, bf.append(makeRecord(0u, "Alpha"), id) == IoError::None);

    std::vector<BorrowerRecord> hits(3, makeRecord(99u, "Old"));
    CHECK(t, bf.findAll<BorrowerRecord>(
        [](const BorrowerRecord&) {
            return false;
        },
        hits
    ) == IoError::None);

    CHECK(t, hits.empty());
}

void testFindAllOnEmptyFile(libster::test::TestContext& t) {
    TempFile file("libster_find_empty_file");
    BinaryFile bf;
    CHECK(t, bf.openOrCreate<BorrowerRecord>(file.path(), FileType::Borrower) == IoError::None);

    std::vector<BorrowerRecord> hits;
    CHECK(t, bf.findAll<BorrowerRecord>(
        [](const BorrowerRecord&) {
            return true;
        },
        hits
    ) == IoError::None);

    CHECK(t, hits.empty());
}

void testOperationsOnClosedFileFail(libster::test::TestContext& t) {
    BinaryFile bf;
    BorrowerRecord record{};
    std::uint32_t id = 0;

    CHECK(t, bf.read(1u, record) == IoError::NotOpen);
    CHECK(t, bf.write(1u, record) == IoError::NotOpen);
    CHECK(t, bf.append(record, id) == IoError::NotOpen);

    std::vector<BorrowerRecord> hits;
    CHECK(t, bf.findAll<BorrowerRecord>([](const BorrowerRecord&) { return true; }, hits)
        == IoError::NotOpen);
}

} // namespace

void runBinaryFileTests(libster::test::TestContext& t) {
    testCreateInitializesHeader(t);
    testAppendAssignsSequentialIds(t);
    testReadReturnsWrittenRecords(t);
    testReadRejectsInvalidIds(t);
    testReopenKeepsDataAndNextId(t);
    testRejectsWrongFileType(t);
    testRejectsWrongRecordSize(t);
    testRejectsInvalidMagic(t);
    testRejectsTruncatedHeader(t);
    testInitializesEmptyExistingFile(t);
    testMissingParentDirectoryFails(t);
    testOverwriteKeepsNextId(t);
    testWriteRejectsInvalidInput(t);
    testFindAllReturnsMatchingRecords(t);
    testFindAllCanMatchById(t);
    testFindAllReturnsEmptyWhenNothingMatches(t);
    testFindAllOnEmptyFile(t);
    testOperationsOnClosedFileFail(t);
}
