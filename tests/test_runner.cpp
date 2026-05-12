#include <iostream>
#include <vector>
#include <string>
#include <functional>

// Forward declarations for test registration functions
void registerProtocolParseTests();
void registerHandshakeTests();
void registerDeltaFrameTests();
void registerRS485FrameTests();

// Simple test framework (replacement for GoogleTest for initial implementation)
class TestSuite;
extern TestSuite* createSuite(const std::string& name);
extern void addTest(TestSuite* suite, const std::string& name, std::function<void()> fn);

class TestCase {
public:
    std::string name;
    std::function<void()> fn;
    bool passed = false;
    std::string error;
};

class TestSuite {
public:
    std::string suiteName;
    std::vector<TestCase> cases;
    int passCount = 0;
    int failCount = 0;
    
    void run() {
        std::cout << "\n=== " << suiteName << " ===\n";
        for (auto& test : cases) {
            try {
                test.fn();
                test.passed = true;
                passCount++;
                std::cout << "  [PASS] " << test.name << "\n";
            } catch (const std::exception& e) {
                test.passed = false;
                test.error = e.what();
                failCount++;
                std::cout << "  [FAIL] " << test.name << " - " << test.error << "\n";
            } catch (...) {
                test.passed = false;
                test.error = "Unknown exception";
                failCount++;
                std::cout << "  [FAIL] " << test.name << " - " << test.error << "\n";
            }
        }
    }
};

// Global test suites
std::vector<TestSuite*> g_suites;

TestSuite* createSuite(const std::string& name) {
    auto suite = new TestSuite{name, {}, 0, 0};
    g_suites.push_back(suite);
    return suite;
}

void addTest(TestSuite* suite, const std::string& name, std::function<void()> fn) {
    suite->cases.push_back({name, fn, false, ""});
}

int main() {
    std::cout << "\n========================================\n";
    std::cout << "  Hornet Link Test Suite\n";
    std::cout << "========================================\n";
    
    // Register all tests
    registerProtocolParseTests();
    registerHandshakeTests();
    registerDeltaFrameTests();
    registerRS485FrameTests();
    
    // Run all suites
    int totalPass = 0, totalFail = 0;
    for (auto suite : g_suites) {
        suite->run();
        totalPass += suite->passCount;
        totalFail += suite->failCount;
    }
    
    // Summary
    std::cout << "\n========================================\n";
    std::cout << "SUMMARY\n";
    std::cout << "========================================\n";
    std::cout << "Passed: " << totalPass << "\n";
    std::cout << "Failed: " << totalFail << "\n";
    std::cout << "Total:  " << (totalPass + totalFail) << "\n\n";
    
    return totalFail > 0 ? 1 : 0;
}
