// ProfileStore tests: documented file format, malformed input, and save
// round-trips with escaping. Uses a temporary directory per test.

#include "ProfileStore.hpp"
#include "test_framework.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

using namespace dcsbios;
namespace fs = std::filesystem;

namespace {

/// Temporary directory removed on scope exit.
struct TempDir {
    fs::path path;
    TempDir() {
        static int counter = 0;
        auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path = fs::temp_directory_path() /
               ("hornet-link-test-" + std::to_string(stamp) + "-" + std::to_string(++counter));
        fs::create_directories(path / "templates");
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
    void write(const fs::path& rel, const std::string& text) const {
        std::ofstream f(path / rel, std::ios::binary);
        f << text;
    }
};

} // namespace

void registerProfileStoreTests() {
    auto suite = createSuite("ProfileStore - load, validate, save");

    addTest(suite, "Loads documented profile and template format", []() {
        TempDir dir;
        dir.write("device_profiles.json",
                  "{ \"1A1-UIP\": { \"templateName\": \"UIP\", \"subscriptions\": [5128, 5130],"
                  " \"wantsAll\": false, \"futureKey\": {\"x\": [1, 2]},"
                  " \"futureScalars\": [true, false, null, -1.25e+2] } }");
        dir.write(fs::path("templates") / "panels.json",
                  "{ \"MASTER_ARM\": { \"description\": \"d\", \"subscriptions\": [13312] } }");
        ProfileStore store;
        EXPECT_EQ(store.load(dir.path), size_t(2));
        EXPECT_TRUE(store.lastError().empty());

        auto p = store.resolve("1A1-UIP");
        EXPECT_TRUE(p.has_value());
        EXPECT_TRUE(p->fromUserProfile);
        EXPECT_EQ(p->subscriptions.size(), size_t(2));
        EXPECT_EQ(p->subscriptions[1], 5130);

        auto t = store.resolve("2A1-MASTER_ARM");
        EXPECT_TRUE(t.has_value());
        EXPECT_TRUE(!t->fromUserProfile);
        EXPECT_TRUE(t->templateName == "MASTER_ARM");
        EXPECT_TRUE(!store.resolve("UNKNOWN").has_value());
    });

    addTest(suite, "Malformed files are rejected without hanging", []() {
        const char* bad[] = {
            "{ \"X\": { \"subscriptions\": [a] } }",
            "{ \"X\": { \"subscriptions\": [-1] } }",
            "{ \"X\": { \"subscriptions\": [99999999999999999999] } }",
            "{ \"X\": { \"subscriptions\": [70000] } }",
            "{ \"X\": { \"templateName\": \"unterminated } }",
            "{ \"X\": { \"subscriptions\": [ { \"address\": 0x741E } ] } }",
            "{ \"X\": { \"futureKey\": garbage } }",
            "{ \"X\": { \"futureKey\": NaN } }",
            "{ \"X\": { \"futureKey\": 01 } }",
            "{ \"X\": { \"futureKey\": 1. } }",
            "{ \"X\": { \"futureKey\": 1e+ } }",
            "{ \"X\": ",
            "[]",
            "",
        };
        for (const char* text : bad) {
            TempDir dir;
            dir.write("device_profiles.json", text);
            ProfileStore store;
            EXPECT_EQ(store.load(dir.path), size_t(0));
            EXPECT_TRUE(!store.resolve("X").has_value());
        }
    });

    addTest(suite, "Oversized file is rejected", []() {
        TempDir dir;
        std::string big = "{ \"X\": { \"templateName\": \"" +
                          std::string(static_cast<size_t>(kProfileFileMaxBytes), 'a') + "\" } }";
        dir.write("device_profiles.json", big);
        ProfileStore store;
        EXPECT_EQ(store.load(dir.path), size_t(0));
        EXPECT_TRUE(!store.lastError().empty());
    });

    addTest(suite, "Save round-trips names that need escaping", []() {
        TempDir dir;
        ProfileStore store;
        store.load(dir.path);
        DeviceProfile p;
        p.templateName  = "Quote\"Back\\slash";
        p.subscriptions = {1, 2, 65535};
        p.wantsAll      = true;
        EXPECT_TRUE(store.save("1A1-\"ODD\\NAME\"", p));

        ProfileStore reloaded;
        EXPECT_EQ(reloaded.load(dir.path), size_t(1));
        auto r = reloaded.resolve("1A1-\"ODD\\NAME\"");
        EXPECT_TRUE(r.has_value());
        EXPECT_TRUE(r->templateName == p.templateName);
        EXPECT_TRUE(r->subscriptions == p.subscriptions);
        EXPECT_TRUE(r->wantsAll);
        EXPECT_TRUE(!fs::exists(dir.path / "device_profiles.json.tmp"));
    });

    addTest(suite, "Save rejects invalid device names", []() {
        TempDir dir;
        ProfileStore store;
        store.load(dir.path);
        DeviceProfile p;
        EXPECT_TRUE(!store.save("", p));
        EXPECT_TRUE(!store.lastError().empty());
        EXPECT_TRUE(!store.save(std::string("bad\nname"), p));
        EXPECT_TRUE(!store.save(std::string(kDeviceNameMaxBytes + 1, 'a'), p));
        EXPECT_TRUE(!fs::exists(dir.path / "device_profiles.json"));
    });

    addTest(suite, "Save without load reports an error", []() {
        ProfileStore store;
        EXPECT_TRUE(!store.save("NAME", DeviceProfile{}));
        EXPECT_TRUE(!store.lastError().empty());
    });

    addTest(suite, "In-memory parse reports origin on error", []() {
        ProfileStore store;
        EXPECT_EQ(store.parseUserProfiles(R"({"A":{"wantsAll":true}})"), size_t(1));
        EXPECT_TRUE(store.resolve("A").has_value());
        EXPECT_EQ(store.parseTemplates("{", "panels.json"), size_t(0));
        EXPECT_TRUE(store.lastError().find("panels.json") != std::string::npos);
    });

    addTest(suite, "Template names decode Unicode escapes without collisions", []() {
        ProfileStore store;
        EXPECT_EQ(store.parseTemplates(
            R"({"\u00e9":{"subscriptions":[1]},"\u00e8":{"subscriptions":[2]},"\ud83d\ude00":{"subscriptions":[3]}})"),
            size_t(3));
        EXPECT_TRUE(store.templateSubscriptions(u8"é").value_or(std::vector<uint16_t>{}) ==
                    std::vector<uint16_t>{1});
        EXPECT_TRUE(store.templateSubscriptions(u8"è").value_or(std::vector<uint16_t>{}) ==
                    std::vector<uint16_t>{2});
        EXPECT_TRUE(store.templateSubscriptions(u8"😀").value_or(std::vector<uint16_t>{}) ==
                    std::vector<uint16_t>{3});

        EXPECT_EQ(store.parseTemplates(R"({"\ud83d":{"subscriptions":[]}})"), size_t(0));
        EXPECT_TRUE(!store.lastError().empty());
    });
}
