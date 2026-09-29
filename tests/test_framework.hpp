#pragma once
// Minimal self-contained test framework shared by all test translation units.

#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

struct TestSuite {
    std::string suiteName;
    std::vector<TestCase> cases;
    int passCount = 0;
    int failCount = 0;

    void run() {
        std::cout << "\n=== " << suiteName << " ===\n";
        for (auto& test : cases) {
            try {
                test.fn();
                passCount++;
                std::cout << "  [PASS] " << test.name << "\n";
            } catch (const std::exception& e) {
                failCount++;
                std::cout << "  [FAIL] " << test.name << " - " << e.what() << "\n";
            } catch (...) {
                failCount++;
                std::cout << "  [FAIL] " << test.name << " - Unknown exception\n";
            }
        }
    }
};

TestSuite* createSuite(const std::string& name);
void addTest(TestSuite* suite, const std::string& name, std::function<void()> fn);

/// Throw with file/line context when @p cond is false.
#define EXPECT_TRUE(cond)                                                          \
    do {                                                                           \
        if (!(cond)) {                                                             \
            std::ostringstream os_;                                                \
            os_ << __FILE__ << ":" << __LINE__ << ": expected " #cond;              \
            throw std::runtime_error(os_.str());                                   \
        }                                                                          \
    } while (0)

/// Throw with both values when @p a != @p b.
#define EXPECT_EQ(a, b)                                                            \
    do {                                                                           \
        const auto va_ = (a);                                                      \
        const auto vb_ = (b);                                                      \
        if (!(va_ == vb_)) {                                                       \
            std::ostringstream os_;                                                \
            os_ << __FILE__ << ":" << __LINE__ << ": " #a " == " #b " failed ("   \
                << +va_ << " vs " << +vb_ << ")";                                  \
            throw std::runtime_error(os_.str());                                   \
        }                                                                          \
    } while (0)

// Suites defined in other translation units.
void registerCoreTests();
void registerProfileStoreTests();
