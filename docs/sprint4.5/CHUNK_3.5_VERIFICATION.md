# Chunk 3.5 Verification - CI Integration: Build + Unit + Integration Tests

**Date:** 2025-01-26  
**Chunk:** 3.5 - CI Integration: Build + Unit + Integration Tests  
**Status:** ✅ Complete

---

## Summary of CI Changes

### Bedrock CI Workflow

**File:** `bedrock/.github/workflows/ci.yml`

**Changes:**
- ✅ Replaced `integration-tests` job with `tests` job
- ✅ Added matrix strategy for `ubuntu-latest` and `macos-latest`
- ✅ Updated to run all tests via `ctest` (not just 2 specific integration tests)
- ✅ Added unit tests to the test run
- ✅ Configured with `BEDROCK_WITH_TRANSPORT_DEPS=ON` and `BUILD_TESTING=ON`

**Jobs:**
- ✅ `build` - Ubuntu build (engine-only, no transport deps) - unchanged
- ✅ `tests` - Tests (Unit + Integration) - runs on both Ubuntu and macOS

**Test job steps (both platforms):**
1. Checkout repo with submodules
2. Install dependencies (cmake, ninja, g++, libtbb-dev, libgtest-dev, libomp-dev on Linux; cmake, ninja, libomp on macOS)
3. Install Qt 6.10.1
4. Setup Qt environment (CMAKE_PREFIX_PATH)
5. Configure CMake with transport deps enabled
6. Build all tests
7. Run all tests via `ctest --output-on-failure`
8. Upload test logs on failure

### Phoenix CI Workflow

**File:** `phoenix/.github/workflows/ci.yml`

**Changes:**
- ✅ Updated `build` job to use matrix strategy for `ubuntu-latest` and `macos-latest`
- ✅ Added `PHX_WITH_TRANSPORT_DEPS=ON` to CMake configuration
- ✅ Added `BUILD_TESTING=ON` to CMake configuration
- ✅ Renamed "Unit tests" step to "Run all tests (ctest)" to reflect it runs all tests
- ✅ Updated gate job to handle matrix build results

**Jobs:**
- ✅ `build` - Build + Tests - runs on both Ubuntu and macOS (matrix strategy)
- ✅ Other jobs (docs, ci_checks, etc.) - unchanged

**Build job steps (both platforms):**
1. Checkout repo with submodules
2. Verify contracts submodule SHA parity
3. Guard checks (no stale LogCategories includes)
4. Install Qt 6.10.1
5. Verify Qt installation
6. Configure CMake with transport deps enabled
7. Build
8. Translation drift guard
9. Run all tests via `ctest --test-dir build/relwithdebinfo --output-on-failure`

---

## Test Coverage in CI

### Bedrock Tests Run in CI

**Unit tests:**
- ✅ `EnvelopeHelpers_test` (19 tests from Chunk 3.1)
- ✅ `ErrorResponse_test` (10 tests from Chunk 3.2)
- ✅ `CapabilitiesService_test` (existing)

**Integration tests:**
- ✅ `CapabilitiesIntegrationTest` (existing)
- ✅ `XYSineIntegrationTest` (existing)
- ✅ `ErrorCasesIntegrationTest` (6 tests from Chunk 3.3)
- ✅ `EdgeCasesIntegrationTest` (5 tests from Chunk 3.4)

**Total:** All unit and integration tests run via `ctest`

### Phoenix Tests Run in CI

**Unit tests:**
- ✅ `envelope_helpers_test` (19 tests from Chunk 3.1)
- ✅ `error_mapping_test` (16 tests from Chunk 3.2)
- ✅ All existing unit tests (feature_registry_tests, canonical_json_tests, etc.)

**Total:** All unit tests run via `ctest`

---

## Verification

### Workflow Syntax

✅ **Bedrock:**
- YAML syntax valid
- Matrix strategy correctly configured
- All steps properly indented
- No linter errors

✅ **Phoenix:**
- YAML syntax valid
- Matrix strategy correctly configured
- All steps properly indented
- No linter errors

### Expected CI Behavior

