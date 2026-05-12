#include <iostream>
#include <vector>
#include <functional>
#include "BiosProtocol.hpp"

using namespace dcsbios;

extern TestSuite* createSuite(const std::string& name);
extern void addTest(TestSuite* suite, const std::string& name, std::function<void()> fn);
extern void TEST_ASSERT(bool cond, const std::string& msg);
extern void TEST_EQUAL(auto a, auto b);
extern void TEST_NOT_EQUAL(auto a, auto b);

void registerProtocolParseTests() {
    auto suite = createSuite("BiosProtocol — ExportParser");
    
    // PP_01: Sync detection
    addTest(suite, "Sync detection", []() {
        BiosStateMap stateMap;
        ExportParser parser;
        int frameCallCount = 0;
        parser.onFrameSync = [&frameCallCount]() { frameCallCount++; };
        
        uint8_t input[] = {0x55, 0x55, 0x55, 0x55};
        parser.processBytes(input, sizeof(input), stateMap);
        if (frameCallCount != 1) throw std::runtime_error("frameCallCount != 1");
    });
    
    // PP_02: Single write record
    addTest(suite, "Single write record", []() {
        BiosStateMap stateMap;
        ExportParser parser;
        parser.onFrameSync = []() {};
        
        uint8_t input[] = {0x55, 0x55, 0x55, 0x55, 0x00, 0x00, 0x02, 0x00, 0xAB, 0xCD};
        parser.processBytes(input, sizeof(input), stateMap);
        if (stateMap.read(0x0000, 2) != static_cast<uint16_t>(0xCDAB)) {
            throw std::runtime_error("stateMap value mismatch");
        }
    });
    
    // PP_03: Multi-record frame
    addTest(suite, "Multi-record frame", []() {
        BiosStateMap stateMap;
        ExportParser parser;
        parser.onFrameSync = []() {};
        
        uint8_t input[] = {
            0x55, 0x55, 0x55, 0x55,  // sync
            0x02, 0x00,              // address 0x0002
            0x04, 0x00,              // length 4
            0x11, 0x22, 0x33, 0x44,  // data
            0x10, 0x00,              // address 0x0010
            0x02, 0x00,              // length 2
            0xFF, 0x00               // data
        };
        parser.processBytes(input, sizeof(input), stateMap);
        
        if (stateMap.read(0x0002, 2) != static_cast<uint16_t>(0x2211)) {
            throw std::runtime_error("0x0002 mismatch");
        }
        if (stateMap.read(0x0004, 2) != static_cast<uint16_t>(0x4433)) {
            throw std::runtime_error("0x0004 mismatch");
        }
        if (stateMap.read(0x0010, 2) != static_cast<uint16_t>(0x00FF)) {
            throw std::runtime_error("0x0010 mismatch");
        }
    });
    
    // PP_04: Fragmented delivery
    addTest(suite, "Fragmented delivery", []() {
        BiosStateMap stateMap;
        ExportParser parser;
        parser.onFrameSync = []() {};
        
        uint8_t frag1[] = {0x55, 0x55};
        uint8_t frag2[] = {0x55, 0x55, 0x00, 0x00};
        uint8_t frag3[] = {0x02, 0x00, 0xAB, 0xCD};
        
        parser.processBytes(frag1, sizeof(frag1), stateMap);
        parser.processBytes(frag2, sizeof(frag2), stateMap);
        parser.processBytes(frag3, sizeof(frag3), stateMap);
        
        if (stateMap.read(0x0000, 2) != static_cast<uint16_t>(0xCDAB)) {
            throw std::runtime_error("fragmented delivery failed");
        }
    });
    
    // PP_05: Spurious bytes before sync
    addTest(suite, "Spurious bytes before sync", []() {
        BiosStateMap stateMap;
        ExportParser parser;
        parser.onFrameSync = []() {};
        
        uint8_t input[] = {
            0xDE, 0xAD, 0xBE, 0xEF,  // spurious
            0x55, 0x55, 0x55, 0x55,  // sync
            0x00, 0x00,              // address 0x0000
            0x02, 0x00,              // length 2
            0x01, 0x00               // data
        };
        parser.processBytes(input, sizeof(input), stateMap);
        
        if (stateMap.read(0x0000, 2) != static_cast<uint16_t>(0x0001)) {
            throw std::runtime_error("spurious bytes test failed");
        }
    });
}


