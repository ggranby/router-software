/**
 * @file main.cpp
 * @brief DCS-BIOS Serial Bridge — Win32 application entry point and bridge controller.
 *
 * @details
 * Receives the DCS-BIOS export stream (UDP multicast 239.255.50.10:5010, or
 * optionally TCP 127.0.0.1:7778), decodes it into an in-memory 64 KB state
 * map, and dispatches compressed delta frames to all connected COM-port devices.
 *
 * Each device undergoes an optional 3-way handshake on connect to negotiate:
 *   - RS-485 topology role (standalone / master / slave)
 *   - Subscription list — which address words the device wants to receive
 *   - Bidirectional flag — whether the device can send import commands to DCS
 *
 * Import commands received from bidirectional devices are forwarded to DCS via
 * UDP to `127.0.0.1:7778` in the standard DCS-BIOS plain-text format.
 *
 * ### Threading model
 * | Thread | Responsibility |
 * |--------|----------------|
 * | UI thread (WndProc) | Win32 message loop, user interaction |
 * | Worker thread (RunLoop) | UDP/TCP receive, ExportParser, OnFrameSync dispatch |
 * | ReadDeviceThread (one per bidir port) | Serial read, ImportLineParser |
 *
 * OnFrameSync() is called synchronously from RunLoop() on the worker thread.
 * The BiosStateMap and dirty-address list are only ever accessed from the
 * worker thread, so no additional locking is needed for those structures.
 *
 * @copyright Copyright 2016-2026 Hornet Link contributors.
 *            Licensed under the Apache License, Version 2.0.
 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <shlwapi.h>
#include <commdlg.h>
#include <shellapi.h>

#include "BiosProtocol.hpp"
#include "ControlDatabase.hpp"
#include "DeviceRegistry.hpp"
#include "SimSource.hpp"
#include "DcsBiosSource.hpp"
#include "DcsDirectSource.hpp"
#include "ReplayFileSource.hpp"
#include "MsfsSource.hpp"
#include "ProfileStore.hpp"
#include "RS485ProtocolSpec.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comdlg32.lib")

namespace {

using namespace dcsbios;

// ─── Window message and control IDs ──────────────────────────────────────────
constexpr int kUdpPort   = 5010;
constexpr int kTcpPort   = 7778;

/// Maximum bytes consumed from the network in a single recv() call.
constexpr int    kExportDatagramMaxBytes = 4096;
/// Legacy alias kept for readability at call sites.
constexpr int    kBufferSize = kExportDatagramMaxBytes;

/// After the bridge starts, state-change logging is suppressed for this long
/// so the operator does not see the initial flood of cockpit-initialisation
/// writes that DCS emits while loading a mission. 15 seconds is empirically
/// long enough for the FA-18C cockpit to reach a stable state on most systems.
constexpr auto   kLogStartupSettleWindow = std::chrono::seconds(15);

/// Timeout applied to recv() / recvfrom() so the RunLoop can check running_
/// even when no export data is arriving (e.g. while DCS is in the menu).
constexpr DWORD  kSocketReceiveTimeoutMs = 5000;
constexpr int kHotkeyId  = 1;
constexpr int kLogMessage          = WM_APP + 1;
constexpr int kBridgeStoppedMessage = WM_APP + 2;
constexpr UINT_PTR kLogFlushTimer = 42;
constexpr UINT_PTR kStatusRefreshTimer = 43;
constexpr UINT_PTR kLogFlushIntervalMs = 50;
constexpr UINT_PTR kStatusRefreshIntervalMs = 250;
constexpr size_t kMaxPendingUiLines = 2000;
constexpr size_t kMaxUiDisplayLines = 400;
/// Maximum number of lines retained in the in-memory logBuffer.
/// Older lines are silently dropped. Use stream-to-disk capture for full retention.
constexpr size_t kMaxLogBufferLines = 5000;

constexpr UINT_PTR kControlMode       = 1001;
constexpr UINT_PTR kControlPorts      = 1002;
constexpr UINT_PTR kControlAutoDetect = 1003;
constexpr UINT_PTR kControlToggle     = 1004;
constexpr UINT_PTR kControlStatus     = 1005;
constexpr UINT_PTR kControlLog        = 1006;
constexpr UINT_PTR kControlDryRun     = 1007;
constexpr UINT_PTR kControlExportLog  = 1008;
constexpr UINT_PTR kControlLogChanges = 1009;
constexpr UINT_PTR kControlSelfTest   = 1010;
constexpr UINT_PTR kControlRawKnobs         = 1011;
constexpr UINT_PTR kControlRawGauges        = 1012;
constexpr UINT_PTR kControlLogDiagnostics   = 1013; ///< Transport Diagnostics log channel checkbox
constexpr UINT_PTR kControlCapture          = 1014; ///< Start / Stop stream-to-disk capture button
constexpr UINT_PTR kControlModeSimBtn       = 1015; ///< "Sim" mode toolbar button
constexpr UINT_PTR kControlModePreflightBtn = 1016; ///< "Preflight" mode toolbar button
constexpr UINT_PTR kControlModeMainBtn      = 1017; ///< "Maintenance" mode toolbar button

// ─── Dark-theme colours ───────────────────────────────────────────────────────
constexpr COLORREF kColorBg        = RGB(26,  28,  34);
constexpr COLORREF kColorPanel     = RGB(44,  48,  56);
constexpr COLORREF kColorText      = RGB(225, 230, 240);
constexpr COLORREF kColorTextMuted = RGB(155, 165, 180);
constexpr COLORREF kColorInputBg   = RGB(35,  38,  46);
constexpr COLORREF kColorButtonBg  = RGB(30,  33,  40);

// ─── Bridge mode + source type enums ─────────────────────────────────────────

/**
 * @brief Hub operating mode, broadcast to all connected devices on change.
 *
 * Matches plan.md Phase 2 mode system.  The numeric values map to the
 * kModeValue* constants in RS485ProtocolSpec.hpp.
 */
enum class BridgeMode : uint8_t {
    Sim         = 0,  ///< Normal operation — import commands forwarded to simulator
    Preflight   = 1,  ///< Switch states tracked; imports NOT forwarded to simulator
    Maintenance = 2,  ///< Same as Preflight + diagnostic output enabled hub-wide
};

/**
 * @brief Which simulator data source to use for this session.
 *
 * Maps to the source selector combo box in the UI (plan.md Phase 1, item 7).
 */
enum class SimSourceType {
    DcsDirect,   ///< Hornet Link Lua export (primary, new)
    DcsBiosUdp,  ///< DCS-BIOS UDP multicast (secondary, backward compat)
    DcsBiosTcp,  ///< DCS-BIOS TCP stream (secondary, backward compat)
    ReplayFile,  ///< Replay a recorded dcsbios_data.json file
    Msfs,        ///< MSFS 2024 (stub — not yet implemented)
};

// ─── Config / startup structs ─────────────────────────────────────────────────

/**
 * @brief Runtime configuration for one bridge session.
 *
 * Assembled from the UI controls when the operator clicks Start, then passed
 * to BridgeController::Start(). Immutable for the lifetime of the session.
 */
struct BridgeConfig {
    SimSourceType sourceType     = SimSourceType::DcsDirect; ///< Which sim source to use
    BridgeMode    initialMode    = BridgeMode::Sim;           ///< Starting hub mode
    bool dryRun           = false;    ///< True → parse frames but skip all serial writes
    bool logStateChanges  = false;    ///< True → emit state-change lines to the operator log
    bool logRawKnobsDials = true;     ///< True → also log high-range pilot-actuated controls
    bool logRawGauges     = false;    ///< True → also log read-only gauge outputs
    bool logDiagnostics   = false;    ///< True → emit transport diagnostics (heartbeats, reconnects)
    std::vector<int> comPorts;        ///< Ordered list of COM port numbers to open
    std::string jsonDir;              ///< DCS-BIOS JSON directory (auto-detected when empty)
    std::wstring replayFilePath;      ///< Path to replay file (only used when sourceType=ReplayFile)
};

/**
 * @brief Command-line options applied once at application startup.
 *
 * Parsed by ParseStartupOptions() from GetCommandLineW() and used to
 * pre-populate UI controls and optionally trigger auto-start.
 */
struct StartupOptions {
    bool autoStart = false;    ///< True → call Start() immediately after window creation
    bool dryRun    = false;    ///< Pre-check the dry-run checkbox
    std::optional<bool>             forceUdp; ///< Override the mode combo (true=UDP, false=TCP)
    std::optional<std::vector<int>> ports;    ///< Pre-fill the COM ports field
};

// ─── Utility helpers ─────────────────────────────────────────────────────────

/**
 * @brief Post a timestamped message to the UI log via the window message queue.
 *
 * @details
 * Allocates a `std::wstring` on the heap and sends its address as the LPARAM
 * of a `kLogMessage` (WM_APP+1) message. The WndProc takes ownership and
 * deletes the string after appending it to the log text buffer.
 *
 * Safe to call from any thread — PostMessage is thread-safe on Win32.
 *
 * @param hwnd     Target window handle.
 * @param message  Text to display (without timestamp or line ending).
 */
void PostLog(HWND hwnd, const std::wstring& message) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm timeinfo;
    localtime_s(&timeinfo, &time);
    wchar_t timestamp[32];
    wcsftime(timestamp, sizeof(timestamp) / sizeof(wchar_t), L"[%H:%M:%S]", &timeinfo);
    auto* payload = new std::wstring(std::wstring(timestamp) + L" " + message + L"\r\n");
    PostMessage(hwnd, kLogMessage, 0, reinterpret_cast<LPARAM>(payload));
}

std::wstring Trim(const std::wstring& input) {
    size_t start = 0;
    while (start < input.size() && iswspace(input[start])) ++start;
    size_t end = input.size();
    while (end > start && iswspace(input[end - 1])) --end;
    return input.substr(start, end - start);
}

// Forward declarations for helpers used by config functions below.
std::vector<int> ParsePorts(const std::wstring& raw);
std::wstring     JoinPorts(const std::vector<int>& ports);

std::vector<int> ParsePorts(const std::wstring& raw) {
    std::wstring s = raw;
    std::replace(s.begin(), s.end(), L';', L',');
    std::replace(s.begin(), s.end(), L' ', L',');
    std::vector<int> ports;
    std::wstringstream ss(s);
    std::wstring chunk;
    while (std::getline(ss, chunk, L',')) {
        chunk = Trim(chunk);
        if (chunk.empty()) continue;
        int v = _wtoi(chunk.c_str());
        if (v > 0 && std::find(ports.begin(), ports.end(), v) == ports.end())
            ports.push_back(v);
    }
    return ports;
}

bool StartsWithI(const std::wstring& value, const std::wstring& prefix) {
    if (value.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i)
        if (towlower(value[i]) != towlower(prefix[i])) return false;
    return true;
}

StartupOptions ParseStartupOptions() {
    StartupOptions opt;
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return opt;
    for (int i = 1; i < argc; ++i) {
        std::wstring arg = argv[i];
        if (arg == L"--autostart") { opt.autoStart = true; continue; }
        if (arg == L"--dry-run")   { opt.dryRun    = true; continue; }
        if (arg == L"--udp")       { opt.forceUdp  = true; continue; }
        if (arg == L"--tcp")       { opt.forceUdp  = false; continue; }
        if (StartsWithI(arg, L"--ports=")) {
            opt.ports = ParsePorts(arg.substr(8)); continue;
        }
        int p = _wtoi(arg.c_str());
        if (p > 0) opt.ports = std::vector<int>{p};
    }
    LocalFree(argv);
    return opt;
}

// ─── Saved config (hornet-link.ini beside the executable) ─────────────────────

