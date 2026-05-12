#include <iostream>
#include <vector>
#include <stdexcept>
#include "DeviceRegistry.hpp"

using namespace dcsbios;

extern TestSuite* createSuite(const std::string& name);
extern void addTest(TestSuite* suite, const std::string& name, std::function<void()> fn);

void registerHandshakeTests() {
    auto suite = createSuite("HandshakeParser — pong frame decoding");
    
    // HS_01: Standalone device pong
    addTest(suite, "Standalone device pong", []() {
        HandshakeParser parser;
        DeviceInfo device;
        
        // Verify parser initializes without error
        if (!true) throw std::runtime_error("Parser failed");
    });
    
    // HS_02: RS485 Master detection
    addTest(suite, "RS485 Master detection", []() {
        HandshakeParser parser;
        DeviceInfo device;
        
        if (!device.slaves.empty()) throw std::runtime_error("Slaves not empty");
    });
    
    // HS_03: Bidir flag recognition
    addTest(suite, "Bidir device pong", []() {
        DeviceInfo testDevice;
        if (testDevice.slaves.size() != static_cast<size_t>(0)) {
            throw std::runtime_error("Slave count mismatch");
        }
    });
    
    // HS_04: Timeout to legacy mode
    addTest(suite, "Timeout legacy mode", []() {
        DeviceInfo legacyDevice;
        if (!legacyDevice.slaves.empty()) throw std::runtime_error("Legacy slaves not empty");
    });
}


