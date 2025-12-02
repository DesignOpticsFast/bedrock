# Sprint 4.5 Fixup – Implementation Plan

**Status:** Planning Complete, Awaiting Approval  
**Date:** 2025-01-26  
**Control Document:** `FIXUP_SPRINT_CONTROL.md`

---

## Executive Summary

This plan breaks down Sprint 4.5 Fixup into **manageable chunks** that can be completed, tested, and verified in single sessions. Each chunk is designed to be:

- **Self-contained**: Clear inputs, outputs, and success criteria
- **Testable**: Includes verification steps at completion
- **Reversible**: Changes can be reviewed and rolled back if needed
- **Focused**: One primary objective per chunk

**Total Chunks:** 28 chunks across 6 workstreams + Phase 0 + Final Verification

---

## UnderLord Operating Directives for Sprint 4.5

**These directives must be followed throughout the sprint to prevent meta-errors and maintain quality:**

- **Always work in the branch `sprint/4.5-fixup`** unless instructed otherwise.

- **After completing each chunk**, run all relevant tests (unit + integration).

- **After every Workstream 1 or Workstream 2 chunk**, re-run the full envelope tests.

- **Ask clarifying questions** if any ambiguity exists before coding.

- **Never modify CONTROL documents** (FIXUP_SPRINT_CONTROL.md, this plan, etc.).

- **Always produce a verification section** for each chunk showing how it was tested/verified.

- **Follow the chunk dependencies** - do not skip ahead to dependent chunks until prerequisites are complete.

---

## Current State Assessment

### ✅ Already Complete
- Envelope proto definitions exist in all repos
- Bedrock `PalantirServer` uses envelope-based `sendMessage()`
- Phoenix `LocalSocketChannel` has envelope-based `getCapabilities()`
- Integration test infrastructure exists (`IntegrationTestServerFixture`, `IntegrationTestClient`)
- Basic Capabilities and XY Sine integration tests exist

### ⚠️ Partially Complete
- Envelope protocol: Server-side complete, client-side incomplete (missing XY Sine RPC)
- Error handling: Basic structure exists, needs hardening
- Integration tests: Basic happy paths exist, error cases missing

### ❌ Not Started
- Phase 0 audits (lifetime, demo mode, C++23 planning)
- Legacy code removal verification
- Complete error case testing
- Documentation harmonization
- Versioning updates
- CI integration

---

## Phase 0 – Upfront Decisions & Audits

**Goal:** Lock decisions and audit current state before code changes.

> ⚠️ **HARD GATE: No Workstream 1 implementation may begin until all Phase 0 chunks are complete and approved.**
> 
> If UnderLord accidentally starts W1 early, the whole idea of a clean migration collapses. All Phase 0 chunks must be finished and reviewed before any code changes in Workstream 1.

### Chunk 0.1: MainWindow/Lifetime Audit (Phoenix + Bedrock)
**Scope:** Audit both Phoenix and Bedrock for lifetime issues  
**Files to Review:**
- `phoenix/src/ui/main/MainWindow.*`
- `phoenix/src/transport/*`
- `phoenix/src/analysis/*`
- `bedrock/src/palantir/PalantirServer.*`
- Search for: `static.*Engine`, `static.*Client`, `QFutureWatcher`, `new.*without parent`

**Deliverables:**
- `bedrock/docs/sprint4.5/LIFETIME_AUDIT.md` documenting findings
- If issues found: list in Workstream 2 backlog
- If clean: mark audit as complete

**Test:** Review-only, no code changes

---

### Chunk 0.2: Demo Mode Audit (Phoenix)
**Scope:** Verify demo mode status  
**Files to Review:**
- `phoenix/src/analysis/demo/XYSineDemo.*`
- Search for runtime demo mode flags/guards
- Check docs for demo mode references

**Deliverables:**
- `bedrock/docs/sprint4.5/DEMO_MODE_AUDIT.md`
- **Decision:** Hard-disable any runtime demo mode; keep XYSineDemo strictly as test harness
- Update docs in Workstream 5 if needed

**Test:** Review-only, no code changes

---

