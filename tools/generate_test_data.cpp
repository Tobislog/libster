#include <charconv>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "core/TimeUtil.h"
#include "core/model/Borrower.h"
#include "core/model/Book.h"
#include "core/model/CD.h"
#include "core/model/MediumType.h"
#include "core/persistence/IoError.h"
#include "core/services/LibraryService.h"

namespace {

using libster::Book;
using libster::Borrower;
using libster::CD;
using libster::IoError;
using libster::LibraryService;
using libster::MediumType;
using libster::ServiceError;
using libster::kUnassignedId;

struct Options {
    std::string dataDir;
    std::uint32_t borrowers = 5;
    std::uint32_t books = 10;
    std::uint32_t cds = 10;
    std::uint32_t loans = 8;
    bool force = false;
};

void printUsage() {
    std::cout
        << "Aufruf: libster_generate <datenverzeichnis> [Optionen]\n"
        << "  --borrowers N   Anzahl Ausleiher (Standard 5)\n"
        << "  --books N       Anzahl Buecher (Standard 10)\n"
        << "  --cds N         Anzahl CDs (Standard 10)\n"
        << "  --loans N       Anzahl Ausleihen (Standard 8)\n"
        << "  --force         vorhandene .bin-Dateien ersetzen\n"
        << "  --help          diese Hilfe anzeigen\n";
}

bool parseCount(const char* text, std::uint32_t& value) {
    if (text == nullptr) return false;

    const std::string_view view(text);
    if(view.empty()) return false;

    std::uint32_t parsed = 0;
    const auto result = std::from_chars(view.data(), view.data() + view.size(), parsed);

    if (result.ec != std::errc{} || result.ptr != view.data() + view.size()) {
        return false;
    }
    
    value = parsed;
    return true;
}

std::string padded(std::uint32_t value, std::size_t width) {
    std::string text = std::to_string(value);
    if(text.size() < width) {
        text.insert(0, width - text.size(), '0');
    }
    return text;
}

constexpr const char* kTopics[] = { "Algorithmen", "Datenbanken", "Netzwerke","Mathematik", "Physik", "Geschichte" };
constexpr std::size_t kTopicCount = sizeof(kTopics) / sizeof(kTopics[0]);
constexpr const char* kPublishers[] = { "O'Reilly", "Hanser", "Springer", "dpunkt", "Pearson" };
constexpr std::size_t kPublisherCount = sizeof(kPublishers) / sizeof(kPublishers[0]);
constexpr const char* kGenres[] = { "Sachbuch", "Roman", "Krimi", "Biografie", "Science-Fiction" };
constexpr std::size_t kGenreCount = sizeof(kGenres) / sizeof(kGenres[0]);
constexpr const char* kAuthors[] = {
    "Anna Beispiel",
    "Bernd Muster;Clara Test",
    "Dieter Autor",
    "Eva Schreiber;Frank Dichter"
};
constexpr std::size_t kAuthorCount = sizeof(kAuthors) / sizeof(kAuthors[0]);

Borrower makeBorrower(std::uint32_t index) {
    const std::string number = padded(index, 4);

    return Borrower{
        kUnassignedId,
        "Nachname" + number,
        "Vorname" + number,
        "leser" + number + "@example.org",
        "Musterweg " + std::to_string(index),
        "2026-11-08",
        "Generator"
    };
}

Book makeBook(std::uint32_t index) {
    const std::string number = padded(index, 4);

    return Book{
        kUnassignedId,
        "Buch " + number + " - " + kTopics[index % kTopicCount],
        kPublishers[index % kPublisherCount],
        kGenres[index % kGenreCount],
        "2000-01-01",
        "2026-11-08",
        "Regal B" + padded(index, 4),
        static_cast<std::uint32_t>(1000 + (index % 50) * 100),
        "Generator",
        "ISBN-" + padded(index, 6),
        static_cast<std::uint16_t>(1 + index % 5),
        kAuthors[index % kAuthorCount],
        static_cast<std::uint16_t>(100 + index % 900)
    };
}

CD makeCD(std::uint32_t index) {
    const std::string number = padded(index, 6);

    return CD{
        kUnassignedId,
        "CD " + number + " - " + kTopics[index % kTopicCount],
        kPublishers[index % kPublisherCount],
        kGenres[index % kGenreCount],
        "2005-03-15",
        "2026-11-08",
        "Regal C" + padded(index, 4),
        static_cast<std::uint32_t>(800 + (index % 40) * 50),
        "Generator",
        kAuthors[index % kAuthorCount],
        static_cast<std::uint16_t>(40 + index % 60)
    };
}

} // namespace