/// Returns the full path to `hornet-link.ini` placed next to the executable.
static std::wstring GetConfigFilePath() {
    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring path(exePath);
    size_t slash = path.find_last_of(L"\\/");
    if (slash != std::wstring::npos)
        path.resize(slash + 1);
    path += L"hornet-link.ini";
    return path;
}

static constexpr wchar_t kIniSection[] = L"HornetLink";

/**
 * @brief Load persisted settings into @p opts, skipping any field already set
 *        by a command-line argument.  No-op if the INI file does not exist yet.
 */
static void LoadSavedConfig(StartupOptions& opts) {
    std::wstring ini = GetConfigFilePath();
    if (GetFileAttributesW(ini.c_str()) == INVALID_FILE_ATTRIBUTES) return; // first run

    if (!opts.ports.has_value()) {
        wchar_t buf[256] = {};
        GetPrivateProfileStringW(kIniSection, L"ports", L"", buf, 256, ini.c_str());
        auto p = ParsePorts(buf);
        if (!p.empty()) opts.ports = std::move(p);
    }

    if (!opts.forceUdp.has_value()) {
        int src = GetPrivateProfileIntW(kIniSection, L"source", -1, ini.c_str());
        if (src == 1) opts.forceUdp = true;  // DCS-BIOS UDP
        if (src == 2) opts.forceUdp = false; // DCS-BIOS TCP
        // 0 (DcsDirect) and other values leave forceUdp unset so the combo default wins
    }
}

/**
 * @brief Persist the settings from a successfully-started session so they are
 *        pre-filled on the next launch.
 */
static void SaveCurrentConfig(const BridgeConfig& cfg) {
    std::wstring ini = GetConfigFilePath();
    WritePrivateProfileStringW(kIniSection, L"ports",
        JoinPorts(cfg.comPorts).c_str(), ini.c_str());
    wchar_t srcBuf[8] = {};
    swprintf_s(srcBuf, L"%d", static_cast<int>(cfg.sourceType));
    WritePrivateProfileStringW(kIniSection, L"source", srcBuf, ini.c_str());
}

std::vector<int> DetectAvailableComPorts() {
    std::vector<int> ports;
    for (int i = 1; i <= 256; ++i) {
        wchar_t name[16] = {}, path[512] = {};
        swprintf_s(name, L"COM%d", i);
        if (QueryDosDeviceW(name, path, static_cast<DWORD>(std::size(path))) != 0)
            ports.push_back(i);
    }
    return ports;
}

std::wstring JoinPorts(const std::vector<int>& ports) {
    std::wstringstream out;
    for (size_t i = 0; i < ports.size(); ++i) {
        if (i > 0) out << L", ";
        out << ports[i];
    }
    return out.str();
}

bool ConfigureSerialPort(HANDLE h) {
    DCB dcb = {};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(h, &dcb)) return false;
    dcb.BaudRate     = 250000;
    dcb.ByteSize     = 8;
    dcb.Parity       = NOPARITY;
    dcb.StopBits     = ONESTOPBIT;
    dcb.fDtrControl  = DTR_CONTROL_DISABLE;
    dcb.fRtsControl  = RTS_CONTROL_DISABLE;
    if (!SetCommState(h, &dcb)) return false;
    COMMTIMEOUTS ct = {};
    ct.ReadIntervalTimeout        = MAXDWORD;
    ct.ReadTotalTimeoutMultiplier = 0;
    ct.ReadTotalTimeoutConstant   = 0;
    ct.WriteTotalTimeoutMultiplier= 0;
    ct.WriteTotalTimeoutConstant  = 50;
    return SetCommTimeouts(h, &ct) == TRUE;
}

// Search for DCS-BIOS JSON control-reference directory near the executable.
/**
 * @brief Locate the DCS-BIOS JSON control-reference directory.
 *
 * @details
 * Searches the following locations in order and returns the first path that
 * exists and contains at least one `.json` file:
 *
 * 1. `%USERPROFILE%\Saved Games\DCS\Scripts\DCS-BIOS\doc\json`
 * 2. Same with `DCS.openbeta` and `DCS.openalpha` variants
 * 3. `json\` relative to the executable directory
 * 4. Several `..\..\Scripts\DCS-BIOS\doc\json` depth variants relative to the
 *    executable (covers development tree layouts)
 *
 * @return UTF-8 path string, or an empty string if no suitable directory is found.
 */
