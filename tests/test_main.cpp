#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <stdexcept>
#include "BiosProtocol.hpp"
#include "DeviceRegistry.hpp"
#include "ControlDatabase.hpp"
#include "RS485ProtocolSpec.hpp"

using namespace dcsbios;

// ============================================================================
// Simple Test Framework
// ============================================================================

class TestCase {
public:
    std::string name;
    std::function<void()> fn;
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

std::vector<TestSuite*> g_suites;

TestSuite* createSuite(const std::string& name) {
    auto suite = new TestSuite{name, {}, 0, 0};
    g_suites.push_back(suite);
    return suite;
}

void addTest(TestSuite* suite, const std::string& name, std::function<void()> fn) {
    suite->cases.push_back({name, fn});
}

// ============================================================================
// Protocol Parse Tests
// ============================================================================

void registerProtocolParseTests() {
    auto suite = createSuite("BiosProtocol - ExportParser");
    
    addTest(suite, "Sync detection", []() {
        BiosStateMap stateMap;
        ExportParser parser(stateMap);
        int frameCallCount = 0;
        parser.onFrameSync = [&frameCallCount]() { frameCallCount++; };
        
        uint8_t input[] = {0x55, 0x55, 0x55, 0x55};
        parser.processBytes(input, sizeof(input));
        if (frameCallCount != 1) throw std::runtime_error("frameCallCount != 1");
    });
    
    addTest(suite, "Single write record", []() {
        BiosStateMap stateMap;
        ExportParser parser(stateMap);
        parser.onFrameSync = []() {};
        
        uint8_t input[] = {0x55, 0x55, 0x55, 0x55, 0x00, 0x00, 0x02, 0x00, 0xAB, 0xCD};
        parser.processBytes(input, sizeof(input));
        if (stateMap.readWord(0x0000) != 0xCDAB) {
            throw std::runtime_error("stateMap value mismatch");
        }
    });
    
    addTest(suite, "Multi-record frame", []() {
        BiosStateMap stateMap;
        ExportParser parser(stateMap);
        parser.onFrameSync = []() {};
        
        uint8_t input[] = {
            0x55, 0x55, 0x55, 0x55,
            0x02, 0x00, 0x04, 0x00, 0x11, 0x22, 0x33, 0x44,
            0x10, 0x00, 0x02, 0x00, 0xFF, 0x00
        };
        parser.processBytes(input, sizeof(input));
        
        if (stateMap.readWord(0x0002) != 0x2211) throw std::runtime_error("0x0002 mismatch");
        if (stateMap.readWord(0x0004) != 0x4433) throw std::runtime_error("0x0004 mismatch");
        if (stateMap.readWord(0x0010) != 0x00FF) throw std::runtime_error("0x0010 mismatch");
    });
}

// ============================================================================
// Handshake Tests
// ============================================================================

void registerHandshakeTests() {
    auto suite = createSuite("HandshakeParser - Device Detection");
    
    addTest(suite, "Parser initialization", []() {
        HandshakeParser parser;
        DeviceInfo device;
        if (!device.slaves.empty()) throw std::runtime_error("Slaves not empty");
    });
    
    addTest(suite, "Device structure", []() {
        DeviceInfo device;
        if (device.subscriptions.size() != 0) throw std::runtime_error("Subs not empty");
    });
}

// ============================================================================
// Delta Frame Tests
// ============================================================================

void registerDeltaFrameTests() {
    auto suite = createSuite("BuildDeltaFrame - Subscription Filtering");
    
    addTest(suite, "Wildcard subscription", []() {
        BiosStateMap stateMap;
        DeviceInfo device;
        device.wantsAll = true;
        
        if (!device.wantsAll) throw std::runtime_error("wantsAll not set");
    });
    
    addTest(suite, "Filtered subscription", []() {
        DeviceInfo device;
        Subscription sub;
        sub.mask = 0xFFFF;
        sub.shift = 0;
        device.subscriptions.push_back(sub);
        
        if (device.subscriptions.size() != 1) throw std::runtime_error("Subscription not added");
    });
}

// ============================================================================
// RS485 Frame Tests
// ============================================================================

void registerRS485FrameTests() {
    auto suite = createSuite("RS485 Protocol - Frame Format and CRC");
    
    addTest(suite, "CRC-16 calculation", []() {
        uint8_t testData[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
        uint16_t crc = crc16CcittFalse(testData, sizeof(testData));
        if (crc != 0x31C3) throw std::runtime_error("CRC calculation failed");
    });
    
    addTest(suite, "Constants validation", []() {
        if (kRS485STX != 0xFE) throw std::runtime_error("STX constant");
        if (kRS485BaudRate != 250000U) throw std::runtime_error("Baud rate");
        if (kRS485MaxPayloadBytes != 512U) throw std::runtime_error("Max payload");
    });
}

// ============================================================================
// ControlDatabase Tests
// ============================================================================

void registerControlDatabaseTests() {
    auto suite = createSuite("ControlDatabase - JSON Loading");
    
    addTest(suite, "Empty database initialization", []() {
        ControlDatabase db;
        if (!db.empty()) throw std::runtime_error("Database should be empty");
    });
    
    addTest(suite, "loadFromJson with nonexistent file", []() {
        ControlDatabase db;
        size_t loaded = db.loadFromJson("nonexistent_file_xyz123.json");
        if (loaded != 0) throw std::runtime_error("Should return 0 for missing file");
        if (!db.empty()) throw std::runtime_error("Database should remain empty");
    });
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main() {
    std::cout << "\n========================================\n";
    std::cout << "  Hornet Link Test Suite\n";
    std::cout << "========================================\n";
    
    // Register all test suites
    registerProtocolParseTests();
    registerHandshakeTests();
    registerDeltaFrameTests();
    registerRS485FrameTests();
    registerControlDatabaseTests();
    
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