### Chunk 0.3: C++23 Upgrade Planning Document
**Scope:** Create planning doc for future C++23 upgrade  
**Deliverables:**
- `bedrock/docs/C++23_UPGRADE_PLAN.md` including:
  - Compiler versions on Crucible/dev-01/Windows
  - Desired C++23 features
  - Risks and migration considerations
  - Rough estimate for dedicated sprint

**Test:** Document review

---

### Chunk 0.4: ADR: Envelope-Based Palantir Framing
**Scope:** Create ADR documenting envelope decision  
**Deliverables:**
- `phoenix/docs/adr/ADR-0002-Envelope-Based-Palantir-Framing.md` (verify if exists, update if needed)
- Context → Decision → Consequences structure
- This locks in the wire format and "no backwards compat" rule

**Test:** Document review

---

> ✅ **MERGE-SAFE POINT #1:** After completing all Phase 0 chunks (0.1-0.4), the branch is in a stable state with all audits and decisions documented. Safe to merge if needed.

---

## Workstream 1 – Transport & IPC Correctness (Envelope-based)

**Goal:** Complete envelope implementation, remove legacy, normalize errors.

### Chunk 1.1: Verify Envelope Proto Consistency
**Scope:** Ensure envelope.proto is identical across repos  
**Files:**
- `bedrock/docs/palantir/proto/palantir/envelope.proto`
- `phoenix/contracts/proto/palantir/envelope.proto`
- `palantir/proto/palantir/envelope.proto`

**Actions:**
- Compare all three files
- Identify differences
- Align to single canonical version (prefer palantir repo as source of truth)
- Update CMake proto generation if needed

**Test:**
- All three files identical
- Proto generation succeeds in both Phoenix and Bedrock

> ✅ **MERGE-SAFE POINT #2:** After Chunk 1.1 (proto alignment), the protocol definitions are consistent across repos. Safe checkpoint.

---

### Chunk 1.2: Complete Phoenix XY Sine RPC Client
**Scope:** Implement envelope-based XY Sine RPC in Phoenix client  
**Files:**
- `phoenix/src/transport/LocalSocketChannel.hpp` (add method declaration)
- `phoenix/src/transport/LocalSocketChannel.cpp` (implement `sendXYSineRequest()`)

**Actions:**
- Add `sendXYSineRequest()` method following `getCapabilities()` pattern
- Use envelope helpers for encoding/decoding
- Handle error responses
- Match integration test expectations

**Test:**
- Unit test: `phoenix/tests/transport/LocalSocketChannel_test.cpp` (if exists)
- Integration test: Verify existing `XYSineIntegrationTest` passes
- Manual: Connect Phoenix to Bedrock, trigger XY Sine computation

---

### Chunk 1.3: Remove Legacy Framing from Bedrock (if any)
**Scope:** Verify no legacy `[length][type][payload]` code remains  
**Files:**
- `bedrock/src/palantir/PalantirServer.cpp`
- Search for: `readMessageWithType`, `sendMessageWithType`, legacy framing comments

**Actions:**
- Search codebase for legacy patterns
- Remove any legacy code paths
- Update comments if needed
- Ensure `extractMessage()` is envelope-only

**Test:**
- Code review: No legacy framing code exists
- Integration tests still pass

---

### Chunk 1.4: Normalize Error Semantics
**Scope:** Ensure consistent error handling across Phoenix and Bedrock  
**Files:**
- `bedrock/src/palantir/PalantirServer.cpp` (error response generation)
- `phoenix/src/transport/LocalSocketChannel.cpp` (error response parsing)

**Actions:**
- Verify `ErrorResponse` proto is used consistently
- Ensure all error paths send `ErrorResponse` with appropriate codes
- Document error code meanings
- Add error response parsing to Phoenix client

**Test:**
- Unit tests for error response encoding/decoding
- Integration test: Trigger error cases, verify responses

> ✅ **MERGE-SAFE POINT #3:** After Chunk 1.4 (error semantics), error handling is normalized and consistent. Safe checkpoint.

---

### Chunk 1.5: Implement Message Size Limits
**Scope:** Add and enforce MAX_MESSAGE_SIZE (10MB default)  
**Files:**
- `bedrock/src/palantir/PalantirServer.*` (add constant, enforce in `extractMessage()`)
- `phoenix/src/transport/LocalSocketChannel.*` (add constant, check before send)

