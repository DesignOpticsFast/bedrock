# Bedrock Threading Model

This document describes the threading and concurrency model for Bedrock, covering all systems including PalantirServer, compute engines, and utility modules.

---

## High-Level Overview

**Bedrock uses a hybrid threading model:**
- **PalantirServer (IPC):** Runs entirely on Qt's event loop thread (main thread)
- **Compute operations:** Currently synchronous, but OpenMP infrastructure exists for future parallel computation
- **Worker threads:** Infrastructure exists in PalantirServer but is currently disabled

**Current state (Sprint 4.5):**
- PalantirServer: All operations on Qt event loop thread
- XY Sine computation: Runs synchronously on event loop thread
- OpenMP: Infrastructure present in `bedrock_core`, not yet used in production compute paths
- No background worker threads are active
- Mutexes protect shared data structures but are currently only accessed from event loop thread

---

## Global Threading Model

### Main Thread (Qt Event Loop)

**Used by:** PalantirServer (IPC layer)

**Ownership:**
- `PalantirServer` object (QObject)
- `QLocalServer` and all `QLocalSocket` objects
- All signal/slot connections

**Operations on this thread:**
- Server startup/shutdown (`startServer()`, `stopServer()`)
- Accepting new connections (`onNewConnection()`)
- Reading socket data (`onClientReadyRead()` → `parseIncomingData()`)
- Message parsing and dispatch (`parseIncomingData()`, `extractMessage()`)
- Request handling (`handleCapabilitiesRequest()`, `handleXYSineRequest()`)
- XY Sine computation (`computeXYSine()` - runs synchronously)
- Sending responses (`sendMessage()`, `sendErrorResponse()`)
- Client disconnection handling (`onClientDisconnected()`)

**Key invariant:** All socket I/O and message handling happens on the Qt event loop thread. This ensures thread-safe access to Qt objects without explicit synchronization.

### Compute Threads (OpenMP - Future)

**Used by:** Future heavy compute operations (ray tracing, optical analysis, etc.)

**Infrastructure:**
- `bedrock::ThreadingConfig` - Manages OpenMP thread configuration
- `bedrock::ThreadingBenchmark` - Performance measurement utilities
- OpenMP pragmas (`#pragma omp parallel for`) for parallel loops

**Current status:**
- OpenMP infrastructure exists in `bedrock_core`
- Not yet used in production compute paths (XY Sine runs synchronously)
- Intended for future heavy compute operations

**Future design:**
- Heavy compute operations will use OpenMP parallel regions
- Each parallel region processes independent data chunks
- Results aggregated and returned to caller
- No shared mutable state within parallel regions

### Worker Threads (Currently Disabled)

**Infrastructure exists but is commented out:**
- `jobThreads_` map stores `std::thread` objects in PalantirServer
- `processJob()` would run on worker threads
- Worker threads would be created via `std::thread` in future `handleStartJob()` implementation

**Future design (when enabled):**
- Long-running computations would run on worker threads
- Worker threads would check `jobCancelled_` for cancellation
- Results would be sent back via `sendMessage()` (which must be called from event loop thread)

---

## OpenMP / TBB Usage Patterns

### OpenMP Infrastructure

**Location:** `bedrock/core/include/bedrock/threading.hpp`, `bedrock/core/src/threading.cpp`

**Components:**
- `ThreadingConfig`: Manages OpenMP thread count and configuration
- `ThreadingBenchmark`: Performance measurement and optimization utilities
- `PerformanceCounter` / `ScopedTimer`: Thread-safe performance measurement

**Current usage:**
- Used in test/benchmark code (`bedrock_threading_test`)
- Not yet used in production compute paths
- Intended for future heavy compute operations

### Intended Parallelism Targets

**Future compute operations that will use OpenMP:**
- Ray tracing (parallel ray processing)
- Optical analysis (parallel surface/ray intersection)
- Heavy mathematical computations (matrix operations, FFTs)
- Large-scale data processing

**High-level patterns (when implementing):**

1. **Parallel loops:**
   ```cpp
   #pragma omp parallel for
   for (int i = 0; i < ray_count; ++i) {
       results[i] = compute_ray(rays[i]);
   }
   ```

2. **Reduction patterns:**
   ```cpp
   double sum = 0.0;
   #pragma omp parallel for reduction(+:sum)
   for (int i = 0; i < size; ++i) {
       sum += compute_value(data[i]);
   }
   ```

3. **Thread-local data:**
   ```cpp
   #pragma omp parallel
   {
       thread_local std::vector<double> temp_buffer;
       // Use temp_buffer for per-thread temporary data
   }
   ```

### OpenMP Guidelines

