// libFuzzer target: device -> bridge import command line parser.
#include "DeviceRegistry.hpp"

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    dcsbios::ImportLineParser parser;
    parser.onCommand = [](const dcsbios::ImportCommand& cmd) {
        if (!dcsbios::ImportLineParser::IsValidImportCommand(cmd)) __builtin_trap();
    };
    parser.processBytes(data, size);
    return 0;
}
