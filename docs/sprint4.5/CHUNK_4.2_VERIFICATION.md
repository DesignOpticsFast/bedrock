# Chunk 4.2 Verification - Create VERSIONS.md (Single Source of Truth)

**Date:** 2025-01-26  
**Chunk:** 4.2 - Create VERSIONS.md (Single Source of Truth)  
**Status:** ✅ Complete

---

## Summary

**VERSIONS.md created for both Bedrock and Phoenix:**
- ✅ `bedrock/docs/VERSIONS.md` - Bedrock toolchain versions
- ✅ `phoenix/docs/VERSIONS.md` - Phoenix toolchain versions
- ✅ Both documents serve as single source of truth for toolchain versions
- ✅ No build or CI changes in this chunk (docs-only)

---

## Bedrock VERSIONS.md Summary

**File:** `bedrock/docs/VERSIONS.md`

### Contents:

**Toolchain Overview:**
- C++ Standard: **C++20** (required)
- CMake: Minimum **3.20**
- Build System: Ninja (recommended), Make (also supported)

**Library Versions:**
- **Protobuf:** 6.33+ (required for Palantir transport)
- **OpenMP:** Required (system-provided, varies by platform)
- **TBB:** Available in CI (libtbb-dev), planned for future use
- **Qt:** 6.10.1 (optional, only when `BEDROCK_WITH_TRANSPORT_DEPS=ON`)

**Supported Platforms:**
- **macOS (Crucible):** Apple Clang 17.0.0, OpenMP via Homebrew, Protobuf 6.33+
- **Linux (Fulcrum/CI):** GCC (varies), OpenMP (libomp-dev), TBB (libtbb-dev), GTest (libgtest-dev)
- **Windows:** Not currently supported

**CI Toolchain:**
- **Linux (ubuntu-latest):** CMake, Ninja, GCC, libtbb-dev, libgtest-dev, libomp-dev, lcov, Qt 6.10.1
- **macOS (macos-latest):** CMake, Ninja, Apple Clang, libomp (Homebrew), Protobuf (Homebrew), Qt 6.10.1

**Future Planning:**
- C++23 upgrade planned (links to `C++23_UPGRADE_PLAN.md`)
- Threading libraries: OpenMP (current), TBB (future)

---

## Phoenix VERSIONS.md Summary

**File:** `phoenix/docs/VERSIONS.md`

### Contents:

**Toolchain Overview:**
- C++ Standard: **C++17** (required)
- **Qt:** **6.10.1** (baseline, required)
- CMake: Minimum **3.20**
- Build System: Ninja (recommended), Make (also supported)

**Library Versions:**
- **Protobuf:** 6.33+ (required for Palantir contracts, when `PHX_WITH_TRANSPORT_DEPS=ON`)
- **libsodium:** Optional (default ON, for cryptographic operations)

**Platform Notes:**
- **macOS (Crucible):**
  - Qt path: `$HOME/Qt/6.10.1/macos`
  - CMAKE_PREFIX_PATH: `$HOME/Qt/6.10.1/macos/lib/cmake`
  - Installation method: Qt Online Installer
  - Compiler: Apple Clang 17.0.0
  - Example CMake configuration included
- **Linux (Fulcrum/CI):**
  - Qt 6.10.1 (via CI action)
  - GCC (varies by CI image)
- **Windows:** Supported (details TBD)

**CI Toolchain:**
- **Linux (ubuntu-latest):** CMake, Ninja, GCC, Qt 6.10.1, Protobuf 6.33+, lcov
- **macOS (macos-latest):** CMake, Ninja, Apple Clang, Qt 6.10.1, Protobuf 6.33+

**CMake Options:**
- `PHX_WITH_TRANSPORT_DEPS` (default OFF)
- `PHX_WITH_PALANTIR_CONTRACTS` (default ON)
- `PHX_WITH_LIBSODIUM` (default ON)
- `PHX_DEV_TOOLS` (default OFF)

**Future Planning:**
- Bedrock C++23 upgrade (informational, Phoenix remains C++17)
- Qt version updates (documented when changes occur)

**Verification Section:**
- Qt 6.10.1 verification results from Chunk 4.1 included

---

## Verification

### Files Created

✅ **Bedrock:**
- `bedrock/docs/VERSIONS.md` - Created (178 lines)
- Contains: C++20, CMake 3.20, Protobuf 6.33+, OpenMP, TBB, Qt 6.10.1 (optional), platform notes, CI toolchain