**Bedrock:**
- ✅ On push/PR to main: `build` job runs (Ubuntu, engine-only)
- ✅ On push/PR to main: `tests` job runs on both Ubuntu and macOS
- ✅ `tests` job builds with `BEDROCK_WITH_TRANSPORT_DEPS=ON`
- ✅ `tests` job runs all tests via `ctest --output-on-failure`
- ✅ Test logs uploaded on failure

**Phoenix:**
- ✅ On push/PR to main (with code changes): `build` job runs on both Ubuntu and macOS
- ✅ `build` job builds with `PHX_WITH_TRANSPORT_DEPS=ON`
- ✅ `build` job runs all tests via `ctest --test-dir build/relwithdebinfo --output-on-failure`
- ✅ Gate job checks that all matrix jobs succeeded

### Local Simulation

**Note:** CI workflows can be simulated locally by running the same commands:

**Bedrock (Linux):**
```bash
cd bedrock
mkdir build && cd build
cmake -S .. -B . -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBEDROCK_WITH_OCCT=OFF \
  -DBEDROCK_WITH_TRANSPORT_DEPS=ON \
  -DBUILD_TESTING=ON
cmake --build . -j
ctest --output-on-failure
```

**Phoenix (Linux):**
```bash
cd phoenix
mkdir build && cd build
cmake -S .. -B . -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DPHX_WITH_TRANSPORT_DEPS=ON \
  -DBUILD_TESTING=ON
cmake --build . -j
ctest --test-dir . --output-on-failure
```

---

## Behavior Confirmation

✅ **No production behavior changed:**
- Only CI workflow files were modified
- No changes to build system or test code
- No changes to test execution logic

✅ **CI now builds and runs all tests:**
- ✅ Bedrock: Unit tests (EnvelopeHelpers, ErrorResponse) + Integration tests (Capabilities, XY Sine, ErrorCases, EdgeCases)
- ✅ Phoenix: Unit tests (envelope_helpers, error_mapping, all existing tests)
- ✅ Both repos run tests on Linux and macOS

✅ **Tests run via ctest:**
- ✅ All tests discovered and run automatically
- ✅ `--output-on-failure` provides detailed output on failures
- ✅ Test logs uploaded as artifacts on failure

✅ **No coverage or sanitizers:**
- ✅ No coverage flags added (that's Chunk 3.6)
- ✅ No TSAN/ASAN flags added
- ✅ Simple Release/RelWithDebInfo builds only

---

## Files Modified

**Bedrock:**
- `bedrock/.github/workflows/ci.yml` - Updated `integration-tests` job to `tests` job with matrix strategy

**Phoenix:**
- `phoenix/.github/workflows/ci.yml` - Updated `build` job to use matrix strategy and enable transport deps

**No production code changes:**
- No changes to CMakeLists.txt files
- No changes to test code
- No changes to build configuration

---

## CI Job Summary

### Bedrock

| Job | Platform | Build Type | Tests Run |
|-----|----------|------------|-----------|
| `build` | Ubuntu | Release (engine-only) | None |
| `tests` | Ubuntu | Release (with transport deps) | All unit + integration |
| `tests` | macOS | Release (with transport deps) | All unit + integration |

### Phoenix

| Job | Platform | Build Type | Tests Run |
|-----|----------|------------|-----------|
| `build` | Ubuntu | RelWithDebInfo (with transport deps) | All unit tests |
| `build` | macOS | RelWithDebInfo (with transport deps) | All unit tests |

---

## Conclusion

**Status:** ✅ **CHUNK 3.5 COMPLETE**

**CI now builds and runs unit + integration tests for Bedrock and Phoenix on Linux and macOS, without coverage or sanitizers.**

**Coverage:**
- ✅ Bedrock: All unit tests (EnvelopeHelpers, ErrorResponse) + All integration tests (Capabilities, XY Sine, ErrorCases, EdgeCases)
- ✅ Phoenix: All unit tests (envelope_helpers, error_mapping, all existing tests)
- ✅ Both repos run on Linux and macOS
- ✅ Tests run via `ctest --output-on-failure`
- ✅ No coverage or sanitizers (that's Chunk 3.6)

**Next Step:** Ready for Chunk 3.6 (Coverage Measurement/Reporting)

---

**Verification Completed By:** UnderLord  
**Review Status:** Awaiting Mark/Lyra approval before proceeding to Chunk 3.6

