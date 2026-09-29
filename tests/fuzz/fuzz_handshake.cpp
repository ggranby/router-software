// libFuzzer target: handshake response parser (bytes from an untrusted device).
#include "DeviceRegistry.hpp"

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    dcsbios::HandshakeParser parser;
    for (size_t i = 0; i < size; ++i) {
        auto r = parser.processByte(data[i]);
        if (r == dcsbios::HandshakeParser::Result::Complete) {
            dcsbios::DeviceInfo dev;
            parser.populateDevice(dev);
            break;
        }
        if (r != dcsbios::HandshakeParser::Result::Pending) break;
    }
    return 0;
}
