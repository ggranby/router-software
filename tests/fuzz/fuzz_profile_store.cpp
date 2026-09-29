// libFuzzer target: ProfileStore JSON reader (user-editable files on disk).
#include "ProfileStore.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::string json(reinterpret_cast<const char*>(data), size);
    dcsbios::ProfileStore store;
    (void)store.parseUserProfiles(json);
    (void)store.parseTemplates(json);
    return 0;
}
