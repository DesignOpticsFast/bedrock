# Sprint 4.5 – Doc Coherence Issues

**Date:** 2025-01-26  
**Purpose:** Track documentation issues identified during Workstream 5.0 Early Light Coherence Scan  
**Status:** ✅ **All issues resolved in Sprint 4.5 (Chunks 5.1–5.4)**

> **Note:** This file serves as a historical record of issues identified and resolved during Sprint 4.5 documentation harmonization. All issues listed below have been resolved in Chunks 5.1–5.4.

---

## Phoenix

### Critical Issues (Stale dev-01 References)

- [x] **File:** `phoenix/docs/CODING_STANDARDS_CPP_QT.md`
  - **Issue:** Section "1. Development Environment First" still states "ALL CODE CHANGES MUST BE DONE ON DEV-01 FIRST" with instructions for Tailscale, Xvfb, etc.
  - **Current State:** dev-01 has been permanently removed; all development is local + GitHub Actions CI
  - **Fix in 5.1:** ✅ **RESOLVED** - Removed dev-01 section, updated to "Development Environment" describing Crucible + CI model
  - **Lines:** 16-23

- [x] **File:** `phoenix/docs/DEVELOPMENT.md`
  - **Issue:** Entire file is about dev-01 preflight workflow. All content is stale:
    - "Dev-01 Preflight (Required)" section
    - References to `scripts/dev01-preflight.sh`
    - References to `make build-dev01`, `make clean-dev01`
    - "dev-01 is the canonical build environment" policy
    - Build directory `build/dev-01-relwithdebinfo`
  - **Current State:** dev-01 removed; CI runs on GitHub Actions; local development is primary
  - **Fix in 5.1:** ✅ **RESOLVED** - Repurposed as "Developer Diagnostics & Troubleshooting", removed all dev-01 content, preserved QML debugging and diagnostics sections
  - **Note:** File has useful QML debugging and diagnostics sections (lines 70-124) that should be preserved

### Stale Environment References

- [x] **File:** `phoenix/docs/MAC_DEVELOPMENT_ENVIRONMENT.md`
  - **Issue:** References Qt 6.9.3 and `/Users/mark/Qt/6.9.3/macos` paths
  - **Current State:** Qt 6.10.1 is baseline; paths should reference VERSIONS.md or use current version
  - **Fix in 5.1:** ✅ **RESOLVED** - Updated all Qt version references to use VERSIONS.md, sanitized all paths to use `$HOME/Qt/<version>/macos` with VERSIONS.md references
  - **Lines:** Multiple references throughout (Qt version, paths, examples)

### Protocol Documentation

- [x] **File:** `phoenix/README.md`
  - **Issue:** Protocol section (lines 154-162) describes legacy "framed binary protocol (`'PLTR'` magic)" with BigEndian header format
  - **Current State:** Phoenix uses envelope-based protocol (`MessageEnvelope` with `[4-byte length][serialized MessageEnvelope]`) per ADR-0002
  - **Fix in 5.1:** ✅ **RESOLVED** - Updated protocol description to envelope-based, added references to ADR-0002 and IPC_ENVELOPE_PROTOCOL.md
  - **Lines:** 154-162

- [x] **File:** `phoenix/docs/DEVELOPMENT.md`
  - **Issue:** Protocol section (lines 153-162) describes legacy "framed binary protocol (`'PLTR'` magic)" format
  - **Current State:** Envelope-based protocol is current
  - **Fix in 5.1:** ✅ **RESOLVED** - Updated protocol section to envelope-based, added references to ADR-0002 and IPC_ENVELOPE_PROTOCOL.md
  - **Lines:** 153-162

### Duplicate/Overlapping Documentation

- [x] **File:** `phoenix/docs/DEVELOPMENT.md` vs `phoenix/docs/DEVELOPMENT_WORKFLOW.md`
  - **Issue:** Two files with similar names and overlapping content:
    - `DEVELOPMENT.md` - Stale dev-01 content + some useful diagnostics
    - `DEVELOPMENT_WORKFLOW.md` - Current workflow (updated 2025-11-25)
  - **Fix in 5.1:** ✅ **RESOLVED** - Chose Option B: `DEVELOPMENT.md` repurposed as "Developer Diagnostics & Troubleshooting" with redirect to `DEVELOPMENT_WORKFLOW.md`, clear separation of concerns

---

## Bedrock

### Transport Protocol Confusion

- [x] **File:** `bedrock/README.md`
  - **Issue:** Line 22 says "Foundation: Qt 6.10 + Qt Graphs / gRPC UDS transport" but line 32 says "Palantir Server: LocalSocket + Protobuf (needs gRPC migration)"
  - **Current State:** Current implementation is LocalSocket + Protobuf with envelope-based protocol. gRPC UDS was validated in Gate 0.5 but not yet implemented.
  - **Fix in 5.3:** ✅ **RESOLVED** - Updated to "Foundation: C++20, LocalSocket + Protobuf (envelope-based Palantir protocol)", clear "Current Implementation" vs "Planned Enhancements" sections, gRPC marked as planned
  - **Lines:** 22, 32

### Incomplete/Placeholder Sections

