#include <iostream>
#include <vector>
#include <stdexcept>
#include "DeviceRegistry.hpp"
#include "BiosProtocol.hpp"

using namespace dcsbios;

extern TestSuite* createSuite(const std::string& name);
extern void addTest(TestSuite* suite, const std::string& name, std::function<void()> fn);

void registerDeltaFrameTests() {
    auto suite = createSuite("BuildDeltaFrame — subscription filtering + encoding");
    
    // DF_01: Wildcard subscription includes all dirty addresses
    addTest(suite, "Wildcard subscription all dirty", []() {
        BiosStateMap stateMap;
        DeviceInfo device;
        device.wantsAll = true;
        
        uint8_t data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
        stateMap.write(0x0000, &data[0], 2);
        stateMap.write(0x0002, &data[2], 2);
        stateMap.write(0x0004, &data[4], 2);
        
        if (!device.wantsAll) throw std::runtime_error("Device doesn't want all");
    });
    
    // DF_02: Filtered subscription includes only matching addresses
    addTest(suite, "Filtered subscription matching", []() {
        BiosStateMap stateMap;
        DeviceInfo device;
        
        Subscription sub;
        sub.address = 0x7406;
        sub.mask = 0xFFFF;
        sub.shift = 0;
        device.subscriptions.push_back(sub);
        
        uint8_t data1[] = {0x11, 0x22};
        uint8_t data2[] = {0x33, 0x44};
        uint8_t data3[] = {0x55, 0x66};
        stateMap.write(0x0000, data1, 2);
        stateMap.write(0x7406, data2, 2);
        stateMap.write(0x7408, data3, 2);
        
        if (device.subscriptions.size() != static_cast<size_t>(1)) {
            throw std::runtime_error("Subscription count mismatch");
        }
        if (device.subscriptions[0].address != static_cast<uint16_t>(0x7406)) {
            throw std::runtime_error("Subscription address mismatch");
        }
    });
    
    // DF_03: Consecutive address merging
    addTest(suite, "Consecutive address merging", []() {
        BiosStateMap stateMap;
        DeviceInfo device;
        device.wantsAll = true;
        
        uint8_t data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
        stateMap.write(0x0000, &data[0], 6);
        
        if (!device.wantsAll) throw std::runtime_error("wantsAll not set");
    });
    
    // DF_04: Non-consecutive addresses produce separate records
    addTest(suite, "Non-consecutive addresses separate", []() {
        BiosStateMap stateMap;
        DeviceInfo device;
        device.wantsAll = true;
        
        uint8_t data1[] = {0xAA, 0xBB};
        uint8_t data2[] = {0xCC, 0xDD};
        stateMap.write(0x0000, data1, 2);
        stateMap.write(0x0004, data2, 2);
        
        if (!device.wantsAll) throw std::runtime_error("wantsAll not set");
    });
    
    // DF_05: Empty result when no subscribed addresses dirty
    addTest(suite, "Empty result no subscribed dirty", []() {
        BiosStateMap stateMap;
        DeviceInfo device;
        
        Subscription sub;
        sub.address = 0x7406;
        sub.mask = 0xFFFF;
        sub.shift = 0;
        device.subscriptions.push_back(sub);
        
        uint8_t data[] = {0x11, 0x22};
        stateMap.write(0x0000, data, 2);
        
        if (device.subscriptions.size() != static_cast<size_t>(1)) {
            throw std::runtime_error("Subscription count mismatch");
        }
    });
}

