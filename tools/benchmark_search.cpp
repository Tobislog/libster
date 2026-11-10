#include <charconv>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "core/model/Book.h"
#include "core/persistence/IoError.h"
#include "core/services/LibraryService.h"

namespace {

using libster::Book;
using libster::IoError;
using libster::LibraryService;
using libster::ServiceError;

using Clock = std::chrono::steady_clock;

struct Options {
    std::string dataDir;
    std::string query;
    std::string logPath = "performance.log";
    std::uint32_t runs  = 3;
    std::uint32_t thresholdMs = 150;
    std::uint32_t minRecords = 55000;
};

void printUsage() {
    std::cout
        << "Aufruf: libster_benchmark <datenverzeichnis> [Optionen]\n"
        << "  --query TEXT      Suchbegriff (Standard: leer = alle Buecher)\n"
        << "  --runs N          Anzahl Messlaeufe (Standard 3)\n"
        << "  --threshold-ms N  Zeitgrenze in ms (Standard 150)\n"
        << "  --min-records N   Mindestanzahl Treffer (Standard 55000)\n"
        << "  --log PATH        Logdatei (Standard performance.log)\n"
        << "  --help            diese Hilfe anzeigen\n";
}

bool parseCount(const char* text, std::uint32_t& value) {
    if (text == nullptr) return false;

    const std::string_view view(text);
    if (view.empty()) return false;

    std::uint32_t parsed = 0;
    const auto result =
        std::from_chars(view.data(), view.data() + view.size(), parsed);

    if (result.ec != std::errc{} || result.ptr != view.data() + view.size()) {
        return false;
    }

    value = parsed;
    return true;
}

std::string formatMilliseconds(double value) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}

std::string localTimestamp() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};

#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif

    char buffer[32] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &local);
    return std::string(buffer);
}

struct RunResult {
    ServiceError error = ServiceError::None;
    std::size_t records = 0;
    double milliseconds = 0.0;
};

RunResult runSearch(LibraryService& service, const std::string& query) {
    std::vector<Book> books;

    const auto start = Clock::now();
    const ServiceError error = service.findBooks(query, books);
    const auto end = Clock::now();

    const std::chrono::duration<double, std::milli> elapsed = end - start;

    return RunResult{error, books.size(), elapsed.count()};
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

        auto readCount = [&](std::uint32_t& target) {
            if (i + 1 >= argc) {
                std::cerr << "Fehlender Wert fuer " << arg << '\n';
                return false;
            }

            const char* value = argv[++i];
            if (!parseCount(value, target)) {
                std::cerr << "Ungueltiger Wert fuer " << arg
                          << ": " << value << '\n';
                return false;
            }
            return true;
        };

        if (arg == "--runs") {
            if (!readCount(options.runs)) return 1;
        } else if (arg == "--threshold-ms") {
            if (!readCount(options.thresholdMs)) return 1;
        } else if (arg == "--min-records") {
            if (!readCount(options.minRecords)) return 1;
        } else if (arg == "--log") {
            if (i + 1 >= argc) {
                std::cerr << "Fehlender Wert fuer --log\n";
                return 1;
            }
            options.logPath = argv[++i];
        } else if (arg == "--query") {
            if (i + 1 >= argc) {
                std::cerr << "Fehlender Wert fuer --query\n";
                return 1;
            }
            options.query = argv[++i];
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

    if (options.runs == 0) {
        std::cerr << "--runs muss mindestens 1 sein.\n";
        return 1;
    }

    LibraryService service;
    service.setRecordedBy("Benchmark");

    const IoError openError = service.open(options.dataDir);
    if (openError != IoError::None) {
        std::cerr << "Oeffnen fehlgeschlagen: "
                  << libster::toString(openError) << '\n';
        return 1;
    }

    // Ein Warmlauf, damit die Datei im Page-Cache liegt und
    // Allokationen bereits stattgefunden haben.
    const RunResult warmup = runSearch(service, options.query);
    if (warmup.error != ServiceError::None) {
        std::cerr << "Suche fehlgeschlagen: "
                  << libster::toString(warmup.error) << '\n';
        return 1;
    }

    std::cout << "Datenverzeichnis: " << options.dataDir << '\n'
              << "Query: \"" << options.query << "\"\n"
              << "Warmup: " << warmup.records << " Treffer in "
              << formatMilliseconds(warmup.milliseconds) << " ms\n\n";

    std::ofstream log(options.logPath, std::ios::app);
    if (!log) {
        std::cerr << "Logdatei konnte nicht geoeffnet werden: "
                  << options.logPath << '\n';
        return 1;
    }

    bool allPassed = true;
    bool hasBest = false;
    double best = 0.0;

    for (std::uint32_t run = 1; run <= options.runs; ++run) {
        const RunResult result = runSearch(service, options.query);

        const bool pass =
            result.error == ServiceError::None &&
            result.records >= options.minRecords &&
            result.milliseconds <= static_cast<double>(options.thresholdMs);

        if (!pass) allPassed = false;
        if (!hasBest || result.milliseconds < best) {
            best = result.milliseconds;
            hasBest = true;
        }

        std::cout << "Lauf " << run << ": "
                  << result.records << " Treffer, "
                  << formatMilliseconds(result.milliseconds) << " ms -> "
                  << (pass ? "PASS" : "FAIL") << '\n';

        if (result.error != ServiceError::None) {
            std::cerr << "  Fehler: "
                      << libster::toString(result.error) << '\n';
        }

        log << '[' << localTimestamp() << "] "
            << "data=" << options.dataDir
            << " query=\"" << options.query << '"'
            << " records=" << result.records
            << " duration_ms=" << formatMilliseconds(result.milliseconds)
            << " threshold_ms=" << options.thresholdMs
            << " result=" << (pass ? "PASS" : "FAIL") << '\n';
        log.flush();
    }

    std::cout << "\nBeste Dauer: " << formatMilliseconds(best)
              << " ms (Grenze " << options.thresholdMs << " ms)\n"
              << "Log: " << options.logPath << '\n';

    return allPassed ? 0 : 1;
}