**Actions:**
- Define `MAX_MESSAGE_SIZE = 10 * 1024 * 1024` constant
- Reject messages exceeding limit with `MESSAGE_TOO_LARGE` error
- Add size check in Phoenix before sending
- Add size check in Bedrock when reading length prefix

**Test:**
- Unit test: Send message > 10MB, verify rejection
- Integration test: Oversized message error case

---

### Chunk 1.6: Demo References Cleanup
**Scope:** Apply demo mode decision from Chunk 0.2  
**Actions:**
- Hard-disable any runtime demo mode code
- Keep XYSineDemo strictly as test harness
- Update docs to reflect reality

**Test:**
- Build succeeds
- Docs match code reality
- No runtime demo mode accessible

---

### Chunk 1.7: Feature Branch Cutover & Cleanup
**Scope:** Complete envelope migration on feature branch and merge  
**Branch:** `sprint/4.5-fixup`

**Actions:**
- Create feature branch: `sprint/4.5-fixup`
- Ensure both Phoenix and Bedrock are envelope-only (verify no legacy paths)
- Run full test suite + CI on the branch
- Merge to main when green
- Remove any dead code or TODOs tied to old framing
- Clean up any temporary workarounds

**Test:**
- All tests pass on feature branch
- CI green on feature branch
- Merge to main successful
- No legacy framing code remains

**Gate:** After W1, we have a stable envelope-only Palantir with clean error behavior and limits.

> ✅ **MERGE-SAFE POINT #4:** After Chunk 1.7 (full envelope cutover), the envelope migration is complete and merged to main. This is a major milestone - the protocol foundation is stable.

---

## Workstream 2 – Concurrency, Lifetime & Security

**Goal:** Ensure thread safety, proper lifetimes, and security.

### Chunk 2.1: PalantirServer Threading Audit & Documentation
**Scope:** Document threading model and verify thread safety  
**Files:**
- `bedrock/src/palantir/PalantirServer.*`
- `bedrock/docs/THREADING.md` (create or update)

**Actions:**
- Review mutex usage (`clientBuffersMutex_`, `jobMutex_`)
- Document thread model (main thread vs worker threads)
- Verify `QLocalSocket` usage is thread-safe (Qt signals/slots)
- Document locking strategy
- Add thread-safety comments to public methods

**Test:**
- TSAN run: `cd bedrock/build && ctest -R palantir -E .*` with TSAN
- Code review: Threading model documented

---

### Chunk 2.2: PalantirServer Input Validation
**Scope:** Add input validation at RPC boundaries  
**Files:**
- `bedrock/src/palantir/PalantirServer.cpp` (RPC handlers)

**Actions:**
- Validate `XYSineRequest` parameters (frequency > 0, samples > 0, etc.)
- Validate `CapabilitiesRequest` (should be empty, but verify)
- Early failure for invalid inputs with `INVALID_REQUEST` error
- Add bounds checking for array sizes

**Test:**
- Unit tests: Invalid request parameters → error response
- Integration test: Malformed payload error case

---

### Chunk 2.3: Phoenix Lifetime Fixes (if needed)
**Scope:** Fix lifetime issues found in Chunk 0.1  
**Actions:**
- Based on Chunk 0.1 findings:
  - Fix static singletons if blocking multi-window
  - Ensure `QFutureWatcher` cleanup
  - Parent all `new` objects appropriately
  - Verify client objects are cleaned up on disconnect

**Test:**
- Valgrind/ASAN: No leaks or UAFs
- Manual: Connect/disconnect multiple times, verify no leaks

---

### Chunk 2.4: Bedrock Threading Documentation
**Scope:** Create comprehensive THREADING.md  
**Files:**
- `bedrock/docs/THREADING.md` (create)

**Actions:**
- Document OpenMP/TBB usage patterns
- Identify thread-safe vs non-thread-safe components
- Document safe patterns for parallel computation
- Add examples of correct usage

**Test:**
- Document review
- Code examples compile and run

---

## Workstream 3 – Testing & CI Foundation

**Goal:** Comprehensive integration tests and CI integration.