- [x] **File:** `bedrock/README.md`
  - **Issue:** "Current Implementation" section mentions "needs gRPC migration" but doesn't clearly state what is actually implemented vs. planned
  - **Fix in 5.3:** ✅ **RESOLVED** - Added clear "Current Implementation (Sprint 4.5)" and "Planned Enhancements" sections, accurately reflects envelope-based LocalSocket implementation

---

## Palantir / Data Contracts

### Transport Documentation Confusion

- [x] **File:** `bedrock/docs/palantir/README.md`
  - **Issue:** "Transport & Binding" section (lines 50-55) describes Arrow Flight streams and Vega-Lite as if they are current implementation
  - **Current State:** Current transport is LocalSocket + Protobuf with envelope-based protocol (`MessageEnvelope`). Arrow Flight is planned for future.
  - **Fix in 5.2:** ✅ **RESOLVED** - Restructured with "Current Implementation" and "Future Phases (Not Implemented Yet)" sections, all planned features clearly marked
  - **Lines:** 50-55

- [x] **File:** `bedrock/docs/palantir/README.md`
  - **Issue:** No mention of current envelope-based protocol implementation
  - **Current State:** Envelope-based protocol is implemented and in use (Sprint 4.5)
  - **Fix in 5.2:** ✅ **RESOLVED** - Added "Current Implementation" section with complete envelope-based protocol documentation, wire format, MessageEnvelope structure, references to ADR-0002
  - **Note:** Should mention `MessageEnvelope` proto, `[4-byte length][serialized MessageEnvelope]` wire format

### Protocol History Clarity

- [x] **File:** `bedrock/docs/palantir/README.md`
  - **Issue:** No mention of protocol evolution (legacy vs. current vs. future)
  - **Fix in 5.2:** ✅ **RESOLVED** - Added "Legacy Protocol History" section documenting old `[length][type][payload]` format, why it was replaced, and references to ADR-0002

---

## Cross-Repo Issues

### Transport Description Consistency

- [x] **Issue:** Phoenix and Bedrock docs describe transport differently:
  - Phoenix: Some docs mention legacy "PLTR magic" protocol
  - Bedrock: Some docs mention gRPC UDS as "foundation" but implementation is LocalSocket
  - Palantir: README describes Arrow Flight as current
  - **Fix in 5.1-5.4:** ✅ **RESOLVED** - All repos now consistently describe:
    - **Current:** LocalSocket + Protobuf (envelope-based `MessageEnvelope`)
    - **Future:** Arrow Flight, gRPC (clearly marked as "Planned" / "Not Implemented")

---

## Summary Counts

- **Phoenix:** 6 issues
  - Critical: 2 (stale dev-01 references)
  - Stale environment: 1 (Qt version/paths)
  - Protocol: 2 (legacy protocol descriptions)
  - Duplicate docs: 1 (DEVELOPMENT.md vs DEVELOPMENT_WORKFLOW.md)

- **Bedrock:** 2 issues
  - Transport confusion: 2 (gRPC vs LocalSocket, incomplete sections)

- **Palantir:** 3 issues
  - Transport confusion: 2 (Arrow Flight described as current)
  - Missing current protocol: 1 (no envelope protocol mention)

- **Cross-Repo:** 1 issue
  - Transport consistency: 1 (different descriptions across repos)

**Total:** 12 issues logged

**Resolution Status:** ✅ **All 12 issues resolved in Chunks 5.1–5.4**

---

## Resolution Summary

### Chunk 5.1 (Phoenix) - ✅ Complete
- Fixed all 6 Phoenix issues:
  - Removed dev-01 references from CODING_STANDARDS_CPP_QT.md
  - Repurposed DEVELOPMENT.md as diagnostics guide
  - Updated MAC_DEVELOPMENT_ENVIRONMENT.md to use VERSIONS.md
  - Updated protocol descriptions in README.md and DEVELOPMENT.md
  - Consolidated DEVELOPMENT.md vs DEVELOPMENT_WORKFLOW.md

### Chunk 5.2 (Palantir) - ✅ Complete
- Fixed all 3 Palantir issues:
  - Restructured README.md with clear "Current Implementation" vs "Future Phases"
  - Added envelope-based protocol documentation
  - Added legacy protocol history section
  - Created IMPLEMENTATION_STATUS.md

### Chunk 5.3 (Bedrock) - ✅ Complete
- Fixed all 2 Bedrock issues:
  - Updated README.md with clear current vs planned separation
  - Removed gRPC confusion, marked as planned
  - Created comprehensive documentation suite (7 new files)

### Chunk 5.4 (Cross-Repo Coherence) - ✅ Complete
- Verified all issues resolved
- Confirmed cross-repo consistency:
  - Transport/protocol: All repos consistently describe envelope-based protocol
  - Toolchain/versions: All repos reference VERSIONS.md
  - Development model: All repos describe Crucible + CI consistently
  - Testing model: All repos describe unit + integration tests consistently

---

## Final Status

✅ **All 12 issues resolved**  
✅ **Cross-repo documentation coherent**  
✅ **No contradictions or stale references in active docs**  
✅ **Workstream 5 complete**

---

**This file serves as a historical record of issues identified and resolved during Sprint 4.5 documentation harmonization.**

