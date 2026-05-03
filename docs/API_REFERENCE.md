# Hornet Link — API Reference

Complete reference for all public C++ types and functions in the PC bridge.

---

## BiosStateMap (`BiosProtocol.hpp`)

64 KiB array representing the DCS-BIOS cockpit state.

```cpp
class BiosStateMap {
public:
    // Read a 16-bit word at an even byte address.
    uint16_t read(uint16_t byteAddr) const;

    // Write a 16-bit word (called by ExportParser).
    void write(uint16_t byteAddr, uint16_t value);

    // Raw byte array pointer (read-only).
    const uint8_t* raw() const;

    // Reset all bytes to zero.
    void clear();
};
```

---

## ExportParser (`BiosProtocol.hpp`)

Byte-stream state machine for the DCS-BIOS export protocol.

```cpp
class ExportParser {
public:
    explicit ExportParser(BiosStateMap& stateMap);

    // Feed raw bytes from the network or COM port.
    void processBytes(const uint8_t* data, size_t len);

    // Force a frame flush (used in UDP mode where datagram = frame).
    void flushFrame();

    // Callback fired after each sync frame is processed.
    // dirty = sorted list of even byte addresses that changed.
    std::function<void(const std::vector<uint16_t>& dirty)> onFrameSync;
};
```

---

## ISimSource (`SimSource.hpp`)

Abstract interface for pluggable simulator data sources.

```cpp
class ISimSource {
public:
    virtual ~ISimSource() = default;

    // Connect to the simulator.  Returns false on error.
    // hwnd is used for PostLog() calls from the worker thread.
    virtual bool connect(BiosStateMap& stateMap, HWND hwnd) = 0;

    // Disconnect and join the worker thread.
    virtual void disconnect() = 0;

    // True when the source is actively receiving data.
    virtual bool isConnected() const = 0;

    // Forward an import command to DCS.
    virtual void sendImport(const ImportCommand& cmd) = 0;

    // Human-readable name shown in log and status bar.
    virtual std::wstring displayName() const = 0;

    // True for stub sources (MsfsSource).
    virtual bool isStub() const { return false; }

    // Callback set by BridgeController::Start() before connect() is called.
    std::function<void(const std::vector<uint16_t>& dirty)> onFrameSync;
};
```

---

## DcsDirectSource (`DcsDirectSource.hpp`)

Receives DCS-BIOS frames via UDP on `127.0.0.1:42002` from the Lua exporter.

```cpp
class DcsDirectSource : public ISimSource {
public:
    static constexpr uint16_t kListenPort = 42002;

    bool connect(BiosStateMap&, HWND) override;
    void disconnect() override;
    bool isConnected() const override;
    void sendImport(const ImportCommand&) override;
    std::wstring displayName() const override;
};
```

---

## DcsBiosSource (`DcsBiosSource.hpp`)

Receives DCS-BIOS frames from the standard UDP multicast or TCP stream.

```cpp
class DcsBiosSource : public ISimSource {
public:
    enum class Mode { Udp, Tcp };
    explicit DcsBiosSource(Mode mode = Mode::Udp);

    bool connect(BiosStateMap&, HWND) override;
    void disconnect() override;
    bool isConnected() const override;
    void sendImport(const ImportCommand&) override;
    std::wstring displayName() const override;
};
```

UDP connects to multicast `239.255.50.10:5010`.
TCP connects to `127.0.0.1:7778`.

---

## ReplayFileSource (`ReplayFileSource.hpp`)

Replays a binary capture file for offline testing.

```cpp
class ReplayFileSource : public ISimSource {
public:
    explicit ReplayFileSource(std::wstring filePath);

    void setSpeedMultiplier(double mult); // Default 1.0

    bool connect(BiosStateMap&, HWND) override;
    void disconnect() override;
    bool isConnected() const override;
    void sendImport(const ImportCommand&) override; // no-op
    std::wstring displayName() const override;
};
```

Binary format: `[timestamp_ms:u32LE][payload_len:u16LE][payload:N bytes]`.

---

## DeviceInfo (`DeviceRegistry.hpp`)

Runtime record for one device connected on a COM port.

```cpp
struct DeviceInfo {
    std::string  comPort;          // e.g. "COM10"
    std::string  deviceName;       // from handshake
    DeviceRole   role;             // Unknown/Legacy/Standalone/RS485Master/RS485Slave
    bool         bidir;            // can send imports
    bool         handshakeDone;

    std::vector<Subscription> subscriptions;
    std::vector<SlaveInfo>    slaves;          // RS-485 masters only

    std::unordered_set<uint16_t> subAddrs;
    bool wantsAll;

    void buildAddrSet();
    bool wantsAddress(uint16_t byteAddr) const;
};
```

---

## SlaveInfo (`DeviceRegistry.hpp`)

RS-485 slave declared by a master during handshake.

