# Bedrock Sprint 4.5 Plan
## Remote Engine Hardening & Multi-Server Execution

**Date**: 2025-11-24  
**Status**: Planning Complete  
**Base Branch**: `main` → `sprint/4.5`

---

## Executive Summary

Bedrock Sprint 4.5 focuses on hardening the PalantirServer, implementing message envelope support, adding status/health endpoints, and implementing Thermal Diffusion computation with progress/cancel.

**Bedrock-Specific Goals**:
- Harden `PalantirServer` with message envelope and size limits
- Clean up threading and ownership model
- Add status/health RPC endpoint
- Implement Thermal Diffusion computation (OpenMP parallelization)
- Support progress reporting and cancel handling

---

## Branch Strategy

### Sprint Branch Creation

```bash
cd /Users/underlord/workspace/bedrock
git checkout main
git pull origin main
git checkout -b sprint/4.5
git push -u origin sprint/4.5
```

### Feature Branch Naming

Feature branches from `sprint/4.5`:
- `feature/wp2a-message-envelope`
- `feature/wp2a-server-hardening`
- `feature/wp2c-status-rpc`
- `feature/wp2d-thermal-diffusion`
- `feature/wp2e-cmake-modularization`

### PR Strategy

- **Per-WP PRs**: Each WP may have multiple feature branches merged to `sprint/4.5`, then one PR per WP
- **Final PR**: `sprint/4.5` → `main` (comprehensive review after all WPs)

---

## Work Package Breakdown

### WP2.A – Transport & Server Hardening (Bedrock Side)

**Goal**: Harden `PalantirServer` with message envelope, size limits, and error handling.

**Chunks**:

1. **WP2.A Chunk 1** – Update Palantir submodule to v1.3.0
   - Update `docs/palantir` submodule
   - Verify `envelope.proto` and `error.proto` are available
   - Update CMake proto codegen

2. **WP2.A Chunk 2** – Implement message envelope in PalantirServer
   - Update `handleMessage()` to parse `MessageEnvelope`
   - Extract `MessageType` and dispatch to appropriate handler
   - Update `sendMessage()` to wrap responses in `MessageEnvelope`
   - Backward compatibility: Handle old messages (no envelope) gracefully

3. **WP2.A Chunk 3** – Add message size limits
   - Reject messages > max size (configurable, default 10MB)
   - Send `ErrorResponse` for size limit violations
   - Add config option: `--max-message-size` (CLI)

