# WP0 Chunk 0.6: Preflight, README, .contract-version & CI Parity - Completion Summary

**Date:** 2025-11-17  
**Status:** ✅ Complete  
**WP0 Phase 0 Status:** ✅ **COMPLETE**

---

## Summary

Successfully completed the final chunk of WP0 Phase 0 by adding preflight script, updating README with Phase 0 documentation, creating `.contract-version` file, and adding CI parity check for contracts submodule. Bedrock Phase 0 is now formally complete and ready for WP1.

---

## Changes Made

### 1. Preflight Script Created

**Created `scripts/preflight.sh`:**
- **Location:** `scripts/preflight.sh` (matches existing scripts convention)
- **Shebang:** `#!/bin/bash` (consistent with other scripts)
- **Critical Checks (fail on missing):**
  - CMake version
  - C++ compiler (g++ or c++)
  - Contracts submodule (initialized and present)
- **Optional Checks (warn but don't fail):**
  - protoc version
  - grpc_cpp_plugin availability
- **Output:** Moderate verbosity (key info + clear pass/fail)
- **Exit Codes:** 0 = success, 1 = critical failure

**Example Output:**
```
=== Bedrock Preflight Check ===

✅ CMake: cmake version 3.22.2
✅ C++ Compiler: g++ (GCC) 11.5.0
✅ Contracts submodule: initialized
    ad0e9882cd3d9cbbf80fc3b4ac23cd1df7547f53 contracts (v1.0.0)
✅ protoc: libprotoc 3.19.6
✅ grpc_cpp_plugin: found

✅ Preflight OK - All checks passed
```

### 2. README Updated

**Updated `README.md` with Phase 0 state:**

**Current Scope Section:**
- Updated from "Phase 0.5 Gate PASS" to "Phase 0 Complete"
- Documented C++23 baseline enforcement
- Documented minimal engine + sanity tests
- Documented optional transport deps (behind CMake option)
- Documented Echo RPC implementation
- Documented contracts submodule status

**Build Instructions Section:**
- **Minimal Build:** Commands for Phase 0 basic build
- **Transport Dependencies Build:** Commands for dev-01 with transport deps
- Clear context for each build mode
- CTest usage examples

**Contracts Submodule Section:**
- Explained `contracts/` submodule purpose
- Documented repository (DesignOpticsFast/palantir)
- Documented current version (v1.0.0, commit `ad0e988...`)
- Explained `.contract-version` pinning mechanism
- Provided instructions for updating contracts submodule

**Preflight Section:**
- How to run `./scripts/preflight.sh`
- What checks it performs
- Integration into development workflow

**Repo Layout Section:**
- Updated to reflect Phase 0 structure
- Added new directories (proto/, scripts/, echo/)
- Documented `.contract-version` file

### 3. .contract-version File Created

**Created `.contract-version` at repo root:**
- **Content:** `ad0e9882cd3d9cbbf80fc3b4ac23cd1df7547f53` (40-char SHA)
- **Format:** Just SHA, no comments or tag names
- **Purpose:** Pins the expected contracts submodule commit
- **Verification:** Matches `contracts/` HEAD SHA exactly

### 4. CI Parity Check Added

**Added to `.github/workflows/ci.yml` (dev01 workflow):**
- **Placement:** After checkout, before build (fail fast)
- **Step Name:** "Verify contracts submodule SHA parity"
- **Logic:**
  - Reads `.contract-version` file
  - Gets actual `contracts/` HEAD SHA via `git -C contracts rev-parse HEAD`
  - Compares expected vs actual
  - Fails with clear error message if mismatch
- **Error Message:** Includes expected/actual SHAs and fix instructions
- **Checkout:** Updated to use `submodules: recursive` to ensure contracts is initialized

**Implementation:**
```bash
expected=$(cat .contract-version)
actual=$(git -C contracts rev-parse HEAD)

if [ "$expected" != "$actual" ]; then
  echo "ERROR: contracts submodule SHA mismatch."
  echo "Expected: $expected"
  echo "Actual:   $actual"
  echo "Fix: update contracts/ to the expected commit, or update .contract-version to match, in a coordinated change."
  exit 1
fi
```

---

## Verification Results

### Preflight Script

**Execution:**
```bash
./scripts/preflight.sh
```

**Result:**
- ✅ All critical checks pass (CMake, compiler, contracts submodule)
- ✅ Optional checks pass (protoc, grpc_cpp_plugin found)
- ✅ Exit code: 0
- ✅ Clear, readable output

### .contract-version Verification

**Check:**
```bash
cat .contract-version
git -C contracts rev-parse HEAD
```

**Result:**
- ✅ `.contract-version` contains: `ad0e9882cd3d9cbbf80fc3b4ac23cd1df7547f53`
- ✅ `contracts/` HEAD is: `ad0e9882cd3d9cbbf80fc3b4ac23cd1df7547f53`
- ✅ Perfect match

### CI Workflow

**ci.yml (dev01):**
- ✅ Parity check step added
- ✅ Checkout updated with `submodules: recursive`
- ✅ Step placed early (after checkout, before build)
- ✅ Clear error messages with fix instructions

**ci-linux-phase0.yml (ubuntu-latest):**
- ✅ Unchanged (as requested)
- ✅ Still passes with minimal build

---

## File Structure

```
bedrock/
├── scripts/
│   └── preflight.sh          # NEW: Environment preflight check
├── .contract-version         # NEW: Pinned contracts commit SHA
├── README.md                  # UPDATED: Phase 0 documentation
└── .github/workflows/
    └── ci.yml                # UPDATED: Added contracts SHA parity check
```

---

## Implementation Details

### Preflight Script Structure

```bash
#!/bin/bash
set -euo pipefail

# Critical checks (fail on missing)
- CMake
- C++ compiler
- Contracts submodule

# Optional checks (warn but don't fail)
- protoc
- grpc_cpp_plugin

# Exit codes
- 0 = success
- 1 = critical failure
```

### .contract-version Format

```
ad0e9882cd3d9cbbf80fc3b4ac23cd1df7547f53
```

- Just the 40-char SHA
- No comments or tag names
- Trailing newline OK

### CI Parity Check Logic

1. Read `.contract-version` file
2. Get `contracts/` HEAD SHA
3. Compare strings
4. Fail with clear error if mismatch
5. Pass silently if match

---

## Verification Checklist

- [x] `scripts/preflight.sh` created with environment checks
- [x] Preflight checks CMake (critical)
- [x] Preflight checks C++ compiler (critical)
- [x] Preflight checks contracts submodule (critical)
- [x] Preflight checks protoc (optional, warns)
- [x] Preflight checks grpc_cpp_plugin (optional, warns)
- [x] README.md updated with Phase 0 state
- [x] README includes minimal build instructions
- [x] README includes transport deps build instructions
- [x] README documents contracts submodule
- [x] README documents preflight usage
- [x] `.contract-version` file created
- [x] `.contract-version` contains contracts/ HEAD SHA
- [x] CI parity check added to `ci.yml` (dev01 workflow)
- [x] Parity check compares `.contract-version` to `contracts/` HEAD
- [x] Parity check fails on mismatch with clear error
- [x] Preflight script runs successfully on dev-01 (exit code 0)
- [x] `.contract-version` matches contracts/ HEAD SHA exactly
- [x] CI workflows remain functional

---

## Notes

- **Preflight Script:** Critical checks fail fast, optional tools warn but don't block
- **README:** Updated to reflect true Phase 0 state, removed outdated Phase 0.5 references
- **.contract-version:** Simple SHA pinning, no comments or metadata
- **CI Parity:** Enforces contract SHA consistency, prevents drift between `.contract-version` and `contracts/` submodule
- **CI Safety:** Parity check only added to dev01 workflow, ubuntu-latest CI unchanged
- **WP0 Complete:** All Phase 0 chunks complete, ready for WP1

---

## WP0 Phase 0 Completion Summary

**All Chunks Complete:**
- ✅ Chunk 0.1: Bedrock C++23 Reset & Layout
- ✅ Chunk 0.2: Linux CI Workflow for Bedrock
- ✅ Chunk 0.3: Palantir Contracts Submodule under contracts/
- ✅ Chunk 0.4: Protobuf, gRPC & libsodium Baseline
- ✅ Chunk 0.5: Echo RPC + Standalone Client
- ✅ Chunk 0.6: Preflight, README, .contract-version & CI Parity

**Phase 0 Deliverables:**
- C++23 baseline with minimal engine
- Linux CI on ubuntu-latest
- Contracts submodule integration
- Transport deps support (optional)
- Echo RPC implementation
- Preflight script
- Complete documentation
- Contract SHA parity enforcement

**Ready for:** WP1 (Week 1 / Sprint 4.2)

---

**Chunk completed by:** UnderLord (AI Assistant)  
**Verification:** All checks passed  
**WP0 Phase 0 Status:** ✅ **COMPLETE**

