// libFuzzer target: RS485 frame CRC verification and encode/verify round trip.
#include "RS485ProtocolSpec.hpp"

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    (void)dcsbios::RS485Frame::verifyCrc(data, size);

    if (size >= 2) {
        size_t len = size - 2;
        const size_t maxLen = static_cast<size_t>(dcsbios::kRS485MaxPayloadBytes);
        if (len > maxLen) len = maxLen;
        auto frame = dcsbios::RS485Frame::encode(data[0], data[1], data + 2,
                                                 static_cast<uint16_t>(len));
        if (!dcsbios::RS485Frame::verifyCrc(frame.data(), frame.size())) __builtin_trap();
    }
    return 0;
}
