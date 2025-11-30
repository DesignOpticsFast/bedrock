# SQ1_CPP23_FEASIBILITY.md — Bedrock C++23 Transition Feasibility Report

**Status:** DRAFT
**Author:** UnderLord
**Date:** 2025-11-30

---

## 1. Executive Summary

Initial investigation confirms **Feasibility on macOS (Crucible)**. The C++23 probe passes all checks. Bedrock compilation and basic execution work identically under C++23 as C++20.

**CI Status:** Triggered. Failures are expected and being logged for later investigation. No action taken yet.

---

## 2. Policy & Criteria

This investigation distinguishes between **Hard Blockers** (must fix environment or abandon) and **Non-Blockers** (workable debt).

### Hard Blockers
*   **Compiler ICE / Crash:** The compiler itself fails on valid C++23 code.
*   **Miscompilation:** Runtime corruption or incorrect behavior specific to C++23 optimization levels.
*   **Missing Critical Features:** Essential C++23 features (e.g., `std::expected`, `std::ranges`) are missing with no viable workaround on a target platform.

### Non-Blockers
*   **Missing Non-Critical Features:** Features we don't immediately need (e.g., `std::generator`, `std::modules`).
*   **New Warnings:** Clean-up tasks for future sprints.
*   **Build Time Regression:** Slower builds are acceptable within reason.
*   **Standard Library Gaps:** Missing features that can be polyfilled or avoided (e.g., `<print>` if missing).

---

## 3. Toolchain Matrix

| Environment | Platform | Compiler & Version | Stdlib Version | C++23 Support Status |
| :--- | :--- | :--- | :--- | :--- |
| **Crucible** | macOS 25.1.0 (Apple Silicon) | Apple Clang 17.0.0 (clang-1700.4.4.1) | Apple Libc++ | ✅ Full (Probe Passed) |
| **CI Linux** | Ubuntu 20.04 (Inferred from Audit) | GCC (Ver TBD) | *TBD* | *TBD* |
| **CI Windows** | Windows (Ver TBD) | *TBD* | *TBD* | *TBD* |
| **Dev Node** | Linux | *TBD (Pending Script)* | *TBD (Pending Script)* | *TBD* |
| **Deepthought** | Windows | *TBD (Pending Script)* | *TBD (Pending Script)* | *TBD* |

> **Note:** CI configurations are currently being inferred from workflow files. `bedrock`'s `ci.yml` uses self-hosted `dev01` which is a Linux node, potentially running an older GCC. `phoenix`'s `ci.yml` also uses `dev01`. Migration to `ubuntu-latest` is planned but not yet active.

---

## 4. C++23 Probe Results

### Probe Target: `cpp23_probe`
A minimal executable testing:
*   `std::expected`
*   `std::ranges` (views)
*   `if consteval`
*   `std::print` (feature guarded)
*   `std::mdspan` (feature guarded)

### Outcomes by Platform

| Feature | Crucible | CI Linux | CI Windows |
| :--- | :--- | :--- | :--- |
| **Compilation** | ✅ Pass | *TBD* | *TBD* |
| **std::expected** | ✅ Pass | *TBD* | *TBD* |
| **std::ranges** | ✅ Pass | *TBD* | *TBD* |
| **std::print** | ✅ Pass | *TBD* | *TBD* |
| **std::mdspan** | ✅ Pass | *TBD* | *TBD* |
| **if consteval** | ✅ Pass | *TBD* | *TBD* |

---

## 5. C++20 vs C++23 Build & Test Outcomes

### Build Times (Coarse)
*Comparing clean build times of `bedrock` target on Crucible.*

| Platform | C++20 Time | C++23 Time | Delta |
| :--- | :--- | :--- | :--- |
| **Crucible** | ~0.600s | ~0.584s | -0.016s (Negligible) |
| **CI Linux** | *TBD* | *TBD* | *TBD* |

### macOS — Tests & Runtime Validation

*   **Tests:** `bedrock_tests`
    *   **Summary:** 1/1 passed.
    *   **Failures:** None.
    *   **Notes:** Used existing test suite. `BedrockMath.Add` verified.

*   **Sample Job:** `bedrock_simple_job` (Engine instantiation + dummy design calculation)
    *   **Status:** ✅ OK
    *   **Runtime Differences:** None observed. Output identical between C++20 and C++23 builds.
    *   **Performance:** Subjectively identical (job is too small for micro-benchmarking).
    *   **Correctness:** No crashes, exceptions, or ABI issues detected.

---

## 6. Issues & Classification

### Hard Blockers
*   *None identified yet on macOS.*

### Non-Blockers / Warnings
*   *None identified yet on macOS.*

---

## 7. Recommendations & Next Steps

1.  **Push Branch to CI:** Trigger GitHub Actions to populate the Linux/Windows columns.
2.  **Run Sysinfo Scripts:** Execute `scripts/bedrock_sysinfo.sh` on Deepthought and Dev Node.
3.  **Evaluate CI Results:** If `dev01` (Linux) fails (likely if old GCC), prioritize the CI migration to `ubuntu-latest` (already planned in `CI_MIGRATION_PLAN.md`).

---

## 8. CI Initial Status

*   **Triggered:** Yes
*   **Branch:** `cxx23-side-quest`
*   **Status:** Failures are being logged for later investigation. No action taken yet.