**✅ Do:**
- Use `ThreadingConfig::initialize()` before using OpenMP
- Keep parallel regions focused on independent computations
- Use reduction patterns for accumulations
- Keep per-thread temporary data thread-local
- Measure performance with `ThreadingBenchmark` utilities

**❌ Don't:**
- Hold locks inside parallel regions (causes contention)
- Share non-thread-safe containers across OpenMP threads
- Access Qt objects from OpenMP threads (Qt objects must stay on event loop thread)
- Mix Qt thread affinity with OpenMP threads
- Capture shared mutable state without protection

**Thread-safe patterns:**
- Read-only shared data (const references) is safe
- Per-thread temporary data (thread_local or stack variables)
- Reduction operations (OpenMP handles synchronization)
- Atomic operations for simple flags

---

## Thread-Safe vs Single-Threaded Components

| Component | Thread-Safe? | Notes |
|-----------|--------------|-------|
| **PalantirServer** | ❌ Single-threaded (event loop only) | Qt sockets must stay on the event loop thread. All operations run on Qt event loop thread. Mutexes protect shared data but are currently only accessed from one thread. |
| **QLocalSocket** | ❌ Not thread-safe | Must be accessed from the thread that owns it (Qt event loop thread). `state()` is thread-safe for reading only. |
| **Local compute (XY Sine)** | ✅ Stateless (thread-safe) | `computeXYSine()` is a pure function with no shared state. Thread-safe if callers provide isolated input/output. Currently runs synchronously on event loop thread. |
| **ThreadingConfig** | ⚠️ Partially thread-safe | Static initialization is not thread-safe (should be called once at startup). Thread count queries are thread-safe after initialization. |
| **ThreadingBenchmark** | ✅ Thread-safe | Uses OpenMP internally, designed for concurrent use. |
| **PerformanceCounter** | ✅ Thread-safe | Designed for performance measurement in multi-threaded contexts. |
| **Future heavy compute** | ✅ Expected to be thread-safe | To be implemented using OpenMP/TBB with proper synchronization. Will use thread-local data and reduction patterns. |

### Component Details

**PalantirServer:**
- **Thread model:** Single-threaded (Qt event loop)
- **Why:** Qt socket operations must be on the socket's owner thread
- **Future:** Worker threads can be enabled for long-running computations, but socket operations must remain on event loop thread

**Local compute (XY Sine):**
- **Thread model:** Stateless function
- **Thread-safety:** Safe for concurrent calls with isolated inputs
- **Current usage:** Called synchronously from event loop thread
- **Future:** Could be called from worker threads or OpenMP regions if needed

**OpenMP infrastructure:**
- **Thread model:** Parallel regions managed by OpenMP runtime
- **Thread-safety:** OpenMP handles thread creation and synchronization
- **Usage:** Initialize once at startup, then use pragmas for parallel loops

---

## Concurrency Guidelines

### General Rules

**1. Never block the Qt event loop:**
- Keep PalantirServer handlers fast (< 100ms ideally)
- Move long-running work to worker threads (when enabled)
- Use OpenMP for parallel compute, not for blocking operations

**2. Mutex usage:**
- Protect only shared mutable state
- Keep lock scope minimal (only protect data structure access)
- Release mutex before doing computation or dispatching messages
- Avoid nested locks (deadlock risk)

**3. Qt thread affinity:**
- Qt objects (QObject, QLocalSocket, etc.) must stay on their owner thread
- Use Qt signal/slot for cross-thread communication
- Avoid mixing Qt thread rules with raw `std::thread` or OpenMP threads

**4. OpenMP/TBB patterns:**
- Avoid capturing shared mutable state without protection
- Use reduction patterns for accumulations
- Keep per-thread data thread-local
- Don't hold locks inside parallel regions

### Patterns for Future Code

**Adding new RPCs (PalantirServer):**
- Handler runs on event loop thread
- Keep computation fast or move to worker thread
- Use `sendMessage()` from event loop thread only
- Access shared data with appropriate mutex

**Adding parallel compute:**
- Use OpenMP `#pragma omp parallel for` for independent iterations
- Use `ThreadingConfig::initialize()` at startup
- Measure performance with `ThreadingBenchmark`
- Keep shared state read-only or use reduction patterns

**Adding worker threads:**
- Lock `jobMutex_` to create and track threads
- Worker threads check `jobCancelled_` for cancellation
- Send results via Qt signal/slot (queued connection) to event loop thread
- Never access Qt objects from worker threads directly

### Anti-Patterns to Avoid

