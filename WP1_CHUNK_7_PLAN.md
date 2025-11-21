# WP1 Chunk 7: Bedrock CI Expansion (macOS + Windows) - Implementation Plan

**Date:** 2025-11-17  
**Status:** Planning Complete - Ready for Review  
**WP1 Chunk:** 7 of 10

---

## Overview

Expand Bedrock's CI to include macOS and Windows builds alongside the existing Linux/dev-01 pipeline. This ensures the minimal C++23 engine + Echo + transport sanity all build and test on all three major platforms.

**Critical Constraint:** Keep `BEDROCK_WITH_TRANSPORT_DEPS=OFF` in CI for now (no gRPC/protobuf/libsodium required on macOS/Windows yet)

---

## Current CI State Analysis

### Existing Workflows:

1. **`.github/workflows/ci.yml`** (Main CI):
   - **Platform:** Linux (self-hosted dev-01 runner)
   - **Job name:** `build`
   - **Steps:**
     - Checkout with submodules
     - Contract SHA parity check ✅
     - Install deps (toolchain + TBB + ccache)
     - Configure CMake (engine-only, BEDROCK_WITH_OCCT=OFF)
     - Build bedrock_engine
     - Uses ccache for caching
   - **Tests:** Not explicitly run (builds libraries only)

2. **`.github/workflows/ci-linux-phase0.yml`** (Phase 0 CI):
   - **Platform:** Linux (ubuntu-latest, GitHub-hosted)
   - **Job name:** `build-and-test`
   - **Steps:**
     - Checkout (no submodules)
     - Install dependencies
     - Configure CMake
     - Build bedrock_engine and bedrock_sanity
     - Run tests via ctest
   - **Tests:** Runs bedrock_sanity via ctest

### Current Test Structure:

**Always built (BUILD_TESTING=ON):**
- `bedrock_sanity` - Basic engine sanity check
- `bedrock_tests` - GoogleTest-based tests (test_math.cpp)

**Conditional (BEDROCK_WITH_TRANSPORT_DEPS=ON):**
- `bedrock_echo_sanity` - Echo RPC sanity
- `bedrock_transport_deps_sanity` - Transport deps sanity

### Contract Parity Check:

Already implemented in main `ci.yml`:
```bash
expected=$(cat .contract-version)
actual=$(git -C contracts rev-parse HEAD)
if [ "$expected" != "$actual" ]; then
  echo "ERROR: contracts submodule SHA mismatch."
  exit 1
fi
```

---

## Implementation Plan

### Strategy:

**Extend main `ci.yml` workflow** with two new jobs:
- `build-macos` - macOS build and test
- `build-windows` - Windows build and test

**Keep existing `build` job** unchanged (Linux/dev-01).

**All jobs should:**
- Checkout with submodules
- Run contract parity check
- Configure CMake with `BUILD_TESTING=ON`
- Build bedrock_engine and bedrock_sanity
- Run tests via ctest
- Keep `BEDROCK_WITH_TRANSPORT_DEPS=OFF` (no transport deps)

---

### Phase 1: Add macOS CI Job

**Job name:** `build-macos`  
**Platform:** `macos-latest` (GitHub-hosted)

**Steps:**

1. **Checkout with submodules:**
```yaml
- name: Checkout repo
  uses: actions/checkout@v5
  with:
    submodules: recursive
```

2. **Verify contracts submodule SHA parity:**
```yaml
- name: Verify contracts submodule SHA parity
  run: |
    expected=$(cat .contract-version)
    actual=$(git -C contracts rev-parse HEAD)
    
    echo "Expected contracts SHA: $expected"
    echo "Actual   contracts SHA: $actual"
    
    if [ "$expected" != "$actual" ]; then
      echo "ERROR: contracts submodule SHA mismatch."
      echo "Expected: $expected"
      echo "Actual:   $actual"
      echo "Fix: update contracts/ to the expected commit, or update .contract-version to match, in a coordinated change."
      exit 1
    fi
    echo "✅ Contracts SHA parity check passed"
```

3. **Configure CMake:**
```yaml
- name: Configure CMake
  run: |
    cmake -S . -B build/macos \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=ON
```

**Notes:**
- Use `build/macos` directory
- Release build type
- BUILD_TESTING=ON (enables bedrock_sanity and bedrock_tests)
- Do NOT set BEDROCK_WITH_TRANSPORT_DEPS (defaults to OFF)

4. **Build:**
```yaml
- name: Build
  run: |
    cmake --build build/macos --config Release
```