4. **WP2.A Chunk 4** – PalantirServer threading & ownership cleanup
   - Review and document ownership model
   - Ensure `QLocalSocket` objects are properly parented to `PalantirServer`
   - Fix any thread-safety issues (use Qt's queued connections)
   - Add connection timeout handling (disconnect idle connections after 5 minutes)

5. **WP2.A Chunk 5** – ErrorResponse integration
   - Implement `ErrorResponse` proto handling
   - Send `ErrorResponse` for all error conditions (size limit, parse error, unknown message type)
   - Add error codes and user-friendly messages

**Files Modified**:
- `src/palantir/PalantirServer.hpp`
- `src/palantir/PalantirServer.cpp`
- `src/palantir/bedrock_server.cpp` (add CLI options)
- `CMakeLists.txt` (proto codegen for envelope/error)

**Testing**:
- Unit tests for envelope parsing/serialization
- Unit tests for message size limits
- Integration test: Old client → new server (backward compat)
- Integration test: New client → new server (envelope)

---

### WP2.C – Bedrock Monitor & Observability (Bedrock Side)

**Goal**: Add status/health RPC endpoint for observability.

**Chunks**:

1. **WP2.C Chunk 1** – Design status RPC
   - Extend Capabilities or create new Status RPC
   - Define `StatusRequest` and `StatusResponse` proto
   - Include: server health, active connections, queue depth, uptime
   - Tag as Palantir v1.4.0 (if separate from Capabilities)

2. **WP2.C Chunk 2** – Implement status tracking in PalantirServer
   - Track active connections (count)
   - Track request count, error count
   - Track server uptime (start time)
   - Track queue depth (if async processing)

3. **WP2.C Chunk 3** – Implement status RPC handler
   - Add `handleStatusRequest()` method
   - Build `StatusResponse` with current state
   - Send response via `sendMessage()`

4. **WP2.C Chunk 4** – Add status to Capabilities (alternative)
   - If Status RPC is separate, skip this
   - If Status is part of Capabilities, extend `CapabilitiesResponse`
   - Update `CapabilitiesService` to include status

**Files Modified**:
- `src/palantir/PalantirServer.hpp` (add status tracking)
- `src/palantir/PalantirServer.cpp` (implement status tracking and handler)
- `src/palantir/CapabilitiesService.cpp` (if status in Capabilities)
- `CMakeLists.txt` (proto codegen for status, if separate)

**Testing**:
- Unit tests for status RPC
- Integration test: Status polling with server restart
- Integration test: Status accuracy (connection count, uptime)

---

### WP2.D – Thermal Diffusion RPC (Bedrock Side)

**Goal**: Implement Thermal Diffusion computation with progress/cancel.

**Chunks**:

1. **WP2.D Chunk 1** – Update Palantir submodule to v1.5.0
   - Update `docs/palantir` submodule
   - Verify `thermal_diffusion.proto` is available
   - Update CMake proto codegen

2. **WP2.D Chunk 2** – Design Thermal Diffusion algorithm
   - Finite difference method (2D grid)
   - Time-stepping loop
   - Boundary conditions (fixed temperature)
   - Initial conditions (temperature field)

3. **WP2.D Chunk 3** – Implement Thermal Diffusion computation
   - Create `src/compute/ThermalDiffusionSolver.hpp/.cpp`
   - Implement finite difference solver
   - Add OpenMP parallelization (parallelize grid updates)
   - Add progress callback support

4. **WP2.D Chunk 4** – Integrate Thermal Diffusion into PalantirServer
   - Add `handleThermalDiffusionRequest()` method
   - Parse `ThermalDiffusionRequest` (grid size, time steps, boundary conditions)
   - Call solver with progress callback
   - Send `ThermalDiffusionProgress` messages during computation
   - Send `ThermalDiffusionResponse` on completion

5. **WP2.D Chunk 5** – Add progress reporting
   - Progress callback sends `ThermalDiffusionProgress` via `sendMessage()`
   - Progress includes: current step, percent complete, estimated time
   - Throttle progress updates (max 10 updates per second)

6. **WP2.D Chunk 6** – Add cancel support
   - Check cancel flag during computation (atomic bool)
   - Handle `CancelRequest` message
   - Send `CancelResponse` on cancel
   - Clean up computation thread on cancel

7. **WP2.D Chunk 7** – Integration and testing
   - End-to-end test: Thermal Diffusion with progress
   - Test cancel during computation
   - Performance test: OpenMP scaling (1, 2, 4, 8 threads)

**Files Modified**:
- `src/compute/ThermalDiffusionSolver.hpp` (new)
- `src/compute/ThermalDiffusionSolver.cpp` (new)
- `src/palantir/PalantirServer.hpp` (add handler)
- `src/palantir/PalantirServer.cpp` (implement handler)
- `CMakeLists.txt` (new solver sources, proto codegen)

**Testing**:
- Unit tests for Thermal Diffusion algorithm
- Unit tests for OpenMP parallelization
- Integration test: Progress streaming
- Integration test: Cancel during computation
- Performance test: OpenMP scaling

---

### WP2.E – Cleanup & Formal Review (Bedrock Side)

**Goal**: Clean up code, modularize CMake, add guardrails.

**Chunks**:

1. **WP2.E Chunk 1** – CMake modularization
   - Extract proto codegen to `cmake/PalantirProto.cmake`
   - Extract abseil linking to `cmake/AbseilLinking.cmake`
   - Update `CMakeLists.txt` to use modules

2. **WP2.E Chunk 2** – Code cleanup
   - Remove TODOs (or convert to GitHub issues)
   - Fix compiler warnings
   - Document public APIs (Doxygen comments)
   - Format code (clang-format)

3. **WP2.E Chunk 3** – Guardrails and formal review
   - Add assertions for invariants (assert, Q_ASSERT)
   - Add bounds checking (vector access, grid bounds)
   - Prepare review checklist
   - Generate architecture diagrams (if needed)

**Files Modified**:
- `cmake/PalantirProto.cmake` (new)
- `cmake/AbseilLinking.cmake` (new)
- `CMakeLists.txt` (use modules)
- Various source files (cleanup, documentation)

**Testing**: Ensure all existing tests still pass

---

## Testing & CI Strategy

### Unit Tests

**New Test Files**:
- `tests/palantir/PalantirServer_envelope_test.cpp`
- `tests/palantir/PalantirServer_size_limit_test.cpp`
- `tests/palantir/PalantirServer_status_test.cpp`
- `tests/compute/ThermalDiffusionSolver_test.cpp`
- `tests/compute/ThermalDiffusionSolver_openmp_test.cpp`

**Updated Test Files**:
- `tests/palantir/CapabilitiesService_test.cpp` (add status tests if in Capabilities)

### Integration Tests

**New Integration Tests**:
- `tests/integration/message_envelope_test.cpp`
- `tests/integration/thermal_diffusion_progress_test.cpp`
- `tests/integration/cancel_during_computation_test.cpp`

### CI Jobs

| Job | Coverage |
|-----|----------|
| `build` | All configs (BEDROCK_WITH_TRANSPORT_DEPS ON/OFF) |
| `test` | Unit + integration tests |
| `codeql` | Security analysis |
| `performance` | OpenMP scaling tests (if separate job) |

---

## Dependencies on Other Repos

### Palantir

**Proto Updates Required**:
- v1.3.0: `envelope.proto`, `error.proto` (WP2.A)
- v1.4.0: Status RPC (if separate from Capabilities) (WP2.C)
- v1.5.0: `thermal_diffusion.proto` (WP2.D)

**Update Sequence**:
1. Palantir creates proto, tags version
2. Bedrock updates `docs/palantir` submodule
3. Bedrock rebuilds with new proto

### Phoenix

**Coordination Required**:
- WP2.A: Phoenix must implement envelope before Bedrock can require it (or backward compat)
- WP2.C: Phoenix status polling
- WP2.D: Phoenix Thermal Diffusion client

**Testing Coordination**:
- Integration tests require both Phoenix and Bedrock running
- CI may need to start Phoenix client for integration tests

---

## Risk Analysis (Bedrock-Specific)

### WP2.A Risks

**Risk**: Breaking existing RPCs when introducing envelope
- **Mitigation**: Backward compatibility layer, support old client messages
- **Testing**: Old client → new server, new client → new server

**Risk**: Message size limits too restrictive
- **Mitigation**: Configurable limit, default 10MB
- **Testing**: Test with various message sizes

**Risk**: Thread-safety issues in PalantirServer
- **Mitigation**: Use Qt's queued connections, document ownership
- **Testing**: Thread-safety tests, stress tests

### WP2.C Risks

**Risk**: Status tracking overhead
- **Mitigation**: Lightweight tracking (counters, timestamps)
- **Testing**: Measure status tracking overhead

**Risk**: Status accuracy (race conditions)
- **Mitigation**: Atomic counters, careful synchronization
- **Testing**: Status accuracy tests under load

### WP2.D Risks

**Risk**: OpenMP parallelization correctness
- **Mitigation**: Careful loop parallelization, avoid race conditions
- **Testing**: OpenMP correctness tests, compare with serial version

**Risk**: Progress reporting overhead
- **Mitigation**: Throttle progress updates (max 10 per second)
- **Testing**: Measure progress reporting overhead

**Risk**: Cancel during computation race conditions
- **Mitigation**: Atomic cancel flag, check at safe points (between time steps)
- **Testing**: Cancel during computation stress test

**Risk**: Memory usage for large grids
- **Mitigation**: Validate grid size limits, use efficient data structures
- **Testing**: Memory usage tests with large grids

---

## Summary

**Total Bedrock Chunks**: 20
- WP2.A: 5 chunks
- WP2.C: 4 chunks
- WP2.D: 7 chunks
- WP2.E: 3 chunks

**Key Bedrock Files**:
- New: `ThermalDiffusionSolver`
- Modified: `PalantirServer`, `CapabilitiesService`, `bedrock_server`

**Open Questions**:
1. Should Status RPC be separate or part of Capabilities?
   - **Recommendation**: Separate (cleaner, more extensible)

2. Should Thermal Diffusion use separate thread or Qt's thread pool?
   - **Recommendation**: Separate thread (better control, easier cancel)

3. Should progress updates be sent via same socket or separate socket?
   - **Recommendation**: Same socket with message envelope (simpler)

4. Should OpenMP thread count be configurable?
   - **Recommendation**: Yes, via CLI option `--openmp-threads`

---

**Bedrock Sprint 4.5 Planning Complete** — Ready for implementation.

