// libFuzzer target: UDP datagrams from the Hornet Link native exporter.
#include "HornetNative.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    hornet_native::CatalogState state;
    hornet_native::DatagramHeader header;
    hornet_native::ParseStats stats;
    const std::string_view text(reinterpret_cast<const char*>(data), size);
    if (hornet_native::parseExporterDatagram(text, state, header, stats) == hornet_native::ParseResult::Ok) {
        uint8_t seq = 0;
        (void)hornet_native::buildStateFrames(state, state.takeDirty(), hn::ADDR_BROADCAST, seq);
        hornet_native::SyncTracker sync;
        (void)sync.overlayText(state);
    }
    return 0;
}