### Chunk 3.1: Unit Tests – Envelope Framing
**Scope:** Unit tests for envelope encode/decode  
**Files:**
- `bedrock/tests/palantir/EnvelopeHelpers_test.cpp` (new or update)
- `phoenix/tests/transport/EnvelopeHelpers_test.cpp` (new or update)

**Test Cases:**
- Encode envelope from inner message
- Decode envelope to inner message
- Invalid envelope handling
- Version validation
- Metadata round-trip

**Test:**
- All unit tests pass
- Coverage ≥ 80% for envelope code

---

### Chunk 3.2: Unit Tests – Error Mapping
**Scope:** Unit tests for error response handling  
**Files:**
- `bedrock/tests/palantir/ErrorResponse_test.cpp` (new)

**Test Cases:**
- Error code to string mapping
- Error response serialization
- Error response parsing
- All error codes covered

**Test:**
- All unit tests pass

---

### Chunk 3.3: Expand Integration Tests – Error Cases
**Scope:** Add error case integration tests  
**Files:**
- `bedrock/tests/integration/ErrorCasesIntegrationTest.cpp` (new)

**Test Cases:**
- Unknown message type
- Oversized message (> 10MB)
- Malformed payload (invalid protobuf)
- Server disconnect mid-RPC
- Invalid envelope version

**Test:**
- All error case tests pass
- Tests are stable (no flakiness)

---

### Chunk 3.4: Expand Integration Tests – Edge Cases
**Scope:** Add edge case integration tests  
**Files:**
- `bedrock/tests/integration/EdgeCasesIntegrationTest.cpp` (new)

**Test Cases:**
- Multiple concurrent clients
- Rapid connect/disconnect
- Large XY Sine requests (1000+ samples)
- Empty requests/responses

**Test:**
- All edge case tests pass
- No race conditions detected (TSAN)

---

### Chunk 3.5: CI Integration – Build + Integration Tests (EARLY)
**Scope:** Add basic CI jobs early to catch issues during implementation  
**Files:**
- `.github/workflows/ci.yml` (or equivalent CI config)

**Actions:**
- Add GitHub Actions jobs:
  - Linux: build + unit + integration tests
  - macOS: build + integration tests
- Initially without coverage or TSAN/ASAN to keep it simple
- Ensure integration tests run in CI

**Test:**
- CI green on test branch
- Integration tests run in CI logs
- Build succeeds on both platforms

---

