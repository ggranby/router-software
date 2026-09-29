# Security Policy

## Supported versions

Only the latest release (and `main`) receives fixes. Hornet Link is pre-1.0;
older tags are not patched.

## Reporting a vulnerability

Please **do not** open a public issue for security problems. Use GitHub's
private vulnerability reporting instead:

1. Go to the repository's **Security** tab.
2. Choose **Report a vulnerability** and describe the issue, affected version,
   and steps to reproduce.

You should receive an acknowledgement within 7 days. Please allow time for a
fix to be released before disclosing publicly.

## Scope and threat model

Hornet Link runs on a local gaming PC. Its inputs are treated as untrusted and
are bounds-checked and fuzzed in CI:

| Input | Source | Exposure |
|---|---|---|
| DCS-BIOS UDP export | Multicast `239.255.50.10:5010` | Socket is bound to **all interfaces** (`INADDR_ANY`), so any host on the LAN can send export frames. Use a host firewall if the PC is on an untrusted network. |
| Hornet Link Lua export | UDP `127.0.0.1:42002` | Loopback only. |
| DCS-BIOS TCP stream | `127.0.0.1:7778` | Loopback only. |
| Device handshake and import lines | USB/serial devices | Any connected device. |
| `device_profiles.json`, `templates/panels.json`, control reference JSON | Files beside the executable | Local user. |

Import commands from devices are forwarded to DCS-BIOS on `127.0.0.1:7778`.
They are restricted to printable ASCII identifier/action/value tokens, but
any connected device can operate cockpit controls; this is by design.

Release binaries are not code-signed yet. Verify downloads against the
`SHA256SUMS.txt` published with each release.
