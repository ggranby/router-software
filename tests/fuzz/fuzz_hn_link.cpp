// libFuzzer target: untrusted bytes into every v2 endpoint (bridge session,
// USB panel, bus slave, bus master on both ports).
#include "HornetNative.hpp"
#include "protocol/HnMaster.h"

#include <cstddef>
#include <cstdint>

namespace {

struct FuzzPort : hn::Port {
    const uint8_t* data = nullptr;
    size_t size = 0;
    size_t pos = 0;
    int read() override { return pos < size ? data[pos++] : -1; }
    void write(const uint8_t*, size_t) override {}
};

struct Handler : hn::NodeHandler {
    void fillHello(hn::Hello& h) override { h.catalogHash = Hornet::kCatalogHash; }
    uint16_t syncCount() override { return 3; }
    hn::SyncRecord syncItem(uint16_t i) override { hn::SyncRecord r; r.id = static_cast<uint16_t>(0x0101 + i); return r; }
    uint16_t describeCount() override { return 50; }
};

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    hornet_native::SyncTracker sync;
    hornet_native::LinkSession session;
    session.sync = &sync;
    session.write = [](const uint8_t*, size_t) {};
    session.feed(data, size);

    Handler handler;
    FuzzPort usb;
    usb.data = data;
    usb.size = size;
    hn::Node panel(usb, handler);
    panel.beginUsb();
    for (uint32_t t = 1; t < 20; t++) panel.update(t * 50);

    FuzzPort bus;
    bus.data = data;
    bus.size = size;
    hn::Node slave(bus, handler);
    slave.beginBus(7);
    for (uint32_t t = 1; t < 20; t++) slave.update(t * 50);

    FuzzPort up, down;
    up.data = down.data = data;
    up.size = down.size = size;
    hn::BusMaster master(up, down);
    master.addSlave(7);
    master.enableDiscovery(true);
    for (uint32_t t = 1; t < 50; t++) master.update(t * 3);
    return 0;
}