5. **Run tests:**
```yaml
- name: Run tests
  run: |
    ctest --test-dir build/macos --output-on-failure
```

6. **Upload CMake logs (on failure):**
```yaml
- name: Upload CMake logs (on failure)
  if: failure()
  uses: actions/upload-artifact@v4
  with:
    name: cmake-logs-macos
    path: |
      build/macos/CMakeFiles/CMakeOutput.log
      build/macos/CMakeFiles/CMakeError.log
    retention-days: 7
```

---

### Phase 2: Add Windows CI Job

**Job name:** `build-windows`  
**Platform:** `windows-latest` (GitHub-hosted)

**Steps:**

1. **Checkout with submodules:**
```yaml
- name: Checkout repo
  uses: actions/checkout@v5
  with:
    submodules: recursive
```

2. **Verify contracts submodule SHA parity:**
```yaml
- name: Verify contracts submodule SHA parity
  shell: bash
  run: |
    expected=$(cat .contract-version)
    actual=$(git -C contracts rev-parse HEAD)
    
    echo "Expected contracts SHA: $expected"
    echo "Actual   contracts SHA: $actual"
    
    if [ "$expected" != "$actual" ]; then
      echo "ERROR: contracts submodule SHA mismatch."
      echo "Expected: $expected"
      echo "Actual:   $actual"
      echo "Fix: update contracts/ to the expected commit, or update .contract-version to match, in a coordinated change."
      exit 1
    fi
    echo "✅ Contracts SHA parity check passed"
```

**Notes:**
- Use `shell: bash` for Windows (Git Bash available)
- Same logic as Linux/macOS

3. **Configure CMake:**
```yaml
- name: Configure CMake
  shell: bash
  run: |
    cmake -S . -B build/windows \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_TESTING=ON
```

**Notes:**
- Use `build/windows` directory
- Use `shell: bash` for consistency
- Release build type
- BUILD_TESTING=ON
- Do NOT set BEDROCK_WITH_TRANSPORT_DEPS

4. **Build:**
```yaml
- name: Build
  shell: bash
  run: |
    cmake --build build/windows --config Release
```

5. **Run tests:**
```yaml
- name: Run tests
  shell: bash
  run: |
    ctest --test-dir build/windows --output-on-failure
```

6. **Upload CMake logs (on failure):**
```yaml
- name: Upload CMake logs (on failure)
  if: failure()
  uses: actions/upload-artifact@v4
  with:
    name: cmake-logs-windows
    path: |
      build/windows/CMakeFiles/CMakeOutput.log
      build/windows/CMakeFiles/CMakeError.log
    retention-days: 7
```

---

### Phase 3: Platform-Specific Considerations

**macOS:**
- Should work out of the box (clang supports C++23)
- No special dependencies needed (CMake, compiler come with Xcode)
- May need to handle case sensitivity in paths (macOS is case-insensitive by default)

**Windows:**
- Use Git Bash (`shell: bash`) for consistency
- CMake should detect MSVC or MinGW compiler
- May need to handle path separators (CMake handles this)
- No Unix domain sockets (if any tests use them, guard with `if(NOT WIN32)`)

**Potential Issues:**

1. **Unix Domain Sockets:**
   - If any tests use Unix domain sockets, they won't work on Windows
   - **Solution:** Guard those tests with `if(NOT WIN32)` in CMakeLists.txt
   - Document in summary

2. **Path Separators:**
   - CMake handles this automatically
   - No changes needed

3. **Case Sensitivity:**
   - macOS is case-insensitive, Linux/Windows are case-sensitive
   - Should be fine if codebase is consistent

4. **Compiler Differences:**
   - macOS: clang
   - Windows: MSVC or MinGW
   - Linux: GCC
   - All should support C++23

---

### Phase 4: Workflow Structure

**Final workflow structure:**

```yaml
name: CI

on:
  push:
    branches: [ "main" ]
    tags: [ "v*" ]
  pull_request:
    branches: [ "main" ]
  workflow_dispatch:

permissions:
  contents: read
  security-events: write

jobs:
  build:                       # Existing Linux/dev-01 job
    name: Ubuntu build (with OpenCascade)
    runs-on: [self-hosted, linux, x64, dev01, fastpath]
    # ... existing steps ...

  build-macos:                 # NEW: macOS job
    name: macOS build
    runs-on: macos-latest
    # ... new steps ...

  build-windows:               # NEW: Windows job
    name: Windows build
    runs-on: windows-latest
    # ... new steps ...
```