**❌ Don't:**
- Block event loop thread with long computations
- Hold mutex while doing long-running work
- Access `QLocalSocket` from worker thread or OpenMP thread
- Call `sendMessage()` from worker thread directly
- Share non-thread-safe containers across OpenMP threads
- Hold locks inside OpenMP parallel regions
- Mix Qt thread affinity with raw `std::thread` or OpenMP

**✅ Do:**
- Keep event loop handlers fast
- Use worker threads for long-running work (when enabled)
- Use OpenMP for parallel compute
- Use Qt signal/slot for cross-thread communication
- Keep mutex lock scope minimal
- Use atomic variables for simple flags
- Use thread-local data in parallel regions

---

## PalantirServer Threading Details

### Thread Architecture

**Main Thread (Qt Event Loop):**
- Owns `QLocalServer` and all `QLocalSocket` objects
- All signal/slot handlers run on this thread
- Message parsing, request handling, and XY Sine computation execute synchronously

**Worker Threads (Currently Disabled):**
- Infrastructure exists (`jobThreads_`, `jobMutex_`) but is commented out
- Future long-running computations would run on worker threads

### Mutexes and Protected Data

**`clientBuffersMutex_`:**
- **Protects:** `clientBuffers_` map (`std::map<QLocalSocket*, QByteArray>`)
- **Purpose:** Thread-safe access to per-client receive buffers
- **Invariants when locked:** Map is in consistent state, no concurrent access
- **Usage:** Locked in `onNewConnection()`, `onClientDisconnected()`, `parseIncomingData()`, `stopServer()`
- **Current access pattern:** All accesses from event loop thread, but mutex provides future-proofing

**`jobMutex_`:**
- **Protects:** `jobClients_`, `jobCancelled_`, `jobThreads_` maps
- **Purpose:** Thread-safe access to job tracking data structures
- **Invariants when locked:** Job tracking maps are in consistent state
- **Usage:** Locked in `stopServer()`, `onClientDisconnected()`, future `handleStartJob()` and `processJob()`
- **Current access pattern:** All accesses from event loop thread, but mutex provides future-proofing

### Socket Operations

**QLocalSocket Thread Safety:**
- `QLocalSocket` is not thread-safe
- Socket operations must be performed from the thread that owns the socket
- `QLocalSocket::state()` is thread-safe for reading (used in `sendMessage()`)

**PalantirServer design:**
- All `QLocalSocket` objects are created and owned by Qt event loop thread
- All socket operations (read, write, state checks) happen on event loop thread
- No socket operations are performed from worker threads

**Safe patterns:**
- Reading: `client->readAll()` in `onClientReadyRead()` (event loop thread)
- Writing: `client->write()` in `sendMessage()` (event loop thread)
- State check: `client->state()` in `sendMessage()` (event loop thread)

### Request Handling Flow

**Current Flow (Sprint 4.5):**

1. **Client connects** → `onNewConnection()` (event loop thread)
   - Creates `QLocalSocket` object
   - Connects signals
   - Adds client to `clientBuffers_` map (with `clientBuffersMutex_`)

2. **Data arrives** → `onClientReadyRead()` (event loop thread)
   - Calls `parseIncomingData()`

3. **Parse message** → `parseIncomingData()` (event loop thread)
   - Locks `clientBuffersMutex_` to append data and extract message
   - Releases lock before dispatching
   - Calls handler (`handleCapabilitiesRequest()` or `handleXYSineRequest()`)

4. **Handle request** → Handler function (event loop thread)
   - For Capabilities: Creates response, calls `sendMessage()`
   - For XY Sine: Validates, computes synchronously, creates response, calls `sendMessage()`

5. **Send response** → `sendMessage()` (event loop thread)
   - Creates envelope, serializes, writes to socket

**Key point:** Everything happens synchronously on the event loop thread. No blocking operations are performed that would stall the event loop.

---

## Guidelines for Future Code

### Adding New RPCs

**Safe patterns:**
- Add handler function (e.g., `handleNewRequest()`)
- Handler runs on event loop thread
- Can call `sendMessage()` directly
- Can access `clientBuffers_` with `clientBuffersMutex_` if needed

**For synchronous computation:**
- Compute directly in handler (like current `handleXYSineRequest()`)
- Keep computation fast (< 100ms ideally)
- Don't block event loop

**For long-running computation (future):**
- Use worker thread infrastructure (when enabled)
- Lock `jobMutex_` to create thread and track job
- Worker thread checks `jobCancelled_` for cancellation
- Send results via `sendMessage()` from event loop thread (use Qt signal/slot or queued connection)

### Adding Parallel Compute

