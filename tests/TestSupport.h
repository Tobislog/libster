#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>

namespace libster::test {

class TempDir {
public:
    explicit TempDir(std::string_view prefix) {
        static std::uint64_t counter = 0;

        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();

        path_ = std::filesystem::temp_directory_path() / (std::string(prefix) + "_" + std::to_string(stamp) + "_" + std::to_string(counter++));

        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
        std::filesystem::create_directories(path_, ec);
    }

    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    std::string path() const {
        return path_.string();
    }

private:
    std::filesystem::path path_;
};

class TestContext {
public:
    void check(bool condition, std::string_view description) {
        if (condition) {
            std::cout << "[ OK ] " << description << '\n';
        } else {
            std::cout << "[ FAIL ] " << description << '\n';
            ++failures_;
        }
    }

    int failures() const { return failures_;}
    bool passed() const { return failures_ == 0;}

private:
    int failures_ = 0;   
};
} // namespace libster::test

#define CHECK(ctx, condition) (ctx).check((condition), #condition)