int main(int argc, char** argv) {
    Options options;

    for (int i = 1; i < argc; ++i) {
        const std::string_view arg(argv[i]);

        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        }
        if (arg == "--force") {
            options.force = true;
            continue;
        }

        auto readCount = [&](std::uint32_t& target) {
            if (i + 1 >= argc) {
                std::cerr << "Fehlender Wert fuer " << arg << '\n';
                return false;
            }

            const char* value = argv[++i];
            if (!parseCount(value, target)) {
                std::cerr << "Ungueltiger Wert fuer " << arg << ": " << value << '\n';
                return false;
            }
            return true;
        };

        if (arg == "--borrowers") {
            if (!readCount(options.borrowers)) return 1;
        } else if (arg == "--books") {
            if (!readCount(options.books)) return 1;
        } else if (arg == "--cds") {
            if (!readCount(options.cds)) return 1;
        } else if (arg == "--loans") {
            if (!readCount(options.loans)) return 1;
        } else if (!arg.empty() && arg.front() == '-') {
            std::cerr << "Unbekannte Option: " << arg << '\n';
            printUsage();
            return 1;
        } else {
            if (!options.dataDir.empty()) {
                std::cerr << "Bitte nur ein Datenverzeichnis angeben.\n";
                return 1;
            }
            options.dataDir = std::string(arg);
        }
    }

    if (options.dataDir.empty()) {
        printUsage();
        return 1;
    }

    if (options.loans > 0 && options.borrowers == 0) {
        std::cerr << "Ausleihen benoetigen mindestens einen Borrower.\n";
        return 1;
    }

    const std::uint64_t avaiableMedia = static_cast<std::uint64_t>(options.books) + options.cds;

    if (options.loans > 0 && avaiableMedia == 0) {
        std::cerr << "Ausleihen benoetigen mindestens ein Medium.\n";
        return 1;
    }

    if (options.loans > avaiableMedia) {
        std::cout << "Hinweis:; hoechstens " << avaiableMedia << " Ausleihen moeglich, reduziere von " << options.loans << ".\n";
        options.loans = static_cast<std::uint32_t>(avaiableMedia);
    }

    const char* const kFileNames[] = {
        "borowers.bin",
        "books.bin",
        "cds.bin",
        "loan_transactions.bin"
    };

    std::error_code ec;
    std::filesystem::create_directories(options.dataDir, ec);
    if (ec) {
        std::cerr << "Verzeichnis konnte nicht angelet werden: " << options.dataDir << '\n';
        return 1;
    }

    bool hasExistingFiles = false;
    for (const char* name : kFileNames) {
        const auto path = std::filesystem::path(options.dataDir) / name;
        if (std::filesystem::exists(path, ec)) {
            hasExistingFiles = true;
        }
    }

    if (hasExistingFiles && !options.force) {
        std::cerr << "Im Datenverzeichnis liegen bereits .bin-Dateien. Nutze --force, um sie zu ersetzen.\n";
    }

    if (options.force) {
        for (const char* name : kFileNames) {
            const auto path = std::filesystem::path(options.dataDir) / name;
            std::filesystem::remove(path, ec);
        }
    }

    LibraryService service;
    service.setRecordedBy("Generator");

    const IoError openError = service.open(options.dataDir);
    if (openError != IoError::None) {
        std::cerr << "Oeffnen fehlgeschlagen: " << libster::toString(openError) << '\n';
        return 1;
    }

    // 1. Borrower
    std::vector<std::uint32_t> borrowerIds;
    borrowerIds.reserve(options.borrowers);

    for (std::uint32_t i = 1; i <= options.borrowers; ++i) {
        Borrower borrower = makeBorrower(i);
        const ServiceError error = service.addBorrower(borrower);
        if (error != ServiceError::None) {
            std::cerr << "Borrower " << i << ": " << libster::toString(error) << '\n';
            return 1;
        }
        borrowerIds.push_back(borrower.getId());
    }

    // 2. Buecher
    std::vector<std::uint32_t> bookIds;
    bookIds.reserve(options.books);

    for (std::uint32_t i = 1; i <= options.books; ++i) {
        Book book = makeBook(i);
        const ServiceError error = service.addBook(book);
        if (error != ServiceError::None) {
            std::cerr << "Buch " << i << ": "
                      << libster::toString(error) << '\n';
            return 1;
        }
        bookIds.push_back(book.getId());
    }

    // 3. CDs
    std::vector<std::uint32_t> cdIds;
    cdIds.reserve(options.cds);

    for (std::uint32_t i = 1; i <= options.cds; ++i) {
        CD cd = makeCD(i);
        const ServiceError error = service.addCD(cd);
        if (error != ServiceError::None) {
            std::cerr << "CD " << i << ": "
                      << libster::toString(error) << '\n';
            return 1;
        }
        cdIds.push_back(cd.getId());
    }

    // 4. Ausleihen: gerade Indizes bleiben offen, ungerade werden nach sieben Tagen zurückgegeben.
    std::int64_t dayStart = 0;
    std::int64_t dayEnd = 0;
    if (!libster::isoDateToDayRange("2026-11-08", dayStart, dayEnd)) {
        std::cerr << "Interner Fehler: Tagesbereich nicht berechenbar.\n";
        return 1;
    }
    (void)dayEnd;

    const std::int64_t baseLoanDate = dayStart + 10 * 3600; // 10:00 Uhr

    std::uint32_t openLoans = 0;
    std::uint32_t returnedLoans = 0;

    for (std::uint32_t i = 0; i < options.loans; ++i) {
        MediumType type = MediumType::BOOK;
        std::uint32_t mediumId = 0;

        if (i < options.books) {
            mediumId = bookIds[i];
        } else {
            type = MediumType::CD;
            mediumId = cdIds[i - options.books];
        }

        const std::uint32_t borrowerId = borrowerIds[i % borrowerIds.size()];
        const std::int64_t loanDate = baseLoanDate + static_cast<std::int64_t>(i) * 3600;

        std::uint32_t loanId = 0;
        ServiceError error = ServiceError::None;

        if (type == MediumType::BOOK) {
            error = service.loanBook(borrowerId, mediumId, loanDate, loanId);
        } else {
            error = service.loanCD(borrowerId, mediumId, loanDate, loanId);
        }

        if (error != ServiceError::None) {
            std::cerr << "Ausleihe " << (i + 1) << ": " << libster::toString(error) <<  '\n';
            return 1;
        }

        if (i % 2u == 1u) {
            const std::uint64_t returnDate = loanDate + 7 * 86400;
            error = service.returnLoan(loanId, returnDate);
            if (error != ServiceError::None) {
                std::cerr << "Rueckgabe " << (i + 1) << ": " << libster::toString(error) << '\n';
                return 1;
            }
            ++returnedLoans;
        } else {
            ++openLoans;
        }
    }

    std::cout << "Testdaten erzeugt in: " << options.dataDir << '\n'
        << "  Borrower:  " << options.borrowers << '\n'
        << "  Buecher:   " << options.books << '\n'
        << "  CDs:       " << options.cds << '\n'
        << "  Ausleihen: " << options.loans
        << " (" << openLoans << " offen, "
        << returnedLoans << " zurueckgegeben)\n";

    return 0;
}
