# DesignOpticsFast Changelog

All notable changes to the **Phoenix** and **Bedrock** projects are documented here.  
This changelog follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) conventions and uses
[Semantic Versioning](https://semver.org/).

---

## [0.0.4] – 2025-01-25
### Bedrock (Backend)
- **Envelope-Based Palantir Transport**: Migrated to envelope-based wire format for all IPC communication. All messages now use `MessageEnvelope` protobuf with version, type, payload, and metadata fields. Wire format: `[4-byte length][serialized MessageEnvelope]`. Replaced legacy `[length][type][payload]` format.
- **Deadlock Fix**: Eliminated deadlock in `parseIncomingData()` by refactoring to `extractMessage()` helper (no locking) and narrowing lock scope in buffer manipulation. Removed mutex from `sendMessage()` to prevent nested locking. Transport layer now safe under concurrency.
- **Envelope Helpers**: Added reusable `makeEnvelope()` and `parseEnvelope()` helpers for creating and parsing envelope messages. Helpers include version validation, type checking, and error reporting.
- **Integration Test Harness**: Added comprehensive integration test framework with `IntegrationTestServerFixture` (in-process server) and `IntegrationTestClient` (minimal test client). Framework validates end-to-end envelope transport for RPCs.
- **Capabilities RPC Integration Test**: End-to-end test validating Capabilities request/response cycle using envelope transport. Test confirms server processes requests and sends responses correctly.
- **XY Sine RPC Integration Test**: End-to-end test validating XY Sine computation with mathematical correctness checks. Test validates envelope transport for numeric RPCs with repeated double fields and confirms server computation matches expected sine wave formula.

### Phoenix (Frontend)
- **Envelope-Based Palantir Protocol**: Client-side migration to envelope-based protocol matching Bedrock server. All RPCs now use envelope-based transport with proper versioning and extensibility.
- **IPC Hardening**: Eliminated deadlocks in transport layer by refactoring lock scope. Transport layer now safe under concurrency.
- **Envelope Helpers**: Added reusable `makeEnvelope()` and `parseEnvelope()` helpers matching server-side implementation.

---

## [v0.1.0-sprint1] – 2025-10-08
### Status
**Sprint 1 Complete — Baseline Release**

This release closes Sprint 1 and establishes the first cross-platform stable baseline of the DesignOpticsFast toolchain.

### Phoenix (Frontend)
- Implemented lifecycle-correct Linux file-dialog handling  
  – Pre-app environment detection (`PlatformGuard`) for Wayland, Flatpak/Snap, VirtualBox.  
  – Runtime D-Bus portal health check post-`QApplication`.  
  – Automatic fallback to Qt’s non-native dialogs when portals are unavailable.  
  – CLI flag `--force-fallback-dialogs` for CI/headless runs.  
- Verified dialog stability across:
  - Ubuntu 24.04 Wayland (native portal path)  
  - Ubuntu 24.04 VirtualBox (XCB forced)  
  - CI (headless `xvfb-run`)  
  - Windows 11 and macOS Sonoma (unchanged)  
- STEP export confirmed functional in all Linux variants.  
- Updated CMake to link `Qt6::DBus`; added `$ORIGIN` RPATH for Linux.  
- Introduced structured logging (`qInfo/qWarning`) for platform decisions.

### Bedrock (Engine)
- Ensured compatibility with OCCT 7.7 and TBB linking on Ubuntu 24.04.  
- Verified STEP export and inter-process communication with Phoenix.  
- No functional changes required for Sprint 1 code; environment stabilization only.

### Infrastructure / Tooling
- Linux VM re-provisioned with:
  - Qt 6.10.1 via aqtinstall  
  - `xdg-desktop-portal` stack installed  
  - Guest Additions (3D accel, bidirectional clipboard)  
- Build validated via CMake + Ninja on AWS Ubuntu 24.04 and VirtualBox Ubuntu 24.04.  
- Repository hygiene:
  - Branches merged and closed  
  - Tags `v0.1.0-sprint1` pushed for Phoenix and Bedrock  

### Summary
> **Result:** Phoenix and Bedrock build and run cleanly on Linux, Windows, and macOS.  
> **Next Step:** Begin Sprint 2 (UnderLord Integration and Automated AI Development Workflow).

---