**Safe patterns:**
- Use OpenMP `#pragma omp parallel for` for independent iterations
- Initialize `ThreadingConfig` at startup
- Use reduction patterns for accumulations
- Keep shared state read-only or use thread-local data

**Example:**
```cpp
#include "bedrock/threading.hpp"

void compute_parallel(const std::vector<Ray>& rays, std::vector<Result>& results) {
    bedrock::ThreadingConfig::initialize();
    
    results.resize(rays.size());
    
    #pragma omp parallel for
    for (size_t i = 0; i < rays.size(); ++i) {
        results[i] = compute_ray(rays[i]);
    }
}
```

### Patterns to Avoid

**❌ Don't:**
- Hold mutex while doing long-running work (blocks other operations)
- Access `QLocalSocket` from worker thread or OpenMP thread
- Call `sendMessage()` from worker thread directly (must use Qt signal/slot)
- Access shared data structures without appropriate mutex
- Block event loop thread with long computations
- Hold locks inside OpenMP parallel regions
- Share non-thread-safe containers across OpenMP threads

**✅ Do:**
- Keep mutex lock scope minimal (only protect data structure access)
- Release mutex before dispatching messages or doing computation
- Use atomic variables for simple flags (`running_`, `jobCancelled_`)
- Use Qt signal/slot for cross-thread communication (when worker threads enabled)
- Use OpenMP for parallel compute with proper patterns
- Keep per-thread data thread-local in parallel regions

---

## TSAN Notes

### TSAN (ThreadSanitizer) Testing

**Configuration:**
Build with `-fsanitize=thread` flag:
```bash
mkdir build-tsan && cd build-tsan
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DCMAKE_CXX_FLAGS="-fsanitize=thread -g" \
         -DBEDROCK_WITH_TRANSPORT_DEPS=ON \
         -DBUILD_TESTING=ON
cmake --build . -j4
ctest -R palantir -E "disabled|slow"
```

**Platform limitations:**
- **macOS (Apple Clang):** TSAN has known limitations and may not fully support all OpenMP patterns
- **Linux:** Full TSAN support recommended for CI integration
- **Recommendation:** Full TSAN testing should be performed on Linux CI in Workstream 3

**Known false positives:**
- Qt's internal thread synchronization (if not properly annotated)
- Atomic operations that TSAN doesn't fully understand
- Signal/slot connections (Qt's internal synchronization)
- OpenMP runtime internal synchronization

**Known safe patterns:**
- Mutex-protected access to shared data structures
- Atomic variables for simple flags (`running_`, `jobCancelled_`)
- Qt signal/slot for cross-thread communication (when enabled)
- All socket operations on event loop thread
- OpenMP parallel regions with proper data isolation

**If TSAN reports issues:**
- Verify mutex is held for all accesses to protected data
- Check that socket operations are only from event loop thread
- Ensure worker threads (when enabled) use proper synchronization
- Review lock scope (should be minimal, released before dispatch)
- Verify OpenMP parallel regions don't share mutable state
- Check that thread-local data is properly isolated

**Sprint 4.5 TSAN status:**
- Threading model documented and verified through code review
- All mutex-protected accesses identified and documented
- Socket operations confirmed to be on event loop thread only
- OpenMP infrastructure present but not yet used in production paths
- Full TSAN run recommended for Workstream 3 (CI integration on Linux)

**Future CI TSAN plan:**
- Configure TSAN build on Linux CI (better TSAN support than macOS)
- Run: `ctest -R palantir -E "disabled|slow"`
- Run: `bedrock_threading_test` with TSAN
- Verify no race conditions reported
- Document any false positives in this file

---

## Summary

**Current state (Sprint 4.5):**
- **PalantirServer:** Single-threaded (all operations on Qt event loop thread)
- **Compute operations:** Synchronous (XY Sine runs on event loop thread)
- **OpenMP:** Infrastructure exists but not yet used in production
- **Worker threads:** Infrastructure exists but disabled
- **Mutexes:** Protect shared data but are currently only accessed from one thread

**Future state (when fully enabled):**
- **Event loop thread:** Socket I/O, message parsing, response sending
- **Worker threads:** Long-running computations (when enabled)
- **OpenMP threads:** Parallel compute operations (ray tracing, heavy math)
- **Mutexes:** Protect shared data accessed from multiple threads
- **Communication:** Qt signal/slot for cross-thread coordination

**Key invariants:**
- `QLocalSocket` objects are always accessed from the Qt event loop thread
- Qt objects must stay on their owner thread
- OpenMP parallel regions must not share mutable state
- Mutexes protect shared data structures

---

**Document Version:** 1.0 (Sprint 4.5)  
**Last Updated:** 2025-01-26  
**Maintained By:** Bedrock Team