static std::string FindJsonDir() {
    /// Return true when @p dir exists and contains at least one `.json` file.
    auto hasJsonFiles = [](const std::wstring& dir) {
        WIN32_FIND_DATAW findData = {};
        std::wstring pattern = dir + L"\\*.json";
        HANDLE h = FindFirstFileW(pattern.c_str(), &findData);
        if (h == INVALID_HANDLE_VALUE) return false;
        FindClose(h);
        return true;
    };

    /// Convert a wide-character Windows path to a UTF-8 std::string.
    auto toUtf8 = [](const std::wstring& path) -> std::string {
        int n = WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (n <= 1) return {};
        std::string result(static_cast<size_t>(n - 1), '\0');
        WideCharToMultiByte(CP_UTF8, 0, path.c_str(), -1, result.data(), n, nullptr, nullptr);
        return result;
    };

    std::vector<std::wstring> candidates;

    wchar_t userProfile[MAX_PATH] = {};
    if (GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH) > 0) {
        candidates.push_back(std::wstring(userProfile) + L"\\Saved Games\\DCS\\Scripts\\DCS-BIOS\\doc\\json");
        candidates.push_back(std::wstring(userProfile) + L"\\Saved Games\\DCS.openbeta\\Scripts\\DCS-BIOS\\doc\\json");
        candidates.push_back(std::wstring(userProfile) + L"\\Saved Games\\DCS.openalpha\\Scripts\\DCS-BIOS\\doc\\json");
    }

    wchar_t exePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring dir = exePath;
    auto slash = dir.rfind(L'\\');
    if (slash != std::wstring::npos) dir = dir.substr(0, slash);

    const std::vector<std::wstring> suffixes = {
        L"\\json",
        L"\\..\\..\\Scripts\\DCS-BIOS\\doc\\json",
        L"\\..\\..\\..\\Scripts\\DCS-BIOS\\doc\\json",
        L"\\..\\..\\..\\..\\Scripts\\DCS-BIOS\\doc\\json",
    };
    for (const auto& suffix : suffixes) {
        wchar_t buf[MAX_PATH] = {};
        if (PathCanonicalizeW(buf, (dir + suffix).c_str())) {
            candidates.push_back(buf);
        }
    }

    for (const auto& candidate : candidates) {
        if (GetFileAttributesW(candidate.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
        if (!hasJsonFiles(candidate)) continue;
        return toUtf8(candidate);
    }
    return {};
}

// ─── OnlineAverage ────────────────────────────────────────────────────────────

/**
 * @brief Thread-safe incremental mean tracker using Welford's online algorithm.
 *
 * @details
 * Welford's algorithm computes a running mean without accumulating a sum,
 * which avoids catastrophic cancellation for large sample counts:
 * @code
 *   new_mean = old_mean + (x - old_mean) / n
 * @endcode
 *
 * All operations on the mean and count are performed on std::atomic values
 * using relaxed loads/stores. The update is not atomically consistent as a
 * whole — a reader may observe a count that is one ahead of the mean, which
 * is acceptable for performance-monitoring instrumentation.
 *
 * @thread_safety
 * Individual load and store operations are atomic. The update sequence
 * (load count, compute new mean, store mean) is not a single atomic
 * transaction, so there is a small window for data races. This is intentional:
 * the metrics are advisory and the overhead of a mutex is not warranted.
 */
struct OnlineAverage {
    std::atomic<uint64_t> count{0}; ///< Number of samples observed so far
    std::atomic<uint64_t> mean{0};  ///< Current running mean (integer microseconds)
    std::atomic<uint64_t> max{0};   ///< Maximum sample value observed

    /**
     * @brief Add a new sample to the running statistics.
     * @param sample  The new measurement to incorporate (microseconds).
     */
    void update(uint64_t sample) {
        uint64_t n    = ++count;
        uint64_t prev = mean.load(std::memory_order_relaxed);
        int64_t  delta = static_cast<int64_t>(sample) - static_cast<int64_t>(prev);
        mean.store(static_cast<uint64_t>(static_cast<int64_t>(prev) + delta / static_cast<int64_t>(n)),
                   std::memory_order_relaxed);
        uint64_t curMax = max.load(std::memory_order_relaxed);
        if (sample > curMax)
            max.store(sample, std::memory_order_relaxed);
    }

    /// Reset all statistics to zero.
    void reset() { count = 0; mean = 0; max = 0; }
};

// ─── BridgeController ─────────────────────────────────────────────────────────

/**
 * @brief Core orchestrator that wires together the network, serial ports,
 *        and protocol layers for one bridge session.
 *
 * @details
 * Lifecycle:
 * 1. Construct with the parent HWND (used for PostLog() and stop notification).
 * 2. Call Start() with a fully populated BridgeConfig. Returns false on error.
 * 3. The worker thread (RunLoop()) starts and drives the receive/dispatch loop.
 * 4. Call Stop() (or let the destructor do it) to cleanly tear down all threads
 *    and release resources.
 *
 * ### Internal threading
 * | Thread | Created by | Runs |
 * |--------|-----------|------|
 * | Worker (RunLoop) | Start() | Network recv → ExportParser → OnFrameSync |
 * | ReadDeviceThread | OpenSerialPorts() | One per bidir port |
 *
 * OnFrameSync() is called synchronously on the worker thread. BiosStateMap
 * and the dirty-address vector are only accessed from that thread.
 *
 * @thread_safety
 * Start(), Stop(), IsRunning(), and GetDispatchMetrics() may be called from
 * the UI thread while the worker thread is running. Internal access to shared
 * sockets and port handles is protected by resourceMutex_.
 */
class BridgeController {
public:
    /**
     * @brief Immutable snapshot of dispatch timing metrics.
     *
     * Returned by GetDispatchMetrics() for display in the status bar. All
     * values are in microseconds unless stated otherwise.
     */
    struct DispatchMetricsSnapshot {
        uint64_t framesObserved         = 0; ///< Total frames processed since Start()
        uint64_t framesWithSerialWrites  = 0; ///< Frames that produced at least one serial write
        uint64_t avgDispatchUs           = 0; ///< Running mean frame dispatch latency (µs)
        uint64_t maxDispatchUs           = 0; ///< Peak frame dispatch latency observed (µs)
        uint64_t avgPortWriteUs          = 0; ///< Running mean per-port WriteFile() latency (µs)
        uint64_t maxPortWriteUs          = 0; ///< Peak per-port WriteFile() latency observed (µs)
    };

    /**
     * @brief Construct a controller whose log messages go to @p hwnd.
     * @param hwnd  Parent window that will receive kLogMessage and kBridgeStoppedMessage.
     */
    explicit BridgeController(HWND hwnd) : hwnd_(hwnd) {}

    /// Stop the session and release all resources. Safe to call if never started.
    ~BridgeController() { Stop(); }

    /**
     * @brief Start a bridge session with the given configuration.
     *
     * Initialises WinSock, loads the control database, opens and handshakes
     * all COM ports, sets up the import socket, and starts the worker thread.
     *
     * @param config  Session configuration (COM ports, UDP/TCP mode, logging flags, …).
     * @return True on success; false if WSAStartup fails, no ports could be opened,
     *         or the session is already running.
     */
    bool Start(const BridgeConfig& config) {
        if (running_) return false;

        if (!config.dryRun && config.comPorts.empty()) {
            PostLog(hwnd_, L"No COM ports specified.");
            return false;
        }

        config_ = config;

        // Load control reference JSON (optional — failure is non-fatal)
        std::string jsonDir = config.jsonDir.empty() ? FindJsonDir() : config.jsonDir;
        if (!jsonDir.empty()) {
            size_t n = controlDb_.load(jsonDir, "FA-18C_hornet");
            loadedControlCount_ = n;
            if (n > 0) {
                std::wstringstream m;
                m << L"Control database (FA-18C_hornet): " << n << L" controls loaded.";
                PostLog(hwnd_, m.str());
            } else {
                PostLog(hwnd_, L"Control database: no JSON found (state-change labels disabled).");
            }
        } else {
            loadedControlCount_ = 0;
            PostLog(hwnd_, L"Control database path not found (state-change labels disabled).");
        }

        logStateChanges_ = config.logStateChanges;
        logStateChangesPrimed_ = false;
        lastLoggedValues_.clear();
        logStateChangesArmedAt_ = std::chrono::steady_clock::now() + kLogStartupSettleWindow;
        logStateChangesArmedNoticeSent_ = false;
        // Initialise live-update atomics from the startup config
        liveLogChanges_.store(config.logStateChanges,   std::memory_order_relaxed);
        liveLogRawKnobs_.store(config.logRawKnobsDials, std::memory_order_relaxed);
        liveLogRawGauges_.store(config.logRawGauges,    std::memory_order_relaxed);
        liveLogDiagnostics_.store(config.logDiagnostics, std::memory_order_relaxed);

        currentMode_ = config.initialMode;

        // Create the sim source based on the operator's selection
        switch (config.sourceType) {
        case SimSourceType::DcsDirect:
            source_ = std::make_unique<DcsDirectSource>();
            break;
        case SimSourceType::DcsBiosTcp:
            source_ = std::make_unique<DcsBiosSource>(DcsBiosSource::Mode::Tcp);
            break;
        case SimSourceType::ReplayFile:
            source_ = std::make_unique<ReplayFileSource>(config.replayFilePath);
            break;
        case SimSourceType::Msfs:
            source_ = std::make_unique<MsfsSource>();
            break;
        case SimSourceType::DcsBiosUdp:
        default:
            source_ = std::make_unique<DcsBiosSource>(DcsBiosSource::Mode::Udp);
            break;
        }

        // Wire the frame-sync callback
        source_->onFrameSync = [this](const std::vector<uint16_t>& dirty) {
            OnFrameSync(dirty);
        };

        if (!OpenSerialPorts()) {
            source_.reset();
            return false;
        }

        if (!source_->connect(stateMap_, hwnd_)) {
            source_.reset();
            CleanupResources();
            return false;
        }

        running_ = true;
        startedAt_ = std::chrono::steady_clock::now();
        firstFrameNoticeSent_ = false;
        frameCounter_ = 0;
        dispatchFramesWithWrites_ = 0;
        dispatchMetrics_.reset();
        portWriteMetrics_.reset();
        PostStartupReadinessSummary();
        PostLog(hwnd_, L"Bridge started.");
        return true;
    }

    /**
     * @brief Stop the bridge session and release all resources.
     *
     * Signals the worker thread to exit, joins all threads, closes all COM
     * port handles, and shuts down WinSock. Safe to call multiple times and
     * from the destructor.
     */
    void Stop() {
        bool hadWork = running_ || !ports_.empty();
        running_ = false;
        if (source_) { source_->disconnect(); source_.reset(); }
        for (auto& serialPort : ports_) {
            serialPort->readRunning = false;
            if (serialPort->readThread.joinable()) serialPort->readThread.join();
        }
        CleanupResources();
        if (hadWork) PostLog(hwnd_, L"Bridge stopped.");
    }

    /// @return True when the worker thread is actively running.
    bool IsRunning() const { return running_; }

    /**
     * @brief Update per-channel logging flags while the session is running.
     *
     * Safe to call from the UI thread at any time. Each flag is stored in a
     * separate atomic so the worker thread sees the change on its next frame
     * without any locking.
     *
     * @param logChanges      Emit control state-change lines.
     * @param logRawKnobs     Include high-range knob/dial outputs in change lines.
     * @param logRawGauges    Include read-only gauge outputs in change lines.
     * @param logDiagnostics  Emit transport diagnostics (heartbeats, reconnects).
     */
    void SetLoggingFlags(bool logChanges, bool logRawKnobs, bool logRawGauges, bool logDiagnostics) {
        liveLogChanges_.store(logChanges,    std::memory_order_relaxed);
        liveLogRawKnobs_.store(logRawKnobs,  std::memory_order_relaxed);
        liveLogRawGauges_.store(logRawGauges, std::memory_order_relaxed);
        liveLogDiagnostics_.store(logDiagnostics, std::memory_order_relaxed);
    }

    /**
     * @brief Change the hub operating mode and broadcast to all connected devices.
     *
     * Sends the mode frame `0xAA 0xDE 0xAD 0x04 [mode]` to every open serial
     * port.  In Preflight and Maintenance modes, import commands from hardware
     * are captured but NOT forwarded to the simulator.
     *
     * Safe to call from the UI thread while the bridge is running.
     *
     * @param mode  New operating mode.
     */
    void SetMode(BridgeMode mode) {
        currentMode_ = mode;
        BroadcastModeFrame(mode);
        std::wstring modeStr;
        switch (mode) {
        case BridgeMode::Sim:         modeStr = L"Sim";         break;
        case BridgeMode::Preflight:   modeStr = L"Preflight";   break;
        case BridgeMode::Maintenance: modeStr = L"Maintenance"; break;
        }
        PostLog(hwnd_, L"Hub mode set to: " + modeStr);
    }

    /// @return Current operating mode (may be called from the UI thread).
    BridgeMode GetMode() const { return currentMode_; }

    /**
     * @brief Return a snapshot of the current dispatch timing metrics.
     *
     * Safe to call from the UI thread while the worker thread is running —
     * each field is read from a separate atomic variable.
     */
    DispatchMetricsSnapshot GetDispatchMetrics() const {
        return DispatchMetricsSnapshot{
            dispatchMetrics_.count.load(),
            dispatchFramesWithWrites_.load(),
            dispatchMetrics_.mean.load(),
            dispatchMetrics_.max.load(),
            portWriteMetrics_.mean.load(),
            portWriteMetrics_.max.load(),
        };
    }

    /**
     * @brief Run an in-process self-test using synthetic protocol data.
     *
     * @details
     * Exercises the ExportParser (sync detection, write record application,
     * dirty-word tracking) and ImportLineParser (line reassembly, field
     * parsing) with hardcoded byte sequences. Results are posted to the log
     * as `PASS` / `FAIL` lines. Runs synchronously on the calling thread.
     */
    void RunSelfTest() {
        PostLog(hwnd_, L"--- Self-Test Begin ---");

        BiosStateMap testMap;
        dcsbios::ExportParser testParser(testMap);
        int syncCount = 0;
        testParser.onFrameSync = [&]() { ++syncCount; };

        // Synthetic stream: sync + write addr=0x0100 len=4 (two words).
        // A UDP datagram contains one full frame, so flushFrame() completes it.
        const uint8_t stream[] = {
            0x55, 0x55, 0x55, 0x55,
            0x00, 0x01,  // addr = 0x0100
            0x04, 0x00,  // len  = 4 bytes
            0x01, 0x00,  // word @ 0x0100 = 1
            0x02, 0x00,  // word @ 0x0102 = 2
        };
        testParser.processBytes(stream, sizeof(stream));
        testParser.flushFrame();
        auto dirty = testMap.takeDirty();

        bool pass = true;
        auto fail = [&](const wchar_t* msg) { PostLog(hwnd_, msg); pass = false; };
        if (syncCount < 1)                      fail(L"  FAIL: no sync frame detected.");
        if (dirty.size() != 2)                  fail(L"  FAIL: expected 2 dirty words.");
        if (testMap.readWord(0x0100) != 0x0001) fail(L"  FAIL: word at 0x0100 wrong.");
        if (testMap.readWord(0x0102) != 0x0002) fail(L"  FAIL: word at 0x0102 wrong.");

        // Test import line parser
        ImportLineParser lp;
        ImportCommand got;
        bool gotCmd = false;
        lp.onCommand = [&](const ImportCommand& c) { got = c; gotCmd = true; };
        const uint8_t line[] = "UFC_COMM1_CHANNEL_SELECT SET_STATE 3\n";
        lp.processBytes(line, sizeof(line) - 1);
        if (!gotCmd || got.identifier != "UFC_COMM1_CHANNEL_SELECT"
                    || got.action     != "SET_STATE"
                    || got.value      != "3") {
            fail(L"  FAIL: import line parser produced wrong result.");
        }

        PostLog(hwnd_, pass ? L"  PASS: all checks passed." : L"  Some checks FAILED — see above.");
        PostLog(hwnd_, L"--- Self-Test End ---");
    }

private:
    // ── Per-device record ────────────────────────────────────────────────────

    /**
     * @brief Runtime record for one open COM port connection.
     *
     * Owns the Win32 HANDLE, the negotiated device metadata, the optional
     * read thread for bidirectional ports, and the import-line parser.
     * Non-copyable because of the Win32 HANDLE and std::thread members.
     */
    struct SerialPort {
        int              comPort    = 0;                      ///< COM port number (e.g. 10 for COM10)
        HANDLE           handle     = INVALID_HANDLE_VALUE;   ///< Win32 file handle to the COM port
        DeviceInfo       info;                                 ///< Negotiated device metadata
        std::thread      readThread;                           ///< Read thread (bidir ports only)
        ImportLineParser importParser;                         ///< Accumulates import command lines
        std::atomic<bool> readRunning{false};                  ///< Set to false to signal the read thread to exit

        SerialPort()                             = default;
        SerialPort(const SerialPort&)            = delete;
        SerialPort& operator=(const SerialPort&) = delete;
    };

    // ── Open serial ports + handshake ────────────────────────────────────────

    /**
     * @brief Open and configure all COM ports listed in config_.comPorts.
     *
     * For each port: CreateFileW → ConfigureSerialPort → PerformHandshake.
     * Ports that fail to open or configure are skipped with a log message.
     * If the device reports bidirectional capability a ReadDeviceThread is started.
     *
     * @return True if at least one port was opened successfully; false otherwise.
     */
    bool OpenSerialPorts() {
        if (config_.dryRun) {
            PostLog(hwnd_, L"Dry-run: serial writes disabled (state machine active).");
            return true;
        }
        bool openedAny = false;
        for (int comPort : config_.comPorts) {
            wchar_t path[32] = {};
            swprintf_s(path, L"\\\\.\\COM%d", comPort);
            HANDLE portHandle = CreateFileW(path, GENERIC_READ | GENERIC_WRITE,
                                   0, nullptr, OPEN_EXISTING, 0, nullptr);
            if (portHandle == INVALID_HANDLE_VALUE) {
                std::wstringstream m;
                m << L"Failed to open COM" << comPort << L" (err " << GetLastError() << L").";
                PostLog(hwnd_, m.str());
                continue;
            }
            if (!ConfigureSerialPort(portHandle)) {
                std::wstringstream m;
                m << L"Failed to configure COM" << comPort << L".";
                PostLog(hwnd_, m.str());
                CloseHandle(portHandle);
                continue;
            }
            auto newPort = std::make_unique<SerialPort>();
            newPort->comPort      = comPort;
            newPort->handle       = portHandle;
            newPort->info.comPort = "COM" + std::to_string(comPort);

            PerformHandshake(*newPort);

            if (newPort->info.bidir) {
                newPort->readRunning = true;
                SerialPort* rawPtr   = newPort.get();
                newPort->readThread  = std::thread(&BridgeController::ReadDeviceThread, this, rawPtr);
            }
            openedAny = true;
            ports_.push_back(std::move(newPort));
        }
        if (!openedAny) PostLog(hwnd_, L"No COM ports could be opened.");
        return openedAny;
    }

    // ── Handshake ───────────────────────────────────────────────────────────

    /**
     * @brief Perform the 3-way capability handshake with the device on @p serialPort.
     *
     * @details
     * Sends the 4-byte ping frame, then reads bytes with a 300 ms deadline
     * into a HandshakeParser. On success: populates serialPort.info and sends
     * the 4-byte ack frame. On timeout or parse failure: classifies the port
     * as Legacy (full unfiltered stream, no bidirectional support).
     *
     * Serial timeouts are temporarily tightened during the handshake window
     * and restored to normal read-interval mode afterwards.
     *
     * @param serialPort  The port record to interrogate; its info field is
     *                    populated in place on return.
     */
    void PerformHandshake(SerialPort& serialPort) {
        auto ping = HandshakeParser::pingFrame();
        DWORD written = 0;
        WriteFile(serialPort.handle, ping.data(), static_cast<DWORD>(ping.size()), &written, nullptr);

        // Tighten timeouts for the handshake window to avoid stalling the
        // open sequence for the full kHandshakeTimeout on a legacy device.
        COMMTIMEOUTS ct = {};
        ct.ReadTotalTimeoutConstant = 300;
        ct.ReadIntervalTimeout      = 50;
        SetCommTimeouts(serialPort.handle, &ct);

        HandshakeParser parser;
        uint8_t b = 0;
        DWORD bytesRead = 0;
        bool completed = false;
        auto deadline = std::chrono::steady_clock::now() + dcsbios::kHandshakeTimeout;

        while (std::chrono::steady_clock::now() < deadline && running_) {
            bytesRead = 0;
            if (ReadFile(serialPort.handle, &b, 1, &bytesRead, nullptr) && bytesRead == 1) {
                auto r = parser.processByte(b);
                if (r == HandshakeParser::Result::Complete) { completed = true; break; }
                if (r == HandshakeParser::Result::Failed)   break;
            }
        }
        ConfigureSerialPort(serialPort.handle);  // restore normal read-interval timeouts

        if (completed) {
            parser.populateDevice(serialPort.info);
            auto ack = HandshakeParser::ackFrame();
            WriteFile(serialPort.handle, ack.data(), static_cast<DWORD>(ack.size()), &written, nullptr);

            std::wstring name(serialPort.info.deviceName.begin(), serialPort.info.deviceName.end());
            std::wstringstream m;
            m << L"COM" << serialPort.comPort << L": ";
            switch (serialPort.info.role) {
            case DeviceRole::RS485Master: m << L"RS-485 Master"; break;
            case DeviceRole::RS485Slave:  m << L"RS-485 Slave";  break;
            default:                      m << L"Standalone";     break;
            }
            if (!name.empty())              m << L" \"" << name << L"\"";
            if (serialPort.info.bidir)      m << L" [bidir]";
            if (serialPort.info.wantsAll)   m << L" [all channels]";
            else m << L" [" << serialPort.info.subAddrs.size() << L" subscriptions]";
            PostLog(hwnd_, m.str());
        } else {
            // No valid response within the handshake window — treat as legacy.
            serialPort.info.role          = DeviceRole::Legacy;
            serialPort.info.handshakeDone = true;
            serialPort.info.wantsAll      = true;
            serialPort.info.bidir         = false;
            std::wstringstream m;
            m << L"COM" << serialPort.comPort << L": Legacy (full stream, no handshake).";
            PostLog(hwnd_, m.str());
        }
    }

    void CloseSerialPorts() {
        for (auto& serialPort : ports_) {
            serialPort->readRunning = false;
            if (serialPort->readThread.joinable()) serialPort->readThread.join();
            if (serialPort->handle != INVALID_HANDLE_VALUE) {
                CloseHandle(serialPort->handle);
                serialPort->handle = INVALID_HANDLE_VALUE;
            }
        }
        ports_.clear();
    }

    // ── Import socket (device → DCS) — delegated to ISimSource ──────────────

    void SendImportCommand(const ImportCommand& cmd) {
        // In Preflight/Maintenance mode: capture import commands but do NOT
        // forward to the simulator.  Full state-store routing is Phase 5.
        if (currentMode_ != BridgeMode::Sim) return;
        if (source_) source_->sendImport(cmd);
    }

    // ── Per-device read thread (bidirectional) ───────────────────────────────

    /**
     * @brief Entry point for the read thread attached to a bidirectional device.
     *
     * @details
     * Runs until @p serialPort->readRunning is set to false. Reads raw bytes
     * from the COM port with a 100 ms inter-character timeout, feeds them
     * into the ImportLineParser, and forwards completed commands to DCS via
     * SendImportCommand().
     *
     * Each forwarded command is also posted to the UI log (→ DCS direction).
     *
     * @param serialPort  Pointer to the owning SerialPort record. Lifetime is
     *                    guaranteed to outlast the thread (Stop() joins before
     *                    destroying the record).
     */
    void ReadDeviceThread(SerialPort* serialPort) {
        COMMTIMEOUTS ct = {};
        ct.ReadIntervalTimeout        = 100;
        ct.ReadTotalTimeoutMultiplier = 0;
        ct.ReadTotalTimeoutConstant   = 100;
        SetCommTimeouts(serialPort->handle, &ct);

        serialPort->importParser.onCommand = [this, serialPort](const ImportCommand& cmd) {
            SendImportCommand(cmd);
            std::wstring id(cmd.identifier.begin(), cmd.identifier.end());
            std::wstring act(cmd.action.begin(), cmd.action.end());
            std::wstring val(cmd.value.begin(), cmd.value.end());
            std::wstringstream m;
            m << L"COM" << serialPort->comPort << L" \u2192 DCS: " << id << L" " << act;
            if (!val.empty()) m << L" " << val;
            PostLog(hwnd_, m.str());
        };

        std::vector<uint8_t> readBuf(dcsbios::kImportLineMaxBytes);
        while (serialPort->readRunning) {
            DWORD bytesRead = 0;
            if (ReadFile(serialPort->handle, readBuf.data(),
                         static_cast<DWORD>(readBuf.size()), &bytesRead, nullptr) && bytesRead > 0)
                serialPort->importParser.processBytes(readBuf.data(), bytesRead);
        }
    }

    void CleanupResources() {
        std::lock_guard<std::mutex> lock(resourceMutex_);
        CloseSerialPorts();
    }

    // ── Mode broadcast ────────────────────────────────────────────────────────

    /**
     * @brief Send the mode frame `0xAA 0xDE 0xAD 0x04 [mode]` to all devices.
     *
     * Each connected serial port receives the frame.  Devices that do not
     * understand the mode frame will ignore the unrecognised bytes (legacy
     * behaviour is gracefully preserved because legacy devices do not
     * implement a command parser).
     *
     * @param mode  Hub mode to broadcast.
     */
    void BroadcastModeFrame(BridgeMode mode) {
        const uint8_t frame[] = {
            0xAA, 0xDE, 0xAD, 0x04,
            static_cast<uint8_t>(mode)
        };
        for (auto& sp : ports_) {
            if (sp->handle == INVALID_HANDLE_VALUE) continue;
            DWORD written = 0;
            WriteFile(sp->handle, frame, sizeof(frame), &written, nullptr);
        }
    }

    // ── Import socket (device → DCS) — now delegated to ISimSource ───────────
    // (kept as stub for backward-compat call sites during refactor)

    /**
     * @brief Called by ExportParser after every complete DCS-BIOS export frame.
     *
     * Orchestrates three sequential operations per frame:
     *  1. Dispatch delta frames to all subscribed serial devices.
     *  2. Update running dispatch and per-port write latency metrics.
     *  3. Emit state-change log lines for controls whose value changed.
     */
    void OnFrameSync(const std::vector<uint16_t>& dirty) {
        ++frameCounter_;
        uint64_t fc = frameCounter_.load();

        if (!firstFrameNoticeSent_.exchange(true)) {
            auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startedAt_).count();
            std::wstringstream firstFrameMsg;
            firstFrameMsg << L"Source data check: first export frame received after "
                          << elapsedMs << L" ms.";
            PostLog(hwnd_, firstFrameMsg.str());
        }

        uint64_t portWritesThisFrame = DispatchDeltaFrames(dirty);
        UpdateDispatchMetrics(portWritesThisFrame);
        LogStateChanges(dirty);

        // Emit a progress heartbeat every 300 frames (~10 s at 30 fps),
        // but only when the Transport Diagnostics channel is enabled.
        if (liveLogDiagnostics_.load(std::memory_order_relaxed)) {
            if (fc == 1 || fc % 300 == 0) {
                std::wstringstream m;
                m << (config_.dryRun ? L"Dry-run: " : L"Bridge: ")
                  << fc << L" frames " << (config_.dryRun ? L"processed." : L"dispatched.");
                PostLog(hwnd_, m.str());
            }
        }
    }

    /**
     * @brief Send a delta frame to each device that subscribed to any of the dirty addresses.
     *
     * @param dirty  List of even byte addresses whose values changed this frame.
     * @return Number of serial port write operations performed.
     */
    uint64_t DispatchDeltaFrames(const std::vector<uint16_t>& dirty) {
        uint64_t portWritesThisFrame = 0;
        if (config_.dryRun) return 0;

        for (auto& serialPort : ports_) {
            if (serialPort->handle == INVALID_HANDLE_VALUE) continue;
            auto frame = BuildDeltaFrame(stateMap_, dirty, serialPort->info);
            if (frame.empty()) continue;

            auto writeStartedAt = std::chrono::steady_clock::now();
            DWORD written = 0;
            BOOL ok = WriteFile(serialPort->handle, frame.data(),
                                static_cast<DWORD>(frame.size()), &written, nullptr);
            auto writeFinishedAt = std::chrono::steady_clock::now();

            uint64_t writeUs = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    writeFinishedAt - writeStartedAt).count());
            ++portWritesThisFrame;
            portWriteMetrics_.update(writeUs);

            if (!ok || written != static_cast<DWORD>(frame.size())) {
                std::wstringstream m;
                m << L"Write failed on COM" << serialPort->comPort << L".";
                PostLog(hwnd_, m.str());
            }
        }
        return portWritesThisFrame;
    }

    /**
     * @brief Update the running dispatch latency metrics for this frame.
     *
     * @details
     * Records total dispatch wall-clock time since the beginning of
     * OnFrameSync() (i.e. including all serial writes for this frame) using
     * the OnlineAverage tracker. The dispatch start time is captured at the
     * top of UpdateDispatchMetrics() itself — this is fine because
     * DispatchDeltaFrames() is called synchronously before this method.
     *
     * @param portWritesThisFrame  Number of serial write calls made this frame.
     */
    void UpdateDispatchMetrics(uint64_t portWritesThisFrame) {
        // The dispatch timer starts when this method is called.  Because
        // DispatchDeltaFrames() runs synchronously before this call, its
        // contribution is already baked into the elapsed time we see here.
        // This is intentional — we want to measure total frame processing
        // latency, not just the time after writes complete.
        auto now = std::chrono::steady_clock::now();
        // Sentinel value: metrics only start after the first frame so the
        // initial large gap (WSA startup → first frame) is excluded.
        static thread_local std::chrono::steady_clock::time_point frameStart;
        static thread_local bool firstFrame = true;
        if (firstFrame) {
            frameStart = now;
            firstFrame = false;
        }
        uint64_t frameDispatchUs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(now - frameStart).count());
        frameStart = now;
        dispatchMetrics_.update(frameDispatchUs);

        if (portWritesThisFrame > 0)
            ++dispatchFramesWithWrites_;
    }

    /**
     * @brief Emit operator log lines for controls whose value changed this frame.
     *
     * @details
     * ### Startup settle window
     * DCS emits a large burst of initialisation writes when a mission loads.
     * To avoid flooding the log with these transient values, state-change
     * logging is suppressed for @ref kLogStartupSettleWindow seconds after
     * Start() is called. When the window expires the function seeds a
     * baseline snapshot of every tracked control so the first real changes
     * emit clean deltas rather than "everything changed at once" noise.
     *
     * ### Per-frame emission cap
     * At most 20 change lines are emitted per frame. If more controls changed
     * a summary `"... and N more changes."` line is appended instead.
     *
     * @param dirty  List of even byte addresses whose values changed this frame.
     */
    void LogStateChanges(const std::vector<uint16_t>& dirty) {
        if (!liveLogChanges_.load(std::memory_order_relaxed) || controlDb_.empty()) return;

        bool logRawKnobs  = liveLogRawKnobs_.load(std::memory_order_relaxed);
        bool logRawGauges = liveLogRawGauges_.load(std::memory_order_relaxed);

        bool loggingArmed = std::chrono::steady_clock::now() >= logStateChangesArmedAt_;
        bool seededBaselineThisFrame = false;

        if (loggingArmed && !logStateChangesArmedNoticeSent_) {
            // Seed every eligible control from the fully-settled state map before
            // emitting runtime changes. Without this, the first activity on a shared
            // address word can dump every sibling control on that word because those
            // identifiers have no baseline yet.
            lastLoggedValues_.clear();
            controlDb_.forEachControl([&](const ControlDescriptor& desc) {
                if (!ShouldLogStateChange(desc, logRawKnobs, logRawGauges))
                    return;
                lastLoggedValues_[desc.identifier] = ReadControlValue(desc, stateMap_);
            });
            logStateChangesPrimed_    = true;
            seededBaselineThisFrame   = true;
            logStateChangesArmedNoticeSent_ = true;
            PostLog(hwnd_, L"State-change logging armed (startup settle complete).");
        }

        if (dirty.empty()) return;

        std::unordered_set<std::string> seenIds;
        size_t changedCount = 0;
        size_t emittedCount = 0;

        for (uint16_t addr : dirty) {
            const auto* list = controlDb_.lookupByAddr(addr);
            if (!list) continue;

            for (const auto* desc : *list) {
                if (!seenIds.insert(desc->identifier).second) continue;
                if (!ShouldLogStateChange(*desc, logRawKnobs, logRawGauges))
                    continue;

                uint32_t value = ReadControlValue(*desc, stateMap_);
                auto [it, inserted] = lastLoggedValues_.emplace(desc->identifier, value);

                if (!logStateChangesPrimed_) {
                    // Not yet primed — accumulate baseline silently.
                    it->second = value;
                    continue;
                }

                // Suppress startup churn: refresh the cached baseline until
                // logging is armed. Also skip the exact frame where we seeded
                // the settled baseline so the arm transition itself is silent.
                if (!loggingArmed || seededBaselineThisFrame) {
                    it->second = value;
                    continue;
                }

                // New identifier — record its initial value without logging.
                if (inserted) {
                    it->second = value;
                    continue;
                }

                // No change — nothing to log.
                if (it->second == value) continue;

                it->second = value;
                ++changedCount;
                if (emittedCount < 20) {
                    PostLog(hwnd_, FormatWireStateChange(*desc, stateMap_));
                    ++emittedCount;
                }
            }
        }

        if (!logStateChangesPrimed_) {
            logStateChangesPrimed_ = true;
        } else if (changedCount > emittedCount) {
            std::wstringstream m;
            m << L"  ... and " << (changedCount - emittedCount) << L" more changes.";
            PostLog(hwnd_, m.str());
        }
    }

    // ── Main receive loop ────────────────────────────────────────────────────

    /**
     * @brief Worker-thread main loop: receive export data and drive the parser.
     *
     * @details
     * Runs on the worker thread started by Start(). When `socket_` is invalid
     * (first run or after a disconnection) it calls the appropriate socket
     * initialiser and retries with exponential backoff (1 s → 5 s cap).
     *
     * Receive timeouts (@ref kSocketReceiveTimeoutMs) are treated as normal
     * idle periods — the loop simply checks `running_` and waits again.
     * Any other socket error triggers a reconnect cycle.
     *
     * For UDP mode, `flushFrame()` is called after each datagram so the parser
     * treats each datagram as a complete frame regardless of sync-byte boundary.
     *
     * On exit the socket is closed, CleanupResources() is called, and
     * `kBridgeStoppedMessage` is posted to the UI window.
     */
    // ── Members ──────────────────────────────────────────────────────────────

    const wchar_t* SourceTypeToText(SimSourceType sourceType) const {
        switch (sourceType) {
        case SimSourceType::DcsDirect:  return L"DCS Lua direct (127.0.0.1:42002)";
        case SimSourceType::DcsBiosUdp: return L"DCS-BIOS UDP multicast (239.255.50.10:5010)";
        case SimSourceType::DcsBiosTcp: return L"DCS-BIOS TCP (127.0.0.1:7778)";
        case SimSourceType::ReplayFile: return L"Replay file";
        case SimSourceType::Msfs:       return L"MSFS (stub)";
        default:                        return L"Unknown";
        }
    }

    void PostStartupReadinessSummary() {
        size_t bidirCount = 0;
        size_t legacyCount = 0;
        size_t masterCount = 0;
        size_t slaveCount = 0;
        size_t downstreamSlaveCount = 0;

        for (const auto& serialPort : ports_) {
            if (!serialPort) continue;
            if (serialPort->info.bidir) ++bidirCount;

            switch (serialPort->info.role) {
            case DeviceRole::Legacy:
                ++legacyCount;
                break;
            case DeviceRole::RS485Master:
                ++masterCount;
                downstreamSlaveCount += serialPort->info.slaves.size();
                break;
            case DeviceRole::RS485Slave:
                ++slaveCount;
                break;
            default:
                break;
            }
        }

        PostLog(hwnd_, L"=== Startup Readiness ===");

        std::wstringstream sourceLine;
        sourceLine << L"Source: " << SourceTypeToText(config_.sourceType)
                   << L" [connected=" << (source_ && source_->isConnected() ? L"yes" : L"no") << L"]";
        PostLog(hwnd_, sourceLine.str());

        if (config_.sourceType == SimSourceType::DcsDirect) {
            PostLog(hwnd_, L"Source data check: waiting for first Lua export frame...");
        } else if (config_.sourceType == SimSourceType::DcsBiosUdp ||
                   config_.sourceType == SimSourceType::DcsBiosTcp) {
            PostLog(hwnd_, L"Source data check: waiting for first DCS-BIOS frame...");
        }

        std::wstringstream controlsLine;
        controlsLine << L"Control database: " << loadedControlCount_
                     << L" controls loaded";
        PostLog(hwnd_, controlsLine.str());

        std::wstringstream comLine;
        comLine << L"COM ports: opened " << ports_.size()
                << L" / requested " << config_.comPorts.size()
                << L" [bidir=" << bidirCount << L", legacy=" << legacyCount << L"]";
        PostLog(hwnd_, comLine.str());

        std::wstringstream rs485Line;
        rs485Line << L"RS-485 topology: masters=" << masterCount
                  << L", slaves=" << slaveCount
                  << L", downstream slaves declared=" << downstreamSlaveCount;
        PostLog(hwnd_, rs485Line.str());

        PostLog(hwnd_, L"=========================");
    }

    HWND  hwnd_                 = nullptr;         ///< Owner window for PostLog() and PostMessage()
    BridgeConfig                config_;           ///< Current run configuration snapshot
    BiosStateMap                stateMap_;         ///< 64 KB live DCS cockpit state
    ControlDatabase             controlDb_;        ///< Address→descriptor lookup table
    std::unique_ptr<ISimSource> source_;           ///< Active simulator data source
    std::vector<std::unique_ptr<SerialPort>> ports_; ///< Open serial ports managed by this controller
    std::atomic<bool>           running_{false};   ///< Session-active flag; set to false by Stop()
    std::mutex                  resourceMutex_;    ///< Serialises CleanupResources()
    std::atomic<uint64_t>       frameCounter_{0};                 ///< Total frames received since Start()
    std::atomic<uint64_t>       dispatchFramesWithWrites_{0};     ///< Frames where at least one port write occurred
    OnlineAverage               dispatchMetrics_;   ///< Frame-level dispatch wall-clock latency
    OnlineAverage               portWriteMetrics_;  ///< Per-port serial WriteFile() latency
    bool                        logStateChanges_ = false;         ///< User option: emit per-frame change lines
    bool                        logStateChangesPrimed_ = false;   ///< True after initial baseline is captured
    std::chrono::steady_clock::time_point logStateChangesArmedAt_ = std::chrono::steady_clock::now(); ///< When settle window expires
    bool                        logStateChangesArmedNoticeSent_ = false; ///< Prevents repeat arm-notice logs
    std::unordered_map<std::string, uint32_t> lastLoggedValues_; ///< Previous value cache for delta detection
    BridgeMode                  currentMode_{BridgeMode::Sim};    ///< Current hub operating mode
    size_t                      loadedControlCount_ = 0;          ///< Number of control descriptors loaded at Start()
    std::chrono::steady_clock::time_point startedAt_ = std::chrono::steady_clock::now(); ///< Session start time
    std::atomic<bool>           firstFrameNoticeSent_{false};     ///< Ensures first-frame readiness line is emitted once

    // Live-updatable logging flags (written by UI thread, read by worker thread)
    std::atomic<bool> liveLogChanges_{false};     ///< Current effective state of logStateChanges_
    std::atomic<bool> liveLogRawKnobs_{true};     ///< Current effective state of logRawKnobsDials
    std::atomic<bool> liveLogRawGauges_{false};   ///< Current effective state of logRawGauges
    std::atomic<bool> liveLogDiagnostics_{false}; ///< Transport diagnostics channel (heartbeats, reconnects)
};

