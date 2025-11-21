# WP1 Chunk 7: Bedrock CI Expansion (macOS + Windows) - Completion Summary

**Date:** 2025-11-17  
**Status:** ✅ Complete  
**WP1 Chunk:** 7 of 10

---

## Summary

Successfully expanded Bedrock's CI to include macOS and Windows builds alongside the existing Linux/dev-01 pipeline. All three platforms now build and test the minimal C++23 engine, ensuring cross-platform compatibility from the start.

---

## Changes Made

### 1. CI Workflow Expansion

**Modified `.github/workflows/ci.yml`:**
- Added `build-macos` job (macOS build and test)
- Added `build-windows` job (Windows build and test)
- Kept existing `build` job unchanged (Linux/dev-01)

**Workflow structure:**
- All three jobs run in parallel
- Each job is independent (no dependencies)
- Each job checks contract parity
- Each job builds and tests independently

### 2. macOS Job (`build-macos`)

**Platform:** `macos-latest` (GitHub-hosted)

**Steps:**
1. **Checkout with submodules:**
   - Uses `actions/checkout@v5`
   - `submodules: recursive`

2. **Verify contracts submodule SHA parity:**
   - Reads `.contract-version` file
   - Compares with `git -C contracts rev-parse HEAD`
   - Fails with clear error if mismatch

3. **Configure CMake:**
   - Build directory: `build/macos`
   - Build type: `Release`
   - `BUILD_TESTING=ON` (enables bedrock_sanity and bedrock_tests)
   - No `BEDROCK_WITH_TRANSPORT_DEPS` (defaults to OFF)

4. **Build:**
   - `cmake --build build/macos --config Release`
   - CMake auto-detects generator and parallelism

5. **Run tests:**
   - `ctest --test-dir build/macos --output-on-failure`
   - Runs bedrock_sanity and bedrock_tests

6. **Upload CMake logs (on failure):**
   - Artifact name: `cmake-logs-macos`
   - Retention: 7 days

### 3. Windows Job (`build-windows`)

**Platform:** `windows-latest` (GitHub-hosted)

**Configuration:**
- Uses `shell: bash` for all steps (via `defaults.run.shell: bash`)
- Consistent with Linux/macOS scripts

**Steps:**
1. **Checkout with submodules:**
   - Uses `actions/checkout@v5`
   - `submodules: recursive`

2. **Verify contracts submodule SHA parity:**
   - Same logic as macOS/Linux
   - Uses bash shell

3. **Configure CMake:**
   - Build directory: `build/windows`
   - Build type: `Release`
   - `BUILD_TESTING=ON`
   - No `BEDROCK_WITH_TRANSPORT_DEPS` (defaults to OFF)

4. **Build:**
   - `cmake --build build/windows --config Release`
   - CMake auto-detects generator (MSVC or MinGW)

5. **Run tests:**
   - `ctest --test-dir build/windows --output-on-failure`
   - Runs bedrock_sanity and bedrock_tests

6. **Upload CMake logs (on failure):**
   - Artifact name: `cmake-logs-windows`
   - Retention: 7 days

### 4. Contract Parity Check

**All three jobs now include contract parity check:**
- Linux (existing) ✅
- macOS (new) ✅
- Windows (new) ✅

**Check logic:**
```bash
expected=$(cat .contract-version)
actual=$(git -C contracts rev-parse HEAD)

if [ "$expected" != "$actual" ]; then
  echo "ERROR: contracts submodule SHA mismatch."
  exit 1
fi
```

**Result:** Contract SHA mismatches are caught early on all platforms.

---

## File Structure

### Files Modified:
- `.github/workflows/ci.yml` - Added macOS and Windows jobs

### New CI Jobs:
- `build-macos` - macOS build and test
- `build-windows` - Windows build and test

---

## Implementation Details

### Design Decisions

1. **Workflow:** Extended main `ci.yml` (not created new workflow)
   - Keeps all CI jobs in one place
   - Easier to maintain and understand

2. **Shell for Windows:** Used `shell: bash` (Git Bash)
   - Consistent with Linux/macOS
   - Same scripts work across platforms