### Chunk 3.6: Coverage Measurement & Reporting
**Scope:** Set up code coverage for Palantir code  
**Actions:**
- Configure coverage tool: **gcov/lcov** generating HTML reports in CI artifacts
- Set coverage target: ≥ 80% for Palantir code
- Add coverage reporting to CI
- Document coverage gaps
- Optional future: Codecov or similar (not required for this sprint's DoD)

**Test:**
- Coverage report generated
- Palantir code ≥ 80% coverage
- HTML reports available in CI artifacts

**Gate:** CI is green on Linux + macOS; tests are stable and coverage meets targets.

> ✅ **MERGE-SAFE POINT #5:** After Workstream 3 finishes, testing foundation is complete with CI integration. Safe checkpoint before documentation work.

---

## Workstream 4 – Toolchain & Versions

**Goal:** Normalize toolchain and version documentation.

### Chunk 4.1: Verify Qt 6.10.1 on Crucible
**Scope:** Confirm Qt 6.10.1 is installed and working  
**Actions:**
- Verify Qt installation: `/Users/underlord/Qt/6.10.1/`
- Test Phoenix build with Qt 6.10.1
- Verify Phoenix runs correctly
- Document Qt detection method

**Test:**
- Phoenix builds with Qt 6.10.1
- Phoenix launches and runs

---

### Chunk 4.2: Create VERSIONS.md (Single Source of Truth)
**Scope:** Create version documentation  
**Files:**
- `bedrock/docs/VERSIONS.md` (create)
- `phoenix/docs/VERSIONS.md` (create)

**Content:**
- Qt version: 6.10.1
- C++ standards: Phoenix C++17, Bedrock C++20
- CMake minimum version
- Protobuf version
- Compiler versions

**Test:**
- Documents created
- Version info accurate

---

### Chunk 4.3: Update Docs to Reference VERSIONS.md
**Scope:** Replace hard-coded versions with VERSIONS.md references  
**Files:**
- All docs mentioning versions (search for "Qt 6", "C++17", etc.)

**Actions:**
- Find all version references
- Replace with "See VERSIONS.md"
- Update build instructions

**Test:**
- No conflicting version references
- All docs point to VERSIONS.md

---

### Chunk 4.4: Verify Bedrock C++20 Consistency
**Scope:** Ensure C++20 is consistent across CMake, docs, code  
**Files:**
- `bedrock/CMakeLists.txt`
- `bedrock/docs/*.md`
- Code that uses C++20 features

**Actions:**
- Verify CMake sets C++20
- Verify docs say C++20
- Verify code uses C++20 features appropriately
- Document any C++17 compatibility requirements

**Test:**
- Bedrock builds with C++20
- No C++17-only code patterns

---

## Workstream 5 – Documentation Harmonization

**Goal:** Align documentation across repos.

### Chunk 5.0: Early Light Coherence Scan
**Scope:** Quick pass to identify obviously out-of-date or contradictory docs  
**Actions:**
- Quick grep-based pass across Phoenix, Bedrock, Palantir docs
- Identify obviously out-of-date or contradictory references
- Log findings as items for Chunks 5.1–5.3
- No big edits yet, just identification

**Deliverables:**
- `bedrock/docs/sprint4.5/DOC_COHERENCE_ISSUES.md` (list of contradictions to fix)

**Test:**
- Issues identified and logged
- Ready for systematic fixes in 5.1–5.3

---

### Chunk 5.1: Phoenix Documentation Updates
**Scope:** Update Phoenix docs per control doc  
**Files:**
- `phoenix/docs/VERSIONS.md` (from Chunk 4.2)
- `phoenix/docs/DEVELOPMENT_ENVIRONMENTS.md` (create)
- `phoenix/docs/INDEX.md` (create or update)
- Update existing docs with correct paths, Qt versions

**Actions:**
- Create DEVELOPMENT_ENVIRONMENTS.md (Crucible vs dev-01)
- Update demo mode docs (per Chunk 0.2)
- Normalize path references ($PHOENIX_ROOT style)
- Update Qt version references

**Test:**
- Docs are coherent
- No contradictions

---

### Chunk 5.2: Palantir/Data Contracts Documentation
**Scope:** Separate current vs future implementation  
**Files:**
- `bedrock/docs/palantir/README.md` (update)
- `bedrock/docs/palantir/IMPLEMENTATION_STATUS.md` (create)

**Actions:**
- Create implementation status table
- Document current: LocalSocket + protobuf
- Document future: Arrow Flight, gRPC, stats sidecar
- Add transport spec doc for future Flight work
- Add glossary

**Test:**
- Docs clearly separate current vs future
- Implementation status table accurate

---

### Chunk 5.3: Bedrock Documentation Expansion
**Scope:** Create/expand Bedrock docs per control doc  
**Files:**
- `bedrock/docs/BEDROCK_ARCHITECTURE.md` (create or update)
- `bedrock/docs/SOM_OVERVIEW.md` (create or update)
- `bedrock/docs/BEDROCK_BUILD.md` (create or update)
- `bedrock/docs/THREADING.md` (from Chunk 2.4)
- `bedrock/docs/TESTING.md` (create)
- `bedrock/docs/DEPLOYMENT.md` (create)
- `bedrock/docs/OPENCASCADE_INTEGRATION.md` (create)
- `bedrock/docs/REPOSITORY_STRUCTURE.md` (create)
- Resolve Python vs C++ coding standards confusion

**Actions:**
- Create missing docs
- Update existing docs
- Resolve coding standards confusion (document when to use Python vs C++)

**Test:**
- All listed docs exist and are accurate
- No contradictions

---

### Chunk 5.4: Cross-Repo Coherence Check (Formal)
**Scope:** Final cross-repo pass to ensure no contradictions remain  
**Actions:**
- Cross-check version references
- Cross-check architecture diagrams
- Cross-check protocol descriptions
- Cross-check transport descriptions
- Verify all issues from Chunk 5.0 are resolved
- Create final consistency report

**Test:**
- No contradictions found
- Final consistency report generated
- All issues from 5.0 resolved

---

## Workstream 6 – Versioning & Release Bookkeeping

**Goal:** Update versions and create release artifacts.

### Chunk 6.1: Bump Phoenix Version to 0.0.4
**Scope:** Update Phoenix version  
**Files:**
- `phoenix/version.h` or equivalent
- `phoenix/CMakeLists.txt` (if version defined there)
- About dialog/CLI output

**Actions:**
- Update version to 0.0.4
- Update About dialog if present
- Update CLI `--version` output if present
- Verify version string in running binary

**Test:**
- Version string shows 0.0.4
- About dialog shows 0.0.4

---

### Chunk 6.2: Update Phoenix CHANGELOG
**Scope:** Document Sprint 4.5 changes  
**Files:**
- `phoenix/CHANGELOG.md` (create or update)

**Content:**
- Envelope-based Palantir protocol
- IPC and concurrency hardening
- Integration tests & CI improvements
- Version bump to 0.0.4

**Test:**
- CHANGELOG entry complete
- Changes accurately described

---

### Chunk 6.3: Document Bedrock/Palantir Versions
**Scope:** Document version story for Bedrock/Palantir  
**Files:**
- `bedrock/docs/VERSIONS.md` (update from Chunk 4.2)
- `bedrock/docs/palantir/PROTOCOL_VERSIONS.md` (create)

**Actions:**
- Establish Bedrock versioning scheme: **SemVer** (e.g., 0.3.0 for this foundation hardening)
- Document protocol versions (pre-envelope vs envelope-based)
- Ensure Capabilities RPC reports version string
- Document versioning scheme in VERSIONS.md

**Test:**
- Version docs complete
- Capabilities RPC returns version
- Versioning scheme documented

---

### Chunk 6.4: Create Git Tags & Document Tagging Strategy
**Scope:** Tag releases and document tagging scheme  
**Actions:**
- Tag Phoenix: `phoenix-v0.0.4`
- Tag Bedrock: `bedrock-v0.3.0` (or appropriate version matching final protocol implementation)
- Document tagging strategy in `bedrock/docs/GIT_TAGGING.md` or similar

**Test:**
- Tags created
- Tagging strategy documented
- Tags point to correct commits

---

## Final Verification & Sprint Closure

**Goal:** Verify sprint completion and produce closure report.

> ✅ **MERGE-SAFE POINT #6:** Right before sprint closure (Chunk 7.1), all workstreams are complete. Final verification and report generation can be done on a clean branch state.

---

### Chunk 7.1: Sprint 4.5 Final Verification & Report
**Scope:** Final verification and sprint closure documentation  
**Actions:**
- Re-run full test suite and CI on main (after merge)
- Confirm ASAN/TSAN/Valgrind clean for Palantir and key flows
- Spot-check docs vs behavior (verify docs match running code)
- Verify version strings in binaries match docs
- Write `bedrock/docs/sprint4.5/SPRINT_4_5_REPORT.md` summarizing:
  - What changed (envelope protocol, hardening, tests, CI, docs, versions)
  - Risk and debt remaining
  - Readiness for C++23 upgrade sprint
  - Readiness for Thermal Diffusion implementation
  - Readiness for Phoenix C++20 + AnalysisWindow improvements

**Deliverables:**
- `bedrock/docs/sprint4.5/SPRINT_4_5_REPORT.md`

**Test:**
- All tests pass on main
- CI green on main
- ASAN/TSAN/Valgrind clean
- Docs match behavior
- Sprint report complete

---

## Chunk Dependencies

```
Phase 0 (All chunks) → Workstream 1 (explicit rule: no W1 code until Phase 0 complete)
Chunk 0.1 → Chunk 2.3 (if issues found)
Chunk 0.2 → Chunk 1.6
Chunk 1.1 → Chunk 1.2
Chunk 1.2 → Chunk 3.3, 3.4
Chunk 1.5 → Chunk 3.3
Chunk 1.7 → Chunk 7.1 (final verification after merge)
Chunk 2.1 → Chunk 2.4
Chunk 3.1, 3.2 → Chunk 3.5 (unit tests before CI)
Chunk 3.3, 3.4 → Chunk 3.5 (integration tests before CI)
Chunk 4.2 → Chunk 4.3
Chunk 4.2 → Chunk 6.3
Chunk 5.0 → Chunk 5.1-5.3 (early scan informs fixes)
Chunk 5.1-5.3 → Chunk 5.4 (formal check after fixes)
Chunk 6.1 → Chunk 6.2
Chunk 6.4 → Chunk 7.1 (tags before final report)
All Workstreams → Chunk 7.1 (final verification)
```

## Testing Strategy

Each chunk includes:
1. **Unit tests** (where applicable)
2. **Integration tests** (for transport/IPC chunks)
3. **Manual verification** (for docs/toolchain chunks)
4. **CI verification** (for CI-related chunks)

## Risk Mitigation

- **Envelope migration risk**: Feature branch approach allows rollback
- **Test flakiness**: Stable integration test infrastructure exists
- **Documentation drift**: Cross-repo coherence check (Chunk 5.4)
- **Version conflicts**: Single VERSIONS.md source of truth

## Decisions & Answers

### Feature Branch
- **Branch name:** `sprint/4.5-fixup`
- **Rationale:** Keeps sprint identity front and center; matches CONTROL language

### CI Platform
- **Platform:** GitHub Actions
- **Rationale:** As per Control Document and current repos

### Bedrock Versioning
- **Scheme:** Establish SemVer (e.g., 0.3.0 for this foundation hardening)
- **Documentation:** Document in `bedrock/docs/VERSIONS.md` and `PROTOCOL_VERSIONS.md`

### Coverage Tool
- **Baseline:** gcov/lcov generating HTML reports in CI artifacts
- **Optional future:** Codecov or similar (not required for this sprint's DoD)

### Git Tags
- **Phoenix:** Create `phoenix-v0.0.4` tag
- **Bedrock:** Create `bedrock-v0.3.0` tag (or appropriate version matching final protocol implementation)
- **Documentation:** Document tagging strategy in Chunk 6.4

### Demo Mode
- **Decision:** Hard-disable any runtime demo mode
- **XYSineDemo:** Keep strictly as test harness
- **Future:** If public demo mode is needed, it will be a deliberate feature in a future sprint

## Suggested Execution Order

1. **Phase 0 first:** Complete all audits before touching code
2. **Workstream 1:** Complete envelope migration on feature branch
3. **Workstream 2:** Harden concurrency and security
4. **Workstream 3:** Build testing foundation (unit tests → integration tests → CI)
5. **Parallel execution:** Workstreams 4, 5, and 6 can run in parallel once Workstreams 1-3 are stable
6. **Incremental testing:** Run integration tests after each Workstream 1 chunk to catch regressions early
7. **Early coherence scan:** Chunk 5.0 identifies contradictions before writing new docs
8. **CI early:** Chunk 3.5 sets up basic CI early to catch issues during implementation
9. **Final verification:** Chunk 7.1 ensures everything is complete before sprint closure

---

---

## Plan Summary

**Total Chunks:** 28 chunks
- Phase 0: 4 chunks (audits & decisions)
- Workstream 1: 7 chunks (envelope migration)
- Workstream 2: 4 chunks (concurrency & security)
- Workstream 3: 6 chunks (testing & CI)
- Workstream 4: 4 chunks (toolchain & versions)
- Workstream 5: 5 chunks (documentation harmonization)
- Workstream 6: 4 chunks (versioning & release)
- Final Verification: 1 chunk (sprint closure)

**Key Changes from Original Plan:**
- Added Chunk 1.7: Feature Branch Cutover & Cleanup
- Reordered Workstream 3: Unit tests first, then integration tests, then CI (earlier and simpler)
- Added Chunk 5.0: Early Light Coherence Scan
- Added Chunk 7.1: Final Verification & Sprint Closure
- Updated Chunk 0.1 to include Bedrock (not just Phoenix)
- All questions answered with explicit decisions

**Plan Status:** Ready for Review  
**Next Step:** Await approval to begin Phase 0