// ─── UI state ─────────────────────────────────────────────────────────────────

/**
 * @brief All Win32 window handles and runtime state for the main UI window.
 *
 * Owned by WinMain() and passed to WndProc() via SetWindowLongPtr/GWLP_USERDATA.
 * All members are accessed from the UI thread only, except where explicitly noted.
 */
struct UiState {
    // Window handles
    HWND sourceLabel       = nullptr; ///< "Source:" static label
    HWND portsLabel        = nullptr; ///< "COM Ports:" static label
    HWND hintLabel         = nullptr; ///< Usage hint label below the port field
    HWND sourceCombo       = nullptr; ///< Sim source selector combo box
    HWND portsEdit         = nullptr; ///< COM port range text field (e.g. "3,5-7")
    HWND autoDetectButton  = nullptr; ///< "Auto-detect" button
    HWND toggleButton      = nullptr; ///< Start / Stop button
    HWND statusText        = nullptr; ///< Current bridge state label
    HWND dryRunCheckbox    = nullptr; ///< "Dry run (no serial writes)" checkbox
    HWND logChangesCheckbox= nullptr; ///< "Log state changes" checkbox
    HWND rawKnobsCheckbox  = nullptr; ///< "Log raw knobs/dials" checkbox
    HWND rawGaugesCheckbox = nullptr; ///< "Log raw gauges" checkbox
    HWND exportLogButton   = nullptr; ///< "Export Log…" button
    HWND selfTestButton    = nullptr; ///< "Self-test" button
    HWND logDiagnosticsCheckbox = nullptr; ///< "Log Diagnostics" channel toggle
    HWND captureButton     = nullptr; ///< "Start Capture" / "Stop Capture" stream-to-disk button
    HWND modeSimBtn        = nullptr; ///< "Sim" mode toolbar button
    HWND modePreflightBtn  = nullptr; ///< "Preflight" mode toolbar button
    HWND modeMainBtn       = nullptr; ///< "Maintenance" mode toolbar button
    HWND logText           = nullptr; ///< Scrolling read-only log edit control