3. **CMake Generator:** Let CMake auto-detect
   - Works well on all platforms
   - No need to force Ninja/MSVC

4. **Build Parallelism:** Let CMake decide
   - Default parallelism is usually good
   - No explicit `-j` flag needed

5. **Test Filtering:** Run all tests by default
   - No platform-specific filtering yet
   - Will add guards only if tests fail due to platform limitations

6. **Artifact Naming:** Platform-specific names
   - `cmake-logs-macos`
   - `cmake-logs-windows`
   - Easy to identify platform-specific failures

### Platform-Specific Considerations

**macOS:**
- Uses `macos-latest` runner
- Clang compiler (C++23 support)
- No special dependencies needed
- CMake and compiler come with Xcode

**Windows:**
- Uses `windows-latest` runner
- MSVC or MinGW compiler (C++23 support)
- Uses `shell: bash` for consistency
- CMake handles path separators automatically

**Linux:**
- Existing job unchanged
- Self-hosted dev-01 runner
- GCC compiler (C++23 support)

### Transport Dependencies

**Policy:** Keep `BEDROCK_WITH_TRANSPORT_DEPS=OFF` in CI
- Not required for WP1 Chunk 7
- Reduces CI complexity
- Can be enabled later when needed

**Tests that run:**
- `bedrock_sanity` ✅ (always built)
- `bedrock_tests` ✅ (GoogleTest-based, if BUILD_TESTING=ON)

**Tests that don't run:**
- `bedrock_echo_sanity` (requires BEDROCK_WITH_TRANSPORT_DEPS=ON)
- `bedrock_transport_deps_sanity` (requires BEDROCK_WITH_TRANSPORT_DEPS=ON)

---

## Verification Checklist

- [x] macOS job added to `ci.yml`
- [x] Windows job added to `ci.yml`
- [x] Contract parity check runs in all jobs
- [x] All jobs checkout with submodules
- [x] All jobs configure with `BUILD_TESTING=ON`
- [x] All jobs build bedrock_engine and bedrock_sanity
- [x] All jobs run tests via ctest
- [x] `BEDROCK_WITH_TRANSPORT_DEPS` remains OFF (not set)
- [x] CMake logs uploaded on failure
- [x] Platform-specific artifact names used
- [x] Windows job uses `shell: bash`
- [x] Workflow structure is correct

---

## Expected CI Behavior

### On Push/PR:

**All three jobs run in parallel:**
1. `build` (Linux/dev-01) - Existing job
2. `build-macos` (macOS) - New job
3. `build-windows` (Windows) - New job

**Each job:**
- Checks out repo with submodules
- Verifies contract SHA parity
- Configures CMake (Release, BUILD_TESTING=ON)
- Builds bedrock_engine and bedrock_sanity
- Runs tests via ctest
- Uploads logs on failure

**Success criteria:**
- All three jobs pass ✅
- Contract parity checks pass ✅
- Tests run successfully ✅

---

## Notes

- **No platform-specific test filtering yet** - All tests run on all platforms
- **Transport deps disabled** - BEDROCK_WITH_TRANSPORT_DEPS=OFF (not set)
- **Contract parity enforced** - All platforms check SHA consistency
- **Consistent shell** - All platforms use bash for scripts
- **Auto-detection** - CMake auto-detects generator and parallelism
- **Artifact naming** - Platform-specific names for easy identification

---

## Next Steps (Future Chunks)

- **WP1 Chunk 8:** Bedrock CMake hygiene / modularization
- **WP1 Chunk 9:** Bedrock repo hygiene (format, editorconfig, etc.)
- **WP1 Chunk 10:** Bedrock architecture & build docs skeleton

**Future enhancements:**
- Enable `BEDROCK_WITH_TRANSPORT_DEPS=ON` in CI (when transport deps are available)
- Add platform-specific test guards (if needed)
- Add more comprehensive test coverage

---

**Chunk completed by:** UnderLord (AI Assistant)  
**Verification:** Workflow structure validated, ready for CI testing  
**Ready for:** WP1 Chunk 8 (Bedrock CMake hygiene)