✅ **Phoenix:**
- `phoenix/docs/VERSIONS.md` - Created (234 lines)
- Contains: C++17, Qt 6.10.1 (baseline), CMake 3.20, Protobuf 6.33+, libsodium, platform notes (including macOS Qt path), CI toolchain, CMake options

### Content Accuracy

✅ **Bedrock VERSIONS.md:**
- C++ standard: ✅ C++20 (matches `CMakeLists.txt`)
- CMake minimum: ✅ 3.20 (matches `CMakeLists.txt`)
- Protobuf: ✅ 6.33+ (matches comments in `CMakeLists.txt` and actual version 33.1)
- OpenMP: ✅ Required (matches `CMakeLists.txt`)
- TBB: ✅ Available in CI (matches CI workflow)
- Qt: ✅ 6.10.1, optional (matches `CMakeLists.txt` and CI workflow)
- Platforms: ✅ macOS, Linux, Windows (not supported)
- CI: ✅ Matches actual CI workflow configuration

✅ **Phoenix VERSIONS.md:**
- C++ standard: ✅ C++17 (matches `CMakeLists.txt`)
- Qt: ✅ 6.10.1 (matches `CMakeLists.txt` and Chunk 4.1 verification)
- CMake minimum: ✅ 3.20 (matches `CMakeLists.txt`)
- Protobuf: ✅ 6.33+ (matches comments in `CMakeLists.txt` and actual version 33.1)
- macOS Qt path: ✅ `$HOME/Qt/6.10.1/macos` (matches Chunk 4.1 verification)
- CMAKE_PREFIX_PATH: ✅ Correct path documented
- CI: ✅ Matches actual CI workflow configuration
- CMake options: ✅ All documented options match `CMakeLists.txt`

### No Build or CI Changes

✅ **No changes to:**
- CMakeLists.txt files
- CI workflow files
- Build system configuration
- Source code

✅ **Only documentation created:**
- Two new markdown files
- No functional changes

---

## Key Information Captured

### Bedrock

**Toolchain:**
- C++20, CMake 3.20, OpenMP (required), Protobuf 6.33+, TBB (available), Qt 6.10.1 (optional)

**Platforms:**
- macOS (Crucible): Apple Clang 17.0.0
- Linux (CI): GCC, OpenMP, TBB, GTest
- Windows: Not supported

**CI:**
- Linux: CMake, Ninja, GCC, libtbb-dev, libgtest-dev, libomp-dev, lcov, Qt 6.10.1
- macOS: CMake, Ninja, Apple Clang, libomp, Protobuf, Qt 6.10.1

### Phoenix

**Toolchain:**
- C++17, Qt 6.10.1 (baseline), CMake 3.20, Protobuf 6.33+, libsodium (optional)

**Platforms:**
- macOS (Crucible): Qt at `$HOME/Qt/6.10.1/macos`, CMAKE_PREFIX_PATH documented, Apple Clang 17.0.0
- Linux (CI): Qt 6.10.1, GCC
- Windows: Supported (details TBD)

**CI:**
- Linux: CMake, Ninja, GCC, Qt 6.10.1, Protobuf 6.33+, lcov
- macOS: CMake, Ninja, Apple Clang, Qt 6.10.1, Protobuf 6.33+

**CMake Options:**
- All options documented with defaults and usage

---

## Future Use

**VERSIONS.md will be referenced by:**
- Chunk 4.3: Update other docs to reference VERSIONS.md instead of hard-coding versions
- Future documentation: All docs should reference VERSIONS.md for toolchain versions
- Developer onboarding: Single source of truth for required toolchain versions
- CI updates: When toolchain versions change, update VERSIONS.md first

---

## Conclusion

**Status:** ✅ **CHUNK 4.2 COMPLETE**

**VERSIONS.md is now the single source of truth for toolchain versions in Bedrock and Phoenix.**

**Deliverables:**
- ✅ `bedrock/docs/VERSIONS.md` - Complete toolchain documentation for Bedrock
- ✅ `phoenix/docs/VERSIONS.md` - Complete toolchain documentation for Phoenix
- ✅ Both documents accurately reflect current toolchain reality
- ✅ No build or CI changes (docs-only chunk)

**Next Step:** Chunk 4.3 - Update other docs to reference VERSIONS.md instead of duplicating version information

---

**Verification Completed By:** UnderLord  
**Review Status:** Awaiting Mark/Lyra approval before proceeding to Chunk 4.3

