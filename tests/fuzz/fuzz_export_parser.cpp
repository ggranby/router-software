// libFuzzer target: DCS-BIOS export stream parser + state map + delta frames.
#include "BiosProtocol.hpp"
#include "DeviceRegistry.hpp"

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    dcsbios::BiosStateMap map;
    dcsbios::ExportParser parser(map);

    dcsbios::DeviceInfo dev;
    dev.subscriptions.push_back(dcsbios::Subscription{});  // wildcard
    dev.buildAddrSet();

    parser.onFrameSync = [&]() {
        auto dirty = map.takeDirty();
        (void)dcsbios::BuildDeltaFrame(map, dirty, dev);
    };
    parser.processBytes(data, size);
    parser.flushFrame();
    return 0;
}