    // GDI resources
    HFONT  uiFont          = nullptr; ///< Segoe UI 9pt font for all controls
    HBRUSH bgBrush         = nullptr; ///< Window background colour brush
    HBRUSH panelBrush      = nullptr; ///< Panel / groupbox background brush
    HBRUSH inputBrush      = nullptr; ///< Edit control background brush
    HBRUSH buttonBrush     = nullptr; ///< Button background brush

    // Runtime state
    bool activeDryRun         = false; ///< Whether the current run is a dry run
    std::wstring statusPrefix = L"Stopped"; ///< Prefix for the status label

    // Log buffer (UI-thread only)
    std::wstring logBuffer;            ///< Full accumulated log text in the edit control
    std::wstring pendingUiLog;         ///< Lines received but not yet flushed to the control
    bool logFlushScheduled = false;    ///< True while a WM_TIMER flush is pending
    size_t pendingUiLineCount = 0;     ///< Number of lines in pendingUiLog

    // Log flow metrics (UI-thread only)
    uint64_t droppedUiLines       = 0;   ///< Lines dropped due to queue overload
    uint64_t linesReceivedWindow  = 0;   ///< Lines received in the current metrics window
    uint64_t linesRenderedWindow  = 0;   ///< Lines rendered in the current metrics window
    double   linesReceivedPerSec  = 0.0; ///< Smoothed receive rate
    double   linesRenderedPerSec  = 0.0; ///< Smoothed render rate
    double   avgFlushMs           = 0.0; ///< Running average UI flush duration
    double   maxFlushMs           = 0.0; ///< Peak UI flush duration since last reset
    uint64_t flushCount           = 0;   ///< Total number of log flushes performed
    bool     queueOverloadNoticeShown = false; ///< Prevents repeated overload warnings
    std::chrono::steady_clock::time_point metricsWindowStartedAt = std::chrono::steady_clock::now(); ///< Window start for rate calc

