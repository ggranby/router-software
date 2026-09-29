# Contributing to Hornet Link

Thanks for your interest! Contributions are accepted under the repository
license (CC BY-NC-SA 4.0, see [LICENSE](LICENSE)).

Start with [docs/DEVELOPER_GUIDE.md](docs/DEVELOPER_GUIDE.md): it is the
authoritative source for project status, build instructions, and the backlog.

## Workflow

1. Fork the repository and create a feature branch from `main`.
2. Make focused changes; keep unrelated refactors and reformatting out of the
   same pull request.
3. Add or update tests for protocol, parser, or state-processing changes
   (`tests/`). Mark hardware-only verification explicitly in the PR.
4. Update documentation alongside behavior changes. Protocol changes must update
   [docs/PROTOCOL_REFERENCE.md](docs/PROTOCOL_REFERENCE.md), the firmware
   library, and the bridge together.
5. Add an entry under **Unreleased** in [CHANGELOG.md](CHANGELOG.md).
6. Open a pull request using the template and make sure CI is green.

## Local checks

From the repository root (see the Developer Guide for details):

```bash
# Unit tests (Windows / MSVC)
cmake -S tests -B tests/build
cmake --build tests/build --config Release
ctest --test-dir tests/build -C Release --output-on-failure

# Unit tests with sanitizers (Linux / GCC or Clang)
cmake -S tests -B build-tests -DHORNET_LINK_SANITIZE=ON
cmake --build build-tests && ctest --test-dir build-tests --output-on-failure

# Lua lint
(cd Programs/dcsbios-serial-bridge/lua && luacheck .)
```

## Style

- C++17, 4-space indentation, no tabs. `.editorconfig` and `.clang-format`
  describe the style; format new or heavily edited code, but do not
  mass-reformat existing files.
- Treat all network, serial, and file input as untrusted: bound every length and
  index, and prefer returning errors to throwing across thread boundaries.
- Do not commit build output, generated docs (`docs/doxygen/`), or secrets.

## Reporting bugs and security issues

Open an issue for bugs and feature requests. Report security vulnerabilities
privately as described in [SECURITY.md](SECURITY.md).
