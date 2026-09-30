// libFuzzer target: protocol v2 frame decoder, every record reader and the
// bridge's readable frame decoder.
#include "HornetNative.hpp"

#include <cstddef>
#include <cstdint>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    hn::FrameDecoder dec;
    for (size_t i = 0; i < size; i++) {
        if (dec.feed(data[i]) != hn::FrameDecoder::Result::Frame) continue;
        const uint8_t* p = dec.payload();
        const size_t n = dec.header().len;
        (void)hornet_native::decodeFrame(dec.header(), p, n);

        hn::StateReader sr(p, n);
        hn::StateRecord s;
        while (sr.next(s)) {}
        hn::InputReader ir(p, n);
        hn::InputRecord in;
        while (ir.next(in)) (void)hornet_native::validateInput(in);
        hn::SyncReader yr(p, n);
        hn::SyncRecord y;
        while (yr.next(y)) {}
        hn::DescribeReader dr(p, n);
        hn::DescribeRecord d;
        while (dr.next(d)) {}
        hn::Hello h;
        (void)hn::readHello(p, n, h);
        hn::Diag g;
        (void)hn::readDiag(p, n, g);
        hornet_native::Subscription sub;
        (void)hornet_native::Subscription::parse(p, n, sub);
    }
    return 0;
}