    // Owned controller
    std::unique_ptr<BridgeController> controller; ///< Active bridge controller, or null if stopped
    StartupOptions startupOptions;                 ///< Parsed command-line options
    std::wofstream captureStream;                  ///< Open when stream-to-disk capture is active
};

// ─── Layout ───────────────────────────────────────────────────────────────────

/**
 * @brief Reposition all controls to fill the client area after a resize.
 *
 * Called on WM_SIZE. Uses a fixed left margin, label column, and row height;
 * the log edit control fills the remaining vertical space.
 */
void LayoutControls(UiState* state, int W, int H) {
    if (!state) return;
    const int margin = 16, gap = 8, rowH = 28;
    const int labelW = 62, modeW = 310, buttonW = 110, startW = 110, modeBtnW = 90;

    int y = margin;
    MoveWindow(state->sourceLabel, margin, y + 4, labelW, 20, TRUE);
    MoveWindow(state->sourceCombo, margin + labelW + 4, y, modeW, rowH + 220, TRUE);
    // Mode toolbar buttons (right-aligned on source row)
    int modeBtnX = W - margin - modeBtnW * 3 - gap * 2;
    MoveWindow(state->modeSimBtn,       modeBtnX,                         y, modeBtnW, rowH, TRUE);
    MoveWindow(state->modePreflightBtn, modeBtnX + modeBtnW + gap,        y, modeBtnW, rowH, TRUE);
    MoveWindow(state->modeMainBtn,      modeBtnX + (modeBtnW + gap) * 2,  y, modeBtnW, rowH, TRUE);

    y += rowH + gap;
    int portsX = margin + labelW + 4;
    int rightW  = (buttonW + gap) + startW;
    int portsW  = std::max(120, W - portsX - margin - rightW - gap);
    MoveWindow(state->portsLabel,       margin,                        y + 4, labelW + 10, 20,   TRUE);
    MoveWindow(state->portsEdit,        portsX,                        y,     portsW,      rowH,  TRUE);
    MoveWindow(state->autoDetectButton, portsX + portsW + gap,         y,     buttonW,     rowH,  TRUE);
    MoveWindow(state->toggleButton,     portsX + portsW + gap*2 + buttonW, y, startW,      rowH,  TRUE);

    y += rowH + gap;
    MoveWindow(state->dryRunCheckbox,     margin,                      y + 2, 200, 22,    TRUE);
    MoveWindow(state->logChangesCheckbox, margin + 210,                y + 2, 200, 22,    TRUE);
    MoveWindow(state->exportLogButton,    std::max(margin, W - margin - buttonW - gap - buttonW), y - 2, buttonW, rowH, TRUE);
    MoveWindow(state->selfTestButton,     std::max(margin, W - margin - buttonW),                 y - 2, buttonW, rowH, TRUE);

    y += rowH;
    MoveWindow(state->rawKnobsCheckbox,      margin,       y + 2, 200, 22, TRUE);
    MoveWindow(state->rawGaugesCheckbox,     margin + 210, y + 2, 180, 22, TRUE);
    MoveWindow(state->logDiagnosticsCheckbox,margin + 400, y + 2, 170, 22, TRUE);
    MoveWindow(state->captureButton, std::max(margin, W - margin - buttonW), y - 2, buttonW, rowH, TRUE);

    y += rowH + gap + 16;
    int hintW = 220;
    int statusW = std::max(320, W - margin * 2 - hintW - gap);
    MoveWindow(state->statusText, margin, y, statusW, 26, TRUE);
    MoveWindow(state->hintLabel,  margin + statusW + gap, y, hintW, 26, TRUE);

    y += 28 + gap + 12;
    int logH = std::max(120, H - y - margin);
    MoveWindow(state->logText, margin, y, std::max(200, W - margin * 2), logH, TRUE);
}

// ─── Wnd helpers ──────────────────────────────────────────────────────────────

