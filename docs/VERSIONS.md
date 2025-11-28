# Bedrock Toolchain Versions

**Last Updated:** 2025-01-26 (Sprint 4.5)  
**Purpose:** Single source of truth for Bedrock toolchain versions

---

## Bedrock Version

**Current Version:** 0.3.0

**Release Notes:**
- 0.3.0 is the first release with envelope-only Palantir protocol, validated threading model, comprehensive integration tests, and CI coverage.
- This version establishes the foundation for future development (C++23 upgrade, Thermal Diffusion, Arrow Flight transport).

**Version History:**
- 0.0.1: Initial baseline (pre-Sprint 4.5)
- 0.3.0: Sprint 4.5 foundation release (envelope protocol, hardening, tests, CI)

**See Also:**
- `docs/palantir/PROTOCOL_VERSIONS.md` - Palantir protocol version history

---

## Toolchain Overview

### C++ Standard
- **C++20** (required)
- Set via `CMAKE_CXX_STANDARD 20` in `CMakeLists.txt`

### CMake
- **Minimum version:** 3.20
- Required for C++20 support and modern CMake features

### Build System
- **Ninja** (recommended, used in CI)
- **Make** (also supported)

---

## Library Versions

### Protobuf
- **Version:** 6.33+ (required for Palantir transport)
- Used for Palantir IPC protocol serialization
- Protobuf 6.33+ requires Abseil libraries (automatically linked)

### OpenMP
- **Required:** Yes
- Used for multithreading support in compute kernels
- Version: System-provided (varies by platform)

### TBB (Intel Threading Building Blocks)
- **Status:** Available in CI (libtbb-dev on Linux)
- **Usage:** Planned for future parallel compute workloads
- Not currently required, but available for future use

### Qt
- **Status:** Optional (only when `BEDROCK_WITH_TRANSPORT_DEPS=ON`)
- **Version:** 6.10.1 (for Palantir server tests)
- **Components:** Core, Network
- **Note:** Core Bedrock engine does not require Qt; Qt is only needed for Palantir IPC server

---

## Supported Platforms

### macOS (Crucible - Primary Development)

**Compiler:**
- **Apple Clang 17.0.0** (or compatible)
- Target: arm64-apple-darwin

**Libraries:**
- OpenMP: System-provided (via Homebrew: `libomp`)
- Protobuf: Homebrew (`protobuf` package, version 6.33+)
- TBB: Homebrew (`libtbb` package, optional)

**Build Notes:**
- CMake finds dependencies via standard paths (Homebrew installs to `/opt/homebrew` on Apple Silicon)
- Palantir tests require Qt 6.10.1 (when `BEDROCK_WITH_TRANSPORT_DEPS=ON`)

### Linux (Fulcrum / CI)

**Compiler:**
- **GCC** (version varies by CI image, typically GCC 11+)
- **Clang** (alternative, if available)

**Libraries (CI):**
- OpenMP: `libomp-dev` (Ubuntu package)
- Protobuf: System package or Homebrew (version 6.33+)
- TBB: `libtbb-dev` (Ubuntu package, available but not required)
- GTest: `libgtest-dev` (for unit tests)

**CI Environment:**
- **Ubuntu:** `ubuntu-latest` (GitHub Actions)
- **macOS:** `macos-latest` (GitHub Actions)

### Windows

**Status:** Not currently supported for core engine development  
**Note:** Windows support may be added in future sprints; toolchain versions TBD

---

## CI Toolchain

### GitHub Actions (Linux - ubuntu-latest)

**Build Tools:**
- CMake (system package)
- Ninja (system package)
- GCC (system default)

**Libraries:**
- `libtbb-dev` (TBB, optional)
- `libgtest-dev` (GTest, for tests)
- `libomp-dev` (OpenMP, required)
- `lcov` (coverage reporting)

**Qt:**
- Qt 6.10.1 (installed via `jurplel/install-qt-action@v3`)
- Only used when `BEDROCK_WITH_TRANSPORT_DEPS=ON`

### GitHub Actions (macOS - macos-latest)

**Build Tools:**
- CMake (Homebrew)
- Ninja (Homebrew)
- Apple Clang (system default)

**Libraries:**
- `libomp` (OpenMP, via Homebrew)
- Protobuf (via Homebrew, version 6.33+)

**Qt:**
- Qt 6.10.1 (installed via `jurplel/install-qt-action@v3`)
- Only used when `BEDROCK_WITH_TRANSPORT_DEPS=ON`

---

## Version Constraints

### Minimum Versions
- **CMake:** 3.20 (required for C++20 support)
- **Protobuf:** 6.33+ (required for Abseil compatibility)
- **OpenMP:** System-provided (version varies)

### Recommended Versions
- **CMake:** 3.20 or later
- **Protobuf:** 6.33+ (latest stable)
- **OpenMP:** Latest system-provided version

---

## Future Planning

### C++23 Upgrade
- **Status:** Planned for future sprint
- **Documentation:** See `docs/C++23_UPGRADE_PLAN.md` for details
- **Current baseline:** C++20 (stable, well-supported)

### Threading Libraries
- **OpenMP:** Current baseline for multithreading
- **TBB:** Available in CI, planned for future parallel workloads
- **Hybrid approach:** OpenMP for compute kernels, TBB for task parallelism (future)

---

## Notes

- **Palantir IPC:** Requires Qt 6.10.1 only when `BEDROCK_WITH_TRANSPORT_DEPS=ON`
- **Core Engine:** No Qt dependency; pure C++20 with OpenMP
- **CI Coverage:** Coverage reporting uses `lcov` (Linux only, Chunk 3.6)
- **Version Updates:** This document should be updated when toolchain versions change

---

**See Also:**
- `docs/THREADING.md` - Threading model and concurrency guidelines
- `docs/C++23_UPGRADE_PLAN.md` - Future C++23 upgrade planning