```cpp
struct SlaveInfo {
    uint8_t     busAddress;    // 1–254
    std::string name;
    std::vector<Subscription> subs;
    std::unordered_set<uint16_t> subAddrs;
    bool wantsAll;

    void buildAddrSet();
    bool wantsAddress(uint16_t byteAddr) const;
};
```

---

## HandshakeParser (`DeviceRegistry.hpp`)

Parses the device pong frame byte by byte.

```cpp
class HandshakeParser {
public:
    enum class Result { Pending, Complete, Failed };

    Result processByte(uint8_t b);
    bool   complete() const;

    // Transfer parsed data into a DeviceInfo record.
    void populateDevice(DeviceInfo& d) const;

    // Build a 4-byte probe (ping) frame.
    static std::array<uint8_t, 4> pingFrame();

    // Build a 4-byte acknowledgement frame.
    static std::array<uint8_t, 4> ackFrame();
};
```

---

## BuildDeltaFrame (`DeviceRegistry.hpp`)

Builds a filtered DCS-BIOS frame for one device.

```cpp
std::vector<uint8_t> BuildDeltaFrame(
    const BiosStateMap&          state,
    const std::vector<uint16_t>& dirtyAddrs,
    const DeviceInfo&            dev);
```

Returns an empty vector if the device has no interest in any dirty address.
Otherwise returns a complete DCS-BIOS frame (sync + write records).

---

## ProfileStore (`ProfileStore.hpp`)

Persists per-device subscription lists as JSON.

```cpp
class ProfileStore {
public:
    // Load device_profiles.json and templates/panels.json from exeDir.
    void load(const std::wstring& exeDir);

    // Return a profile for deviceName, or nullopt if none found.
    std::optional<DeviceProfile> resolve(const std::string& deviceName) const;

    // Persist a device's subscription list.
    void save(const std::string& deviceName, const DeviceProfile& profile);
};
```

---

## RS485ProtocolSpec (`RS485ProtocolSpec.hpp`)

Protocol constants and utilities.

```cpp
// Constants
constexpr uint8_t  kRS485STX              = 0xFE;
constexpr uint32_t kRS485BaudRate         = 250000;
constexpr uint16_t kRS485ReplyTimeoutMs   = 10;
constexpr uint16_t kRS485KeepAlivePollMs  = 20;
constexpr uint16_t kRS485ProbeTimeoutMs   = 300;
constexpr uint16_t kRS485MaxPayloadBytes  = 512;

// Sub-bus message type bytes
constexpr uint8_t kSubBusMsgProbe       = 0x10;
constexpr uint8_t kSubBusMsgProbeAck    = 0x11;
constexpr uint8_t kSubBusMsgData        = 0x20;
constexpr uint8_t kSubBusMsgImport      = 0x30;
constexpr uint8_t kSubBusMsgMode        = 0x40;
constexpr uint8_t kSubBusMsgModeAck     = 0x41;

// Mode values
constexpr uint8_t kModeValueSim         = 0;
constexpr uint8_t kModeValuePreflight   = 1;
constexpr uint8_t kModeValueMaintenance = 2;

// CRC
uint16_t crc16CcittFalse(const uint8_t* data, size_t len);

// Frame encoding / decoding
struct RS485Frame {
    uint8_t  dst;
    uint8_t  src;
    std::vector<uint8_t> payload;

    std::vector<uint8_t> encode() const;
    static bool verifyCrc(const uint8_t* frame, size_t len);
};
```

---

## BridgeController (`main.cpp`)

Main orchestrator.  Owns the source, COM ports, and dispatch loop.

```cpp
class BridgeController {
public:
    explicit BridgeController(HWND hwnd);

    bool Start(const BridgeConfig& config);
    void Stop();
    bool IsRunning() const;

    void SetMode(BridgeMode mode);
    BridgeMode GetMode() const;

    void SetLoggingFlags(bool changes, bool knobs, bool gauges, bool diagnostics);
};
```

### BridgeConfig

```cpp
struct BridgeConfig {
    SimSourceType  sourceType      = SimSourceType::DcsDirect;
    BridgeMode     initialMode     = BridgeMode::Sim;
    std::wstring   replayFilePath;        // used when sourceType == ReplayFile
    std::vector<int> comPorts;            // port numbers to open
    bool           dryRun          = false;
    bool           logStateChanges = false;
    bool           logRawKnobsDials = true;
    bool           logRawGauges    = false;
    bool           logDiagnostics  = false;
};
```

### BridgeMode

```cpp
enum class BridgeMode : uint8_t {
    Sim         = 0,
    Preflight   = 1,
    Maintenance = 2,
};
```

### SimSourceType

```cpp
enum class SimSourceType {
    DcsDirect,   // Lua UDP receiver on port 42002
    DcsBiosUdp,  // DCS-BIOS multicast UDP 239.255.50.10:5010
    DcsBiosTcp,  // DCS-BIOS TCP 127.0.0.1:7778
    ReplayFile,  // Binary capture replay
    Msfs,        // MSFS 2024 (stub)
};
```