UiState* GetUiState(HWND hwnd) {
    return reinterpret_cast<UiState*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

size_t CountLogicalLines(const std::wstring& text) {
    if (text.empty()) return 0;

    size_t lines = 0;
    for (wchar_t ch : text) {
        if (ch == L'\n') ++lines;
    }
    return lines == 0 ? 1 : lines;
}

void UpdateStatusText(UiState* s) {
    if (!s || !s->statusText) return;

    if (!s->controller || !s->controller->IsRunning()) {
        SetWindowTextW(s->statusText, s->statusPrefix.c_str());
        return;
    }

    std::wstringstream status;
    status << s->statusPrefix
           << L" | q:" << s->pendingUiLineCount
           << L" drop:" << s->droppedUiLines
           << L" rx:" << std::fixed << std::setprecision(0) << s->linesReceivedPerSec << L"/s"
           << L" ui:" << std::fixed << std::setprecision(0) << s->linesRenderedPerSec << L"/s"
           << L" flush:" << std::fixed << std::setprecision(1) << s->avgFlushMs << L"/" << s->maxFlushMs << L" ms";

    if (s->controller) {
        auto dispatch = s->controller->GetDispatchMetrics();
        status << L" tx:" << std::fixed << std::setprecision(2)
               << (static_cast<double>(dispatch.avgDispatchUs) / 1000.0)
               << L"/" << (static_cast<double>(dispatch.maxDispatchUs) / 1000.0) << L" ms"
               << L" port:" << (static_cast<double>(dispatch.avgPortWriteUs) / 1000.0)
               << L"/" << (static_cast<double>(dispatch.maxPortWriteUs) / 1000.0) << L" ms";
    }

    if (s->pendingUiLineCount >= (kMaxPendingUiLines / 2) || s->droppedUiLines > 0) {
        status << L" [UI lag]";
    }

    SetWindowTextW(s->statusText, status.str().c_str());
}

void SetStatus(UiState* s, const std::wstring& t) {
    if (!s) return;
    s->statusPrefix = t;
    UpdateStatusText(s);
}

void ResetUiMetrics(UiState* s) {
    if (!s) return;

    s->pendingUiLineCount = 0;
    s->droppedUiLines = 0;
    s->linesReceivedWindow = 0;
    s->linesRenderedWindow = 0;
    s->linesReceivedPerSec = 0.0;
    s->linesRenderedPerSec = 0.0;
    s->avgFlushMs = 0.0;
    s->maxFlushMs = 0.0;
    s->flushCount = 0;
    s->queueOverloadNoticeShown = false;
    s->metricsWindowStartedAt = std::chrono::steady_clock::now();
}

void UpdateUiRates(UiState* s) {
    if (!s) return;

    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed = now - s->metricsWindowStartedAt;
    if (elapsed.count() < 1.0) return;

    s->linesReceivedPerSec = static_cast<double>(s->linesReceivedWindow) / elapsed.count();
    s->linesRenderedPerSec = static_cast<double>(s->linesRenderedWindow) / elapsed.count();
    s->linesReceivedWindow = 0;
    s->linesRenderedWindow = 0;
    s->metricsWindowStartedAt = now;
}

void RecordUiFlush(UiState* s, size_t renderedLines, double flushMs) {
    if (!s) return;

    s->linesRenderedWindow += renderedLines;
    ++s->flushCount;
    double sampleCount = static_cast<double>(s->flushCount);
    s->avgFlushMs += (flushMs - s->avgFlushMs) / sampleCount;
    s->maxFlushMs = std::max(s->maxFlushMs, flushMs);
}

void ApplyFont(UiState* s, HWND c) {
    if (s && c && s->uiFont) SendMessage(c, WM_SETFONT, reinterpret_cast<WPARAM>(s->uiFont), TRUE);
}

void ScrollLogControlToBottom(UiState* s) {
    if (!s || !s->logText) return;

    int end = GetWindowTextLengthW(s->logText);
    SendMessage(s->logText, EM_SETSEL, end, end);
    SendMessage(s->logText, EM_SCROLLCARET, 0, 0);
    SendMessage(s->logText, WM_VSCROLL, SB_BOTTOM, 0);
}

void TrimLogControlToMaxLines(UiState* s) {
    if (!s || !s->logText) return;

    int n = GetWindowTextLengthW(s->logText);
    if (n <= 0) return;

    std::wstring text(static_cast<size_t>(n) + 1, L'\0');
    GetWindowTextW(s->logText, text.data(), n + 1);
    text.resize(static_cast<size_t>(n));

    size_t lineCount = CountLogicalLines(text);
    if (lineCount <= kMaxUiDisplayLines) return;

    size_t linesToDrop = lineCount - kMaxUiDisplayLines;
    size_t cutPos = 0;
    while (cutPos < text.size() && linesToDrop > 0) {
        if (text[cutPos] == L'\n') {
            --linesToDrop;
        }
        ++cutPos;
    }

    if (cutPos > 0) {
        SendMessage(s->logText, EM_SETSEL, 0, static_cast<LPARAM>(cutPos));
        SendMessage(s->logText, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(L""));
        ScrollLogControlToBottom(s);
    }
}

void AppendToLogControl(UiState* s, const std::wstring& text) {
    if (!s || !s->logText) return;

    // Update the edit control as one visual transaction. We do briefly select
    // the oldest text while trimming the ring buffer, but suppress redraw until
    // the control is re-anchored at the newest line so the user never sees the
    // thumb or caret jump to the top mid-update.
    SendMessage(s->logText, WM_SETREDRAW, FALSE, 0);
    int n = GetWindowTextLengthW(s->logText);
    SendMessage(s->logText, EM_SETSEL, n, n);
    SendMessage(s->logText, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(text.c_str()));
    TrimLogControlToMaxLines(s);
    ScrollLogControlToBottom(s);
    SendMessage(s->logText, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(s->logText, nullptr, TRUE);
    UpdateWindow(s->logText);
}

void AppendToLog(UiState* s, const std::wstring& text) {
    s->logBuffer += text;
    AppendToLogControl(s, text);
}

bool WriteUtf8File(const std::wstring& path, const std::wstring& content) {
    int n = WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (n <= 0) return false;
    std::string utf8(static_cast<size_t>(n - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, content.c_str(), -1, utf8.data(), n, nullptr, nullptr);
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return false;
    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    out.write(reinterpret_cast<const char*>(bom), sizeof(bom));
    out.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
    return out.good();
}

void ExportLogToFile(HWND hwnd) {
    UiState* s = GetUiState(hwnd);
    if (!s) return;
    if (s->logBuffer.empty()) { AppendToLog(s, L"No log content to export.\r\n"); return; }

    wchar_t fp[MAX_PATH] = L"dcsbios-serial-bridge-log.txt";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner   = hwnd;
    ofn.lpstrFilter = L"Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile   = fp;
    ofn.nMaxFile    = MAX_PATH;
    ofn.Flags       = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"txt";
    if (!GetSaveFileNameW(&ofn)) return;

    if (WriteUtf8File(fp, s->logBuffer)) {
        std::wstringstream m; m << L"Log exported to: " << fp;
        AppendToLog(s, m.str() + L"\r\n");
    } else {
        AppendToLog(s, L"Failed to export log.\r\n");
    }
}

std::wstring GetWindowTextContent(HWND hwnd) {
    int n = GetWindowTextLengthW(hwnd);
    std::wstring t(static_cast<size_t>(n) + 1, L'\0');
    if (n > 0) GetWindowTextW(hwnd, t.data(), n + 1);
    t.resize(static_cast<size_t>(n));
    return t;
}

// ─── Bridge toggle ───────────────────────────────────────────────────────────

void ToggleBridge(HWND hwnd) {
    UiState* s = GetUiState(hwnd);
    if (!s || !s->controller) return;

    if (s->controller->IsRunning()) {
        s->controller->Stop();
        SetWindowTextW(s->toggleButton, L"Start");
        SetStatus(s, L"Stopped");
        EnableWindow(s->sourceCombo, TRUE);
        EnableWindow(s->portsEdit, TRUE);
        EnableWindow(s->autoDetectButton, TRUE);
        return;
    }

    int sourceIndex = static_cast<int>(SendMessage(s->sourceCombo, CB_GETCURSEL, 0, 0));
    auto parsedPorts = ParsePorts(GetWindowTextContent(s->portsEdit));
    if (parsedPorts.empty()) {
        parsedPorts = DetectAvailableComPorts();
        if (!parsedPorts.empty())
            SetWindowTextW(s->portsEdit, JoinPorts(parsedPorts).c_str());
    }

    BridgeConfig cfg;
    // Map combo index to SimSourceType
    switch (sourceIndex) {
    case 0: cfg.sourceType = SimSourceType::DcsDirect;  break;
    case 1: cfg.sourceType = SimSourceType::DcsBiosUdp; break;
    case 2: cfg.sourceType = SimSourceType::DcsBiosTcp; break;
    case 3: cfg.sourceType = SimSourceType::ReplayFile; break;
    case 4: cfg.sourceType = SimSourceType::Msfs;       break;
    default: cfg.sourceType = SimSourceType::DcsDirect; break;
    }

    // For replay file source, ask for a file path
    if (cfg.sourceType == SimSourceType::ReplayFile) {
        wchar_t fp[MAX_PATH] = {};
        OPENFILENAMEW ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = hwnd;
        ofn.lpstrFilter = L"Binary Replay Files (*.bin;*.replay)\0*.bin;*.replay\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile   = fp;
        ofn.nMaxFile    = MAX_PATH;
        ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (!GetOpenFileNameW(&ofn)) return;
        cfg.replayFilePath = fp;
    }

    cfg.dryRun          = (SendMessage(s->dryRunCheckbox,     BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.logStateChanges = (SendMessage(s->logChangesCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.logRawKnobsDials = (SendMessage(s->rawKnobsCheckbox,  BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.logRawGauges     = (SendMessage(s->rawGaugesCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.logDiagnostics   = (SendMessage(s->logDiagnosticsCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.comPorts        = std::move(parsedPorts);
    s->activeDryRun     = cfg.dryRun;

    if (!s->controller->Start(cfg)) { SetStatus(s, L"Start failed"); return; }

    SaveCurrentConfig(cfg);  // persist ports + source for next launch
    ResetUiMetrics(s);
    SetWindowTextW(s->toggleButton, L"Stop");
    std::wstring status;
    switch (cfg.sourceType) {
    case SimSourceType::DcsDirect:  status = L"Running (Hornet Link Lua)"; break;
    case SimSourceType::DcsBiosUdp: status = L"Running (DCS-BIOS UDP)";    break;
    case SimSourceType::DcsBiosTcp: status = L"Running (DCS-BIOS TCP)";    break;
    case SimSourceType::ReplayFile: status = L"Running (Replay)";           break;
    case SimSourceType::Msfs:       status = L"Running (MSFS stub)";        break;
    }
    if (cfg.dryRun) status += L" [Dry Run]";
    SetStatus(s, status);
    EnableWindow(s->sourceCombo, FALSE);
    EnableWindow(s->portsEdit, FALSE);
    EnableWindow(s->autoDetectButton, FALSE);
}

// ─── Window proc ─────────────────────────────────────────────────────────────

/**
 * @brief Main window procedure for the bridge UI.
 *
 * Handles WM_CREATE (create controls, restore settings), WM_SIZE (re-layout),
 * WM_COMMAND (button/checkbox interactions), WM_TIMER (log flush, metrics),
 * kLogMessage (append log line from any thread), kBridgeStoppedMessage
 * (re-enable Start button after worker exits), WM_CTLCOLORSTATIC /
 * WM_CTLCOLOREDIT (themed colours), and WM_DESTROY (cleanup).
 */
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {

    case WM_CREATE: {
        auto* s = new UiState();
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(s));
        s->controller  = std::make_unique<BridgeController>(hwnd);
        s->bgBrush     = CreateSolidBrush(kColorBg);
        s->panelBrush  = CreateSolidBrush(kColorPanel);
        s->inputBrush  = CreateSolidBrush(kColorInputBg);
        s->buttonBrush = CreateSolidBrush(kColorButtonBg);
        s->uiFont      = CreateFontW(-18, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        auto cw = [&](LPCWSTR cls, LPCWSTR txt, DWORD style, UINT_PTR id = 0) {
            return CreateWindowW(cls, txt, WS_CHILD | WS_VISIBLE | style,
                                 0, 0, 100, 28, hwnd, reinterpret_cast<HMENU>(id), nullptr, nullptr);
        };

        s->sourceLabel  = cw(L"STATIC",   L"Source:",                  0);
        s->sourceCombo  = cw(L"COMBOBOX", L"",          CBS_DROPDOWNLIST,  kControlMode);
        // Source options in plan.md Phase 1 order
        SendMessage(s->sourceCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"DCS \u2014 Hornet Link (Lua export)"));
        SendMessage(s->sourceCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"DCS \u2014 DCS-BIOS Stream (UDP)"));
        SendMessage(s->sourceCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"DCS \u2014 DCS-BIOS Stream (TCP)"));
        SendMessage(s->sourceCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Replay File\u2026"));
        SendMessage(s->sourceCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"MSFS 2024 (Coming Soon)"));
        SendMessage(s->sourceCombo, CB_SETCURSEL, 0, 0); // Default: Hornet Link

        // Mode toolbar buttons (Sim / Preflight / Maintenance)
        s->modeSimBtn       = cw(L"BUTTON", L"Sim",         BS_PUSHBUTTON, kControlModeSimBtn);
        s->modePreflightBtn = cw(L"BUTTON", L"Preflight",   BS_PUSHBUTTON, kControlModePreflightBtn);
        s->modeMainBtn      = cw(L"BUTTON", L"Maintenance", BS_PUSHBUTTON, kControlModeMainBtn);

        s->portsLabel       = cw(L"STATIC",   L"COM Ports:",               0);
        s->portsEdit        = cw(L"EDIT",      L"",    WS_BORDER | ES_AUTOHSCROLL, kControlPorts);
        s->autoDetectButton = cw(L"BUTTON",    L"Auto Detect",  BS_PUSHBUTTON, kControlAutoDetect);
        s->toggleButton     = cw(L"BUTTON",    L"Start",        BS_PUSHBUTTON, kControlToggle);
        s->dryRunCheckbox   = cw(L"BUTTON",    L"Dry Run (no serial write)", BS_AUTOCHECKBOX, kControlDryRun);
        s->logChangesCheckbox = cw(L"BUTTON",  L"Log State Changes",        BS_AUTOCHECKBOX, kControlLogChanges);
        s->rawKnobsCheckbox = cw(L"BUTTON",    L"Raw Knobs/Dials",          BS_AUTOCHECKBOX, kControlRawKnobs);
        s->rawGaugesCheckbox = cw(L"BUTTON",   L"Raw Gauges",               BS_AUTOCHECKBOX, kControlRawGauges);
        s->logDiagnosticsCheckbox = cw(L"BUTTON", L"Log Diagnostics",        BS_AUTOCHECKBOX, kControlLogDiagnostics);
        s->exportLogButton  = cw(L"BUTTON",    L"Export Log",   BS_PUSHBUTTON, kControlExportLog);
        s->selfTestButton   = cw(L"BUTTON",    L"Self Test",    BS_PUSHBUTTON, kControlSelfTest);
        s->captureButton    = cw(L"BUTTON",    L"Start Capture", BS_PUSHBUTTON, kControlCapture);
        s->statusText       = cw(L"STATIC",    L"Stopped",                  0, kControlStatus);
        s->hintLabel        = cw(L"STATIC",    L"F8 toggles Start/Stop",    0);
        s->logText          = cw(L"EDIT",       L"",
                                  WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL,
                                  kControlLog);

        // Apply font to all controls
        for (HWND c : { s->sourceLabel, s->portsLabel, s->sourceCombo, s->portsEdit,
                         s->autoDetectButton, s->toggleButton, s->dryRunCheckbox,
                         s->logChangesCheckbox, s->rawKnobsCheckbox, s->rawGaugesCheckbox,
                         s->logDiagnosticsCheckbox, s->captureButton,
                         s->modeSimBtn, s->modePreflightBtn, s->modeMainBtn,
                         s->exportLogButton, s->selfTestButton,
                         s->statusText, s->hintLabel, s->logText }) {
            ApplyFont(s, c);
        }

        SendMessage(s->rawKnobsCheckbox, BM_SETCHECK, BST_CHECKED, 0);
        SendMessage(s->rawGaugesCheckbox, BM_SETCHECK, BST_UNCHECKED, 0);

        auto detected = DetectAvailableComPorts();
        if (!detected.empty()) SetWindowTextW(s->portsEdit, JoinPorts(detected).c_str());

        RECT rc = {};
        GetClientRect(hwnd, &rc);
        LayoutControls(s, rc.right, rc.bottom);

        RegisterHotKey(hwnd, kHotkeyId, MOD_NOREPEAT, VK_F8);
        SetTimer(hwnd, kStatusRefreshTimer, kStatusRefreshIntervalMs, nullptr);
        return 0;
    }

    case WM_SIZE: {
        UiState* s = GetUiState(hwnd);
        if (s && wParam != SIZE_MINIMIZED) LayoutControls(s, LOWORD(lParam), HIWORD(lParam));
        return 0;
    }

    case WM_ERASEBKGND: {
        UiState* s = GetUiState(hwnd);
        RECT rc = {};
        GetClientRect(hwnd, &rc);
        FillRect(reinterpret_cast<HDC>(wParam), &rc,
                 s && s->bgBrush ? s->bgBrush : reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        return 1;
    }

    case WM_CTLCOLORSTATIC: {
        UiState* s = GetUiState(hwnd);
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, kColorText);
        SetBkColor(hdc, kColorBg);
        SetBkMode(hdc, TRANSPARENT);
        return reinterpret_cast<INT_PTR>(s && s->bgBrush ? s->bgBrush : GetStockObject(NULL_BRUSH));
    }

    case WM_CTLCOLOREDIT: {
        UiState* s = GetUiState(hwnd);
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, kColorText);
        SetBkColor(hdc, kColorInputBg);
        return reinterpret_cast<INT_PTR>(s && s->inputBrush ? s->inputBrush : GetStockObject(BLACK_BRUSH));
    }

    case WM_CTLCOLORBTN: {
        UiState* s   = GetUiState(hwnd);
        HWND ctrl    = reinterpret_cast<HWND>(lParam);
        HDC  hdc     = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, IsWindowEnabled(ctrl) ? kColorText : kColorTextMuted);
        SetBkColor(hdc, kColorButtonBg);
        return reinterpret_cast<INT_PTR>(s && s->buttonBrush ? s->buttonBrush : GetStockObject(GRAY_BRUSH));
    }

    case WM_HOTKEY:
        if (wParam == kHotkeyId) ToggleBridge(hwnd);
        return 0;

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        UiState* s = GetUiState(hwnd);
        if (!s) return 0;
        switch (id) {
        case kControlAutoDetect: {
            auto detected = DetectAvailableComPorts();
            SetWindowTextW(s->portsEdit, JoinPorts(detected).c_str());
            AppendToLog(s, detected.empty() ? L"No COM ports detected.\r\n"
                                             : L"Detected: " + JoinPorts(detected) + L"\r\n");
            break;
        }
        case kControlToggle:
            ToggleBridge(hwnd);
            break;
        case kControlDryRun:
            if (s->controller && s->controller->IsRunning()) {
                SendMessage(s->dryRunCheckbox, BM_SETCHECK,
                            s->activeDryRun ? BST_CHECKED : BST_UNCHECKED, 0);
                AppendToLog(s, L"Dry Run can only be changed while stopped.\r\n");
            }
            break;
        case kControlLogChanges:
        case kControlRawKnobs:
        case kControlRawGauges:
        case kControlLogDiagnostics: {
            // Apply checkbox changes immediately to the running controller.
            if (s->controller && s->controller->IsRunning()) {
                bool changes  = (SendMessage(s->logChangesCheckbox,      BM_GETCHECK, 0, 0) == BST_CHECKED);
                bool knobs    = (SendMessage(s->rawKnobsCheckbox,         BM_GETCHECK, 0, 0) == BST_CHECKED);
                bool gauges   = (SendMessage(s->rawGaugesCheckbox,        BM_GETCHECK, 0, 0) == BST_CHECKED);
                bool diag     = (SendMessage(s->logDiagnosticsCheckbox,   BM_GETCHECK, 0, 0) == BST_CHECKED);
                s->controller->SetLoggingFlags(changes, knobs, gauges, diag);
            }
            break;
        }
        case kControlModeSimBtn:
            if (s->controller && s->controller->IsRunning())
                s->controller->SetMode(BridgeMode::Sim);
            break;
        case kControlModePreflightBtn:
            if (s->controller && s->controller->IsRunning())
                s->controller->SetMode(BridgeMode::Preflight);
            break;
        case kControlModeMainBtn:
            if (s->controller && s->controller->IsRunning())
                s->controller->SetMode(BridgeMode::Maintenance);
            break;
        case kControlExportLog:
            ExportLogToFile(hwnd);
            break;
        case kControlCapture: {
            // Toggle stream-to-disk capture.
            if (s->captureStream.is_open()) {
                s->captureStream.close();
                SetWindowTextW(s->captureButton, L"Start Capture");
                AppendToLog(s, L"Capture stopped.\r\n");
            } else {
                // Open a Save dialog to choose the capture file path.
                wchar_t filePath[MAX_PATH] = {};
                OPENFILENAMEW ofn = {};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner   = hwnd;
                ofn.lpstrFilter = L"Log Files (*.log)\0*.log\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
                ofn.lpstrFile   = filePath;
                ofn.nMaxFile    = MAX_PATH;
                ofn.lpstrDefExt = L"log";
                ofn.Flags       = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
                if (GetSaveFileNameW(&ofn)) {
                    s->captureStream.open(filePath, std::ios::out | std::ios::binary);
                    if (s->captureStream.is_open()) {
                        // Write UTF-16 LE BOM so editors recognise the encoding.
                        static constexpr wchar_t kUtf16LeBom = L'\xFEFF';
                        s->captureStream.put(kUtf16LeBom);
                        // Seed with the current in-memory log so the capture
                        // starts from the beginning of the session.
                        s->captureStream << s->logBuffer;
                        s->captureStream.flush();
                        SetWindowTextW(s->captureButton, L"Stop Capture");
                        AppendToLog(s, L"Capture started.\r\n");
                    } else {
                        AppendToLog(s, L"Failed to open capture file.\r\n");
                    }
                }
            }
            break;
        }
        case kControlSelfTest:
            if (s->controller) s->controller->RunSelfTest();
            break;
        }
        return 0;
    }

    case WM_TIMER: {
        UiState* s = GetUiState(hwnd);
        if (!s) return 0;
        if (wParam == kLogFlushTimer) {
            KillTimer(hwnd, kLogFlushTimer);
            s->logFlushScheduled = false;
            if (!s->pendingUiLog.empty()) {
                size_t renderedLines = s->pendingUiLineCount;
                auto flushStart = std::chrono::steady_clock::now();
                AppendToLogControl(s, s->pendingUiLog);
                auto flushEnd = std::chrono::steady_clock::now();
                double flushMs = std::chrono::duration<double, std::milli>(flushEnd - flushStart).count();
                RecordUiFlush(s, renderedLines, flushMs);
                s->pendingUiLog.clear();
                s->pendingUiLineCount = 0;
            }
            UpdateUiRates(s);
            UpdateStatusText(s);
        } else if (wParam == kStatusRefreshTimer) {
            UpdateUiRates(s);
            UpdateStatusText(s);
        }
        return 0;
    }

    case kLogMessage: {
        UiState* s = GetUiState(hwnd);
        if (s) {
            auto* payload = reinterpret_cast<std::wstring*>(lParam);
            if (payload) {
                size_t incomingLines = CountLogicalLines(*payload);

                // Write to capture file before any buffering decisions.
                if (s->captureStream.is_open()) {
                    s->captureStream << *payload;
                    s->captureStream.flush();
                }

                // Maintain bounded in-memory log. Trim from the front when the
                // buffer exceeds kMaxLogBufferLines so long sessions don't exhaust memory.
                s->logBuffer += *payload;
                size_t totalLines = CountLogicalLines(s->logBuffer);
                if (totalLines > kMaxLogBufferLines) {
                    size_t linesToDrop = totalLines - kMaxLogBufferLines;
                    size_t cutPos = 0;
                    while (cutPos < s->logBuffer.size() && linesToDrop > 0) {
                        if (s->logBuffer[cutPos] == L'\n') --linesToDrop;
                        ++cutPos;
                    }
                    s->logBuffer.erase(0, cutPos);
                }

                s->linesReceivedWindow += incomingLines;

                if (s->pendingUiLineCount + incomingLines > kMaxPendingUiLines) {
                    s->droppedUiLines += incomingLines;

                    // Preserve the full in-memory capture while dropping only the
                    // queued UI work. This keeps overload visible without implying
                    // the serial/export pipeline itself lost data.
                    if (!s->queueOverloadNoticeShown) {
                        AppendToLogControl(s, L"[ui] Pending render queue exceeded limit; dropping queued UI lines until it recovers.\r\n");
                        s->queueOverloadNoticeShown = true;
                    }
                } else {
                    s->pendingUiLog += *payload;
                    s->pendingUiLineCount += incomingLines;
                    if (!s->logFlushScheduled) {
                        SetTimer(hwnd, kLogFlushTimer, kLogFlushIntervalMs, nullptr);
                        s->logFlushScheduled = true;
                    }
                }

                if (s->pendingUiLineCount == 0) {
                    s->queueOverloadNoticeShown = false;
                }

                // Do NOT call UpdateStatusText here — it runs at kStatusRefreshIntervalMs
                // via kStatusRefreshTimer. Calling it on every message causes visible flicker.
                delete payload;
            }
        }
        return 0;
    }

    case kBridgeStoppedMessage: {
        UiState* s = GetUiState(hwnd);
        if (s) {
            SetWindowTextW(s->toggleButton, L"Start");
            SetStatus(s, L"Stopped");
            EnableWindow(s->sourceCombo, TRUE);
            EnableWindow(s->portsEdit, TRUE);
            EnableWindow(s->autoDetectButton, TRUE);
        }
        return 0;
    }

    case WM_DESTROY: {
        UiState* s = GetUiState(hwnd);
        if (s) {
            if (s->logFlushScheduled) KillTimer(hwnd, kLogFlushTimer);
            KillTimer(hwnd, kStatusRefreshTimer);
            if (!s->pendingUiLog.empty()) {
                AppendToLogControl(s, s->pendingUiLog);
                s->pendingUiLog.clear();
                s->pendingUiLineCount = 0;
            }
            if (s->controller) s->controller->Stop();
            if (s->captureStream.is_open()) s->captureStream.close();
            UnregisterHotKey(hwnd, kHotkeyId);
            if (s->uiFont)      DeleteObject(s->uiFont);
            if (s->bgBrush)     DeleteObject(s->bgBrush);
            if (s->panelBrush)  DeleteObject(s->panelBrush);
            if (s->inputBrush)  DeleteObject(s->inputBrush);
            if (s->buttonBrush) DeleteObject(s->buttonBrush);
            delete s;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        }
        PostQuitMessage(0);
        return 0;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

} // namespace

// ─── Entry point ─────────────────────────────────────────────────────────────

/**
 * @brief Windows application entry point.
 *
 * Parses startup options (--autostart, --ports, etc.), registers the window
 * class, creates the main window, and runs the standard Win32 message loop.
 */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    StartupOptions opts = ParseStartupOptions();
    LoadSavedConfig(opts);  // pre-fill any field not already set from the command line

    const wchar_t kClass[] = L"HornetLinkWindow";
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = kClass;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        WS_EX_COMPOSITED, kClass, L"Hornet Link",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 960, 600,
        nullptr, nullptr, hInstance, nullptr);
    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);

    UiState* s = GetUiState(hwnd);
    if (s) {
        s->startupOptions = std::move(opts);
        if (s->startupOptions.forceUdp.has_value()) {
            // Map legacy --udp / --tcp flags to new source combo
            // --udp → DCS-BIOS UDP (index 1), --tcp → DCS-BIOS TCP (index 2)
            SendMessage(s->sourceCombo, CB_SETCURSEL,
                        s->startupOptions.forceUdp.value() ? 1 : 2, 0);
        }
        if (s->startupOptions.ports.has_value())
            SetWindowTextW(s->portsEdit, JoinPorts(s->startupOptions.ports.value()).c_str());
        if (s->startupOptions.dryRun)
            SendMessage(s->dryRunCheckbox, BM_SETCHECK, BST_CHECKED, 0);
        if (s->startupOptions.autoStart)
            ToggleBridge(hwnd);
    }

    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return static_cast<int>(msg.wParam);
}
