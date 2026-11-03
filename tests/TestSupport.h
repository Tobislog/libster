#pragma once

#include <iostream>
#include <string_view>

namespace libster::test {

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
    bool passed() const { return failures_ == 0; }

private:
    int failures_ = 0;    
};
} // namespace libster::test

#define CHECK(ctx, condition) (ctx).check((condition), #condition)
