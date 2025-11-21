# WP0 Chunk 0.2: Linux CI Workflow for Bedrock - Completion Summary

**Date:** 2025-11-17  
**Status:** ✅ Complete  
**Workflow:** `.github/workflows/ci-linux-phase0.yml`

---

## Summary

Successfully added a Linux GitHub Actions CI workflow for Bedrock Phase 0 that builds the minimal C++23 `bedrock_engine` and `bedrock_sanity` targets, and runs the sanity test via CTest. The workflow is ready to run on `ubuntu-latest` for all pushes and pull requests to `main`.

---

## Changes Made

### 1. CTest Integration

**Updated `CMakeLists.txt`:**
- Added `enable_testing()` to enable CTest framework
- Added `add_test(NAME bedrock_sanity COMMAND bedrock_sanity)` to register the sanity executable as a test

**Location:** After `bedrock_sanity` executable definition (lines 147-148)

**Verification:**
- ✅ Tested locally: `ctest --output-on-failure` runs successfully
- ✅ Test passes: `bedrock_sanity` returns exit code 0
- ✅ CTest reports: "100% tests passed, 0 tests failed out of 1"

### 2. Linux CI Workflow

**Created `.github/workflows/ci-linux-phase0.yml`:**

**Triggers:**
- `push` to `main` branch
- `pull_request` targeting `main` branch
- `workflow_dispatch` (manual trigger)

**Runner:** `ubuntu-latest`

**Steps:**
1. **Checkout repository** - Uses `actions/checkout@v5`
2. **Install dependencies** - Installs `build-essential` and `cmake` via apt-get
3. **Print tool versions** - Displays GCC and CMake versions for debugging
4. **Configure** - Runs `cmake -S . -B build/ci -DCMAKE_BUILD_TYPE=Release`
5. **Build** - Runs `cmake --build build/ci --config Release` (builds both `bedrock_engine` and `bedrock_sanity`)
6. **Run tests** - Runs `ctest --output-on-failure` from `build/ci` directory
7. **Upload logs on failure** - Uploads CMake logs as artifacts if build/test fails

**Key Features:**
- Uses `build/ci` directory as specified
- Builds in `Release` mode (consistent with existing CI)
- Runs CTest for test execution (standard approach)
- Includes artifact upload for debugging failures

---

## Workflow Structure

```yaml
name: CI Linux Phase 0

on:
  push:
    branches: [ "main" ]
  pull_request:
    branches: [ "main" ]
  workflow_dispatch:

jobs:
  build-and-test:
    name: Ubuntu build (Phase 0 minimal)
    runs-on: ubuntu-latest
    steps:
      - Checkout
      - Install dependencies
      - Print versions
      - Configure CMake
      - Build targets
      - Run CTest
      - Upload logs (on failure)
```

---

## Local Testing

**Tested locally on dev-01:**
```bash
cd /home/ec2-user/workspace/bedrock
rm -rf build/test-ci
cmake -S . -B build/test-ci -DCMAKE_BUILD_TYPE=Release
cmake --build build/test-ci --config Release
cd build/test-ci
ctest --output-on-failure
```

**Results:**
- ✅ CMake configuration: Success
- ✅ Build: Success (bedrock_engine + bedrock_sanity built)
- ✅ CTest: All tests passed (1/1)
- ✅ Exit code: 0

---

## Existing Workflows Status

**Left unchanged (as instructed):**
- ✅ `.github/workflows/ci.yml` - Self-hosted dev01 runner (different purpose)
- ✅ `.github/workflows/codeql.yml` - CodeQL analysis on dev01 (different purpose)
- ✅ Other workflows (pr-guard, auto-merge, etc.) - Unchanged

**New workflow:**
- ✅ `.github/workflows/ci-linux-phase0.yml` - Phase 0 Linux CI on ubuntu-latest

---

## Verification Checklist

- [x] CTest integration added to CMakeLists.txt
- [x] Workflow file created: `.github/workflows/ci-linux-phase0.yml`
- [x] Workflow triggers on push/PR to main
- [x] Uses `ubuntu-latest` runner
- [x] Checks out repository
- [x] Installs dependencies (CMake, compiler)
- [x] Configures CMake in `build/ci` directory
- [x] Builds `bedrock_engine` and `bedrock_sanity`
- [x] Runs tests via `ctest --output-on-failure`
- [x] Workflow YAML syntax validated
- [x] Local testing successful
- [x] Changes committed
- [x] No old/legacy targets reintroduced
- [ ] **TODO:** Push to GitHub and verify workflow runs in Actions tab (requires push to remote)

---

## Next Steps

1. **Push to GitHub:** Commit is ready on branch `fix/workflow-improvements`
2. **Verify in Actions:** Once pushed, workflow should appear in GitHub Actions tab
3. **Monitor first run:** Ensure workflow passes on first execution
4. **Future:** Consider making workflow a required check for PRs (separate admin task)

---

## Notes

- **CTest approach:** Used CTest integration (standard approach) rather than direct execution
- **Build directory:** Uses `build/ci` as specified (separate from existing `build/` used by dev01 CI)
- **Dependencies:** Minimal - only CMake and compiler (Ubuntu image provides these)
- **Scope:** Kept minimal - no old targets, no palantir submodule changes
- **Branch protection:** Not modified (as instructed - will be separate task)

---

## Workflow File Location

- **Path:** `.github/workflows/ci-linux-phase0.yml`
- **Job name:** `build-and-test`
- **Job display name:** "Ubuntu build (Phase 0 minimal)"

---

**Chunk completed by:** UnderLord (AI Assistant)  
**Local verification:** All checks passed  
**Ready for:** Push to GitHub and workflow execution verification

