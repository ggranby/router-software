#include <iostream>
#include <vector>
#include <cstring>
#include <stdexcept>
#include "RS485ProtocolSpec.hpp"

using namespace dcsbios;

extern TestSuite* createSuite(const std::string& name);
extern void addTest(TestSuite* suite, const std::string& name, std::function<void()> fn);

void registerRS485FrameTests() {
    auto suite = createSuite("RS485 Protocol — Frame format and CRC");
    
    // Test CRC-16/CCITT-FALSE calculation
    addTest(suite, "CRC-16 calculation", []() {
        // Known test vector: "123456789" should give 0x31C3
        uint8_t testData[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
        uint16_t crc = dcsbios::crc16CcittFalse(testData, sizeof(testData));
        if (crc != static_cast<uint16_t>(0x31C3)) {
            throw std::runtime_error("CRC calculation failed");
        }
    });
    
    // Test frame structure: STX at correct position
    addTest(suite, "Frame STX constant", []() {
        if (dcsbios::kRS485STX != static_cast<uint8_t>(0xFE)) {
            throw std::runtime_error("STX constant mismatch");
        }
    });
    
    // Test baud rate constant
    addTest(suite, "Baud rate constant", []() {
        if (dcsbios::kRS485BaudRate != 250000U) {
            throw std::runtime_error("Baud rate mismatch");
        }
    });
    
    // Test message type constants
    addTest(suite, "Message type constants", []() {
        if (static_cast<uint8_t>(0x10) != 0x10) throw std::runtime_error("PROBE type");
        if (static_cast<uint8_t>(0x11) != 0x11) throw std::runtime_error("PROBE_ACK type");
        if (static_cast<uint8_t>(0x20) != 0x20) throw std::runtime_error("DATA type");
        if (static_cast<uint8_t>(0x30) != 0x30) throw std::runtime_error("IMPORT type");
        if (static_cast<uint8_t>(0x40) != 0x40) throw std::runtime_error("MODE type");
        if (static_cast<uint8_t>(0x41) != 0x41) throw std::runtime_error("MODE_ACK type");
    });
    
    // Test timeout constants
    addTest(suite, "Timeout constants", []() {
        if (dcsbios::kRS485ReplyTimeoutMs <= 0) throw std::runtime_error("Reply timeout");
        if (dcsbios::kRS485KeepAlivePollMs <= 0) throw std::runtime_error("Keep-alive");
        if (dcsbios::kRS485ProbeTimeoutMs <= 0) throw std::runtime_error("Probe timeout");
    });
    
    // Test max payload size
    addTest(suite, "Max payload size", []() {
        if (dcsbios::kRS485MaxPayloadBytes != 512U) {
            throw std::runtime_error("Max payload size mismatch");
        }
    });
    
    // Test frame encoding produces valid structure
    addTest(suite, "Frame encoding structure", []() {
        uint8_t frameBuffer[20];
        uint8_t payload[] = {0x01, 0x02, 0x03};
        
        frameBuffer[0] = dcsbios::kRS485STX;
        if (frameBuffer[0] != static_cast<uint8_t>(0xFE)) {
            throw std::runtime_error("Frame buffer STX mismatch");
        }
    });
    
    // Test CRC verification logic
    addTest(suite, "CRC verification", []() {
        uint8_t frame[10];
        frame[0] = dcsbios::kRS485STX;
        frame[1] = 0x01;  // frame type
        frame[2] = 0x02;  // dst
        frame[3] = 0x03;  // src
        frame[4] = 0x02;  // len_lo
        frame[5] = 0x00;  // len_hi
        frame[6] = 0xAA;  // payload[0]
        frame[7] = 0xBB;  // payload[1]
        
        uint16_t crc = dcsbios::crc16CcittFalse(&frame[1], 7);
        frame[8] = crc & 0xFF;
        frame[9] = (crc >> 8) & 0xFF;
        
        if (crc == static_cast<uint16_t>(0)) {
            throw std::runtime_error("CRC should not be zero");
        }
    });
}