**Job independence:**
- All jobs run in parallel
- Each job is independent (no dependencies)
- Each job checks contract parity
- Each job builds and tests independently

---

## Implementation Steps

1. **Read current `ci.yml` workflow:**
   - Understand existing structure
   - Identify where to add new jobs

2. **Add macOS job:**
   - Add `build-macos` job after `build` job
   - Copy contract parity check from existing job
   - Add macOS-specific steps

3. **Add Windows job:**
   - Add `build-windows` job after `build-macos` job
   - Copy contract parity check from existing job
   - Add Windows-specific steps (use `shell: bash`)

4. **Test locally (if possible):**
   - Verify CMake configuration works
   - Verify build succeeds
   - Verify tests run

5. **Push and verify:**
   - Push changes to a branch
   - Open PR or push to main
   - Verify all three jobs pass:
     - Linux (existing)
     - macOS (new)
     - Windows (new)

6. **Handle any platform-specific issues:**
   - If tests fail on Windows due to Unix sockets, add guards
   - Document any platform-specific workarounds

---

## Verification Checklist

- [ ] macOS job added to `ci.yml`
- [ ] Windows job added to `ci.yml`
- [ ] Contract parity check runs in all jobs
- [ ] All jobs checkout with submodules
- [ ] All jobs configure with `BUILD_TESTING=ON`
- [ ] All jobs build bedrock_engine and bedrock_sanity
- [ ] All jobs run tests via ctest
- [ ] `BEDROCK_WITH_TRANSPORT_DEPS` remains OFF (not set)
- [ ] CMake logs uploaded on failure
- [ ] Linux job still passes
- [ ] macOS job passes
- [ ] Windows job passes
- [ ] Contract parity checks pass in all jobs

---

## Expected Outcomes

### Files Modified:
- `.github/workflows/ci.yml` - Added macOS and Windows jobs

### New CI Jobs:
- `build-macos` - macOS build and test
- `build-windows` - Windows build and test

### Test Coverage:
- All platforms run `bedrock_sanity`
- All platforms run `bedrock_tests` (if BUILD_TESTING=ON)
- Transport deps tests NOT run (BEDROCK_WITH_TRANSPORT_DEPS=OFF)

### Contract Parity:
- All three platforms check contract SHA parity
- Failures caught early on all platforms

---

## Platform-Specific Notes

### macOS:
- Uses `macos-latest` runner
- Clang compiler (C++23 support)
- No special dependencies needed
- CMake and compiler come with Xcode

### Windows:
- Uses `windows-latest` runner
- MSVC or MinGW compiler (C++23 support)
- Uses `shell: bash` for consistency
- CMake handles path separators automatically

### Linux:
- Existing job unchanged
- Self-hosted dev-01 runner
- GCC compiler (C++23 support)

---

## Questions & Decisions

### ✅ Resolved:

1. **Workflow file:** Extend main `ci.yml` (not create new workflow)
2. **Job structure:** Add two new jobs (macOS, Windows) alongside existing Linux job
3. **Contract parity:** Copy existing check to all new jobs
4. **Build type:** Release for all platforms
5. **Testing:** BUILD_TESTING=ON for all platforms
6. **Transport deps:** Keep OFF (not required for WP1 Chunk 7)

### ❓ Open Questions:

1. **Shell for Windows:** Use `shell: bash` (Git Bash) or PowerShell?
   - **Recommendation:** Use `shell: bash` for consistency with Linux/macOS scripts

2. **CMake generator:** Let CMake auto-detect or specify explicitly?
   - **Recommendation:** Let CMake auto-detect (works well on all platforms)

3. **Build parallelism:** Use `-j` flag or let CMake decide?
   - **Recommendation:** Let CMake decide (default parallelism is usually good)

4. **Test filtering:** Should we filter out any tests on Windows?
   - **Recommendation:** Only if tests fail due to platform limitations (e.g., Unix sockets)

5. **Artifact naming:** Use platform-specific names for logs?
   - **Recommendation:** Yes (`cmake-logs-macos`, `cmake-logs-windows`)

---

## Summary

This chunk expands Bedrock CI to macOS and Windows by:
1. **Adding two new jobs** to the main `ci.yml` workflow
2. **Copying contract parity check** to all platforms
3. **Building and testing** on all three platforms
4. **Keeping transport deps OFF** (not required yet)
5. **Handling platform-specific quirks** as needed

The result is comprehensive CI coverage across Linux, macOS, and Windows, ensuring Bedrock builds and tests successfully on all major platforms.

---

**Ready for review and approval before implementation.**

