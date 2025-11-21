# Sprint 4.2 Chunk 2A: Echo RPC CMake Integration Plan

**Date:** 2025-11-21  
**Status:** PLANNING ONLY — No implementation yet  
**Intent:** Analyze and plan Echo RPC CMake integration before making changes

---

## 1. Current Echo RPC Layout

### Directory Tree

```
bedrock/
├── proto/
│   ├── bedrock_echo.proto                    # Source proto definition
│   ├── bedrock_echo.pb.h                      # Generated (manually, needs CMake)
│   ├── bedrock_echo.pb.cc                     # Generated (manually, needs CMake)
│   ├── bedrock_echo.grpc.pb.h                 # Generated (manually, needs CMake)
│   └── bedrock_echo.grpc.pb.cc                # Generated (manually, needs CMake)
│
├── src/
│   └── echo/
│       ├── echo_service_impl.h                # Echo service implementation header
│       └── echo_service_impl.cpp              # Echo service implementation
│
├── tests/
│   ├── bedrock_echo_sanity.cpp                # In-process server+client test
│   └── bedrock_transport_deps_sanity.cpp      # Dependency presence check
│
├── CMakeLists.txt                             # Top-level (no transport deps yet)
└── tests/CMakeLists.txt                       # Test config (protobuf QUIET checks only)
```

### File Analysis

**Proto Definition:**
- `proto/bedrock_echo.proto` — Simple echo service with EchoRequest/EchoReply messages
- Package: `bedrock.echo`
- Service: `EchoService` with single `Echo` RPC method

**Generated Files (Currently Manual):**
- `proto/bedrock_echo.pb.h/cc` — Protobuf message classes
- `proto/bedrock_echo.grpc.pb.h/cc` — gRPC service stubs
- **Status:** Files exist but were manually generated (not via CMake)

**Implementation:**
- `src/echo/echo_service_impl.h/cpp` — EchoServiceImpl class inheriting from `EchoService::Service`
- Simple implementation: returns same message as received

**Test Files:**
- `tests/bedrock_echo_sanity.cpp` — **In-process test** (server + client in single executable)
  - Starts server in background thread on `localhost:50051`
  - Creates client, calls Echo RPC, verifies response
  - Uses 100ms delay for server startup
  - Returns 0 on success, 1 on failure
- `tests/bedrock_transport_deps_sanity.cpp` — Dependency presence check (protobuf/gRPC/libsodium headers)

---

## 2. Existing CMake Transport Hooks

### Current State

**What's Already There:**
- ✅ `tests/CMakeLists.txt` has `find_package(Protobuf QUIET)` checks (lines 42-44, 69-71)
  - Used for `test_xysine_compute` and `test_palantir_xysine`
  - Links protobuf if found (QUIET mode, optional)
- ✅ Preflight script (`scripts/preflight.sh`) checks for `protoc` and `grpc_cpp_plugin` (optional warnings)

**What's Missing:**
- ❌ No `BEDROCK_WITH_TRANSPORT_DEPS` option in top-level `CMakeLists.txt`
- ❌ No gRPC `find_package` calls anywhere
- ❌ No proto code generation (`add_custom_command` for protoc/grpc_cpp_plugin)
- ❌ No echo-related targets (`bedrock_echo_sanity`, `bedrock_transport`, etc.)
- ❌ No generated directory structure for proto outputs
- ❌ No CTest entries for echo RPC tests

**Unused/Half-Finished Fragments:**
- ⚠️ `WP0_CHUNK_0.5_SUMMARY.md` documents planned implementation but it was never integrated
- ⚠️ Generated proto files exist in `proto/` directory (manually created, should be in build directory)
- ⚠️ `bedrock_transport_deps_sanity.cpp` exists but has no CMake target

### Pattern Analysis

**Existing Option Pattern:**
- `BEDROCK_WITH_OCCT` option (line 62 in `CMakeLists.txt`)
  - Default: `ON`
  - Controls `find_package(OpenCASCADE REQUIRED)`
  - Guards `bedrock_geom` library creation
  - Similar pattern should be used for `BEDROCK_WITH_TRANSPORT_DEPS`

**Existing Protobuf Pattern:**
- `find_package(Protobuf QUIET)` in `tests/CMakeLists.txt`
- Conditional linking: `if(Protobuf_FOUND)`
- **Issue:** Uses QUIET mode, should be REQUIRED when transport deps enabled

---

## 3. Proposed CMake Structure

### Plan Overview

**Goal:** Integrate Echo RPC into CMake build system with proper proto generation, library organization, and test integration.

### Step-by-Step Plan

#### **Step 1: Add CMake Option**

**Location:** Top-level `CMakeLists.txt` (after `BUILD_TESTING` option, ~line 21)

**Action:**
```cmake
option(BEDROCK_WITH_TRANSPORT_DEPS "Enable gRPC/protobuf transport dependencies (Echo RPC)" OFF)
```

**Rationale:**
- Default `OFF` to keep CI clean (matches current CI behavior)
- Explicit opt-in for transport functionality
- Follows pattern of `BEDROCK_WITH_OCCT`

---

#### **Step 2: Create Transport Subdirectory Structure**

**Location:** New `src/transport/CMakeLists.txt` (or integrate into existing structure)

**Decision Point:** Two options:
- **Option A:** Create `src/transport/` subdirectory with its own `CMakeLists.txt`
- **Option B:** Add transport targets directly to top-level `CMakeLists.txt`

**Recommendation:** **Option B** (simpler, matches existing structure)
- Echo RPC is minimal (single service)
- No need for separate subdirectory yet
- Can refactor later if transport grows

**If Option A chosen:**
- Create `src/transport/CMakeLists.txt`
- Add `add_subdirectory(src/transport)` guarded by `BEDROCK_WITH_TRANSPORT_DEPS`

---

#### **Step 3: Proto Code Generation**

**Location:** Top-level `CMakeLists.txt` (after `BEDROCK_WITH_TRANSPORT_DEPS` option, guarded by `if(BEDROCK_WITH_TRANSPORT_DEPS)`)

**Actions:**

1. **Find Dependencies:**
   ```cmake
   find_package(Protobuf REQUIRED)
   find_package(gRPC REQUIRED)
   ```

2. **Locate Tools:**
   ```cmake
   find_program(PROTOC protoc)
   find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
   ```

3. **Set Generated Directory:**
   ```cmake
   set(ECHO_GENERATED_DIR ${CMAKE_BINARY_DIR}/generated/echo)
   file(MAKE_DIRECTORY ${ECHO_GENERATED_DIR})
   ```

4. **Generate Proto Files:**
   ```cmake
   # Protobuf messages
   add_custom_command(
     OUTPUT ${ECHO_GENERATED_DIR}/bedrock_echo.pb.h
            ${ECHO_GENERATED_DIR}/bedrock_echo.pb.cc
     COMMAND ${PROTOC}
     ARGS --cpp_out=${ECHO_GENERATED_DIR}
          --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto
          ${CMAKE_CURRENT_SOURCE_DIR}/proto/bedrock_echo.proto
     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/proto/bedrock_echo.proto
     COMMENT "Generating bedrock_echo protobuf files"
   )
   
   # gRPC service stubs
   add_custom_command(
     OUTPUT ${ECHO_GENERATED_DIR}/bedrock_echo.grpc.pb.h
            ${ECHO_GENERATED_DIR}/bedrock_echo.grpc.pb.cc
     COMMAND ${PROTOC}
     ARGS --grpc_out=${ECHO_GENERATED_DIR}
          --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
          --proto_path=${CMAKE_CURRENT_SOURCE_DIR}/proto
          ${CMAKE_CURRENT_SOURCE_DIR}/proto/bedrock_echo.proto
     DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/proto/bedrock_echo.proto
            ${ECHO_GENERATED_DIR}/bedrock_echo.pb.h
     COMMENT "Generating bedrock_echo gRPC files"
   )
   ```

**Generated Files Location:**
- `${CMAKE_BINARY_DIR}/generated/echo/` (keeps source tree clean)
- Matches pattern described in `WP0_CHUNK_0.5_SUMMARY.md`

---

#### **Step 4: Create Transport Library (Optional)**

**Decision Point:** Do we need a `bedrock_transport` library?

**Options:**
- **Option A:** Create `bedrock_transport` library containing echo service implementation
- **Option B:** Link echo implementation directly into test executable

**Recommendation:** **Option B** (simpler for Phase 0)
- Echo RPC is minimal (single service, single implementation file)
- No other consumers yet (Phoenix will come later)
- Can add library later when transport grows

**If Option A chosen:**
```cmake
add_library(bedrock_transport
  src/echo/echo_service_impl.cpp
  ${ECHO_GENERATED_DIR}/bedrock_echo.pb.cc
  ${ECHO_GENERATED_DIR}/bedrock_echo.grpc.pb.cc
)
target_include_directories(bedrock_transport PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${ECHO_GENERATED_DIR}
)
target_link_libraries(bedrock_transport PUBLIC
  protobuf::libprotobuf
  gRPC::grpc++
)
```

---

#### **Step 5: Create Echo Test Executable**

**Location:** `tests/CMakeLists.txt` (guarded by `if(BEDROCK_WITH_TRANSPORT_DEPS)`)

**Actions:**

1. **Add Executable:**
   ```cmake
   add_executable(bedrock_echo_sanity
     bedrock_echo_sanity.cpp
     ../src/echo/echo_service_impl.cpp
     ${ECHO_GENERATED_DIR}/bedrock_echo.pb.cc
     ${ECHO_GENERATED_DIR}/bedrock_echo.grpc.pb.cc
   )
   ```

2. **Set Include Directories:**
   ```cmake
   target_include_directories(bedrock_echo_sanity PRIVATE
     ${CMAKE_CURRENT_SOURCE_DIR}/../src
     ${ECHO_GENERATED_DIR}
   )
   ```

3. **Link Libraries:**
   ```cmake
   target_link_libraries(bedrock_echo_sanity PRIVATE
     protobuf::libprotobuf
     gRPC::grpc++
     Threads::Threads  # For std::thread
   )
   ```

4. **Add CTest Entry:**
   ```cmake
   add_test(NAME bedrock_echo_rpc COMMAND bedrock_echo_sanity)
   set_tests_properties(bedrock_echo_rpc PROPERTIES
     TIMEOUT 10
   )
   ```

**Note:** Generated files must be added as sources to ensure CMake tracks dependencies correctly.

---

#### **Step 6: Handle Dependency Detection**

**Location:** Top-level `CMakeLists.txt` (in `BEDROCK_WITH_TRANSPORT_DEPS` block)

**Actions:**

1. **Check for Required Tools:**
   ```cmake
   if(BEDROCK_WITH_TRANSPORT_DEPS)
     find_package(Protobuf REQUIRED)
     find_package(gRPC REQUIRED)
     
     find_program(PROTOC protoc REQUIRED)
     find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)
     
     message(STATUS "Transport deps enabled: protobuf=${Protobuf_VERSION}, gRPC=${gRPC_VERSION}")
   endif()
   ```

2. **Handle Optional libsodium:**
   ```cmake
   find_package(sodium QUIET)
   if(sodium_FOUND)
     message(STATUS "libsodium found (optional)")
   endif()
   ```

---

#### **Step 7: Update CI Workflow**

**Location:** `.github/workflows/ci.yml`

**Current State:**
- Linux CI builds `bedrock_engine` only
- No transport deps enabled

**Proposed Changes:**

**Option A: Add Separate Transport Test Job**
```yaml
jobs:
  build:
    # ... existing build job ...
  
  test-transport:
    name: Transport deps test (Linux)
    runs-on: [self-hosted, linux, x64, dev01, fastpath]
    steps:
      - uses: actions/checkout@v5
      - name: Configure with transport deps
        run: |
          cmake -S . -B build-transport -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DBEDROCK_WITH_TRANSPORT_DEPS=ON
      - name: Build echo test
        run: cmake --build build-transport --target bedrock_echo_sanity
      - name: Run echo RPC test
        run: cd build-transport && ctest -R bedrock_echo_rpc --output-on-failure
```

**Option B: Add Transport Test to Existing Job**
```yaml
- name: Configure (CMake - engine-only build)
  run: |
    cmake -S . -B build -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_COMPILER_LAUNCHER=ccache \
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
      -DBEDROCK_WITH_OCCT=OFF \
      -DBEDROCK_WITH_TRANSPORT_DEPS=ON  # Add this

- name: Build libraries + transport test
  run: |
    ccache -z
    cmake --build build --config Release --target bedrock_engine bedrock_echo_sanity -j
    ccache -s

- name: Run transport test
  run: cd build && ctest -R bedrock_echo_rpc --output-on-failure
```

**Recommendation:** **Option B** (simpler, single job)

---

## 4. Test Strategy for Echo RPC

### Current Test Architecture

**`bedrock_echo_sanity.cpp` uses in-process architecture:**
- Single executable contains both server and client
- Server starts in background thread (`std::thread`)
- Client runs in main thread
- 100ms delay after server start (hardcoded)
- Server shutdown after test completes

### Test Execution Flow

1. **Server Startup:**
   - Create `EchoServiceImpl` instance
   - Build gRPC server on `localhost:50051`
   - Start server in background thread
   - Wait 100ms for readiness

2. **Client Test:**
   - Create gRPC channel to `localhost:50051`
   - Create stub from channel
   - Prepare `EchoRequest` with message "hello"
   - Call `Echo` RPC
   - Verify `EchoReply` matches request

3. **Cleanup:**
   - Shutdown server
   - Join server thread
   - Exit with code 0 (success) or 1 (failure)

### Flakiness Mitigation

**Current Issues:**
- ⚠️ Hardcoded port (`localhost:50051`) — could conflict if test runs in parallel
- ⚠️ Hardcoded 100ms delay — may be insufficient on slow systems
- ⚠️ No retry logic for connection failures

**Proposed Improvements (Future):**
1. **Dynamic Port Selection:**
   - Use port 0 (OS-assigned) or `get_free_port()` helper
   - Pass port via command-line argument or environment variable

2. **Better Startup Detection:**
   - Poll server readiness instead of fixed delay
   - Use gRPC health check or simple TCP connect retry

3. **Timeout Handling:**
   - Set gRPC deadline on client calls
   - Use CTest `TIMEOUT` property (already proposed)

4. **Isolation:**
   - Use Unix Domain Socket (UDS) instead of TCP for better isolation
   - Or use unique port per test run (process ID based)

**For Phase 0 (Current Plan):**
- Keep existing in-process architecture (works, simple)
- Add CTest `TIMEOUT` property (10 seconds)
- Document port assumption in test comments
- **No changes to test code** — only CMake integration

### CTest Integration

**Test Name:** `bedrock_echo_rpc`

**Command:** `bedrock_echo_sanity` (executable)

**Properties:**
- `TIMEOUT 10` — Fail if test takes > 10 seconds
- `WORKING_DIRECTORY ${CMAKE_BINARY_DIR}` — Run from build directory

**Execution:**
```bash
ctest -R bedrock_echo_rpc --output-on-failure
```

**Expected Output:**
- Test passes (exit code 0)
- Execution time < 1 second (typically ~100-200ms)

---

## 5. Open Questions for Mark

### Question 1: CMake Option Default

**Question:** Should `BEDROCK_WITH_TRANSPORT_DEPS` default to `ON` or `OFF`?

**Options:**
- **Option A:** Default `OFF` (current plan)
  - Keeps CI clean (no transport deps required)
  - Explicit opt-in for transport features
  - Matches current CI behavior

- **Option B:** Default `ON`
  - Transport deps always available in dev builds
  - Simpler for developers (no flag needed)
  - Requires CI to have transport deps installed

**Recommendation:** **Option A (OFF)** — safer, matches current state

**Your Decision:** ❓

---

### Question 2: Library vs Direct Linking

**Question:** Should we create a `bedrock_transport` library, or link echo implementation directly into test?

**Options:**
- **Option A:** Create `bedrock_transport` library
  - Cleaner separation of concerns
  - Reusable for future transport features
  - Phoenix can link against it later

- **Option B:** Link directly into test executable
  - Simpler for Phase 0 (minimal code)
  - No library overhead
  - Can refactor later when needed

**Recommendation:** **Option B** — simpler for Phase 0, can add library later

**Your Decision:** ❓

---

### Question 3: Test Architecture

**Question:** Keep in-process server+client, or split into separate executables?

**Current:** Single executable (`bedrock_echo_sanity`) with in-process server+client

**Options:**
- **Option A:** Keep in-process (current)
  - Simple, no IPC complexity
  - Fast, no network overhead
  - Works well for unit tests

- **Option B:** Split into `bedrock_echo_server` + `bedrock_echo_client`
  - More realistic (matches production)
  - Better isolation
  - More complex (process management, IPC)

**Recommendation:** **Option A** — keep existing architecture, works well

**Your Decision:** ❓

---

### Question 4: CI Integration Strategy

**Question:** How should CI handle transport deps?

**Options:**
- **Option A:** Always enable in Linux CI (add `-DBEDROCK_WITH_TRANSPORT_DEPS=ON`)
  - Validates transport on every build
  - Requires transport deps in CI environment
  - **Current CI:** Transport deps available on dev01 ✅

- **Option B:** Separate CI job for transport tests
  - Keeps main build fast
  - Can skip if transport deps unavailable
  - More CI complexity

- **Option C:** Manual/optional CI (workflow_dispatch only)
  - No automatic transport testing
  - Developers test locally
  - Simplest CI setup

**Recommendation:** **Option A** — transport deps available, should test them

**Your Decision:** ❓

---

### Question 5: Generated Files Location

**Question:** Where should generated proto files live?

**Options:**
- **Option A:** `${CMAKE_BINARY_DIR}/generated/echo/` (current plan)
  - Keeps source tree clean
  - Standard CMake practice
  - Matches `WP0_CHUNK_0.5_SUMMARY.md`

- **Option B:** `proto/generated/` (in source tree)
  - Easier to inspect generated files
  - Clutters source tree
  - Not recommended

**Recommendation:** **Option A** — build directory is standard

**Your Decision:** ❓

---

### Question 6: macOS/Windows CI

**Question:** Should transport deps be enabled in macOS/Windows CI (when available)?

**Current:** macOS/Windows CI not configured

**Options:**
- **Option A:** Enable transport deps on all platforms (when CI available)
  - Validates cross-platform transport
  - Requires transport deps on all CI runners

- **Option B:** Linux only (for now)
  - Simpler, matches current CI setup
  - Can add other platforms later

**Recommendation:** **Option B** — Linux only for now, add others when CI available

**Your Decision:** ❓

---

### Question 7: libsodium Integration

**Question:** How should libsodium be handled?

**Current:** `bedrock_transport_deps_sanity.cpp` checks for libsodium but it's optional

**Options:**
- **Option A:** Keep optional (QUIET find_package)
  - Matches current state
  - Transport works without libsodium

- **Option B:** Make required when `BEDROCK_WITH_TRANSPORT_DEPS=ON`
  - Enforces consistent environment
  - May block on platforms without libsodium

**Recommendation:** **Option A** — keep optional, not needed for basic echo RPC

**Your Decision:** ❓

---

### Question 8: Existing Generated Files

**Question:** What should we do with manually-generated proto files in `proto/`?

**Current:** `proto/bedrock_echo.pb.h/cc` and `proto/bedrock_echo.grpc.pb.h/cc` exist (manually generated)

**Options:**
- **Option A:** Delete them (generated files shouldn't be in source)
  - Clean source tree
  - CMake will regenerate in build directory

- **Option B:** Keep them (for reference/documentation)
  - Can inspect generated code
  - Clutters source tree
  - May confuse developers

**Recommendation:** **Option A** — delete manually-generated files, CMake will generate

**Your Decision:** ❓

---

## Summary

### Proposed CMake Structure (Summary)

1. **Add `BEDROCK_WITH_TRANSPORT_DEPS` option** (default OFF) to top-level `CMakeLists.txt`

2. **In top-level `CMakeLists.txt` (guarded by option):**
   - Find protobuf + gRPC (REQUIRED)
   - Locate protoc + grpc_cpp_plugin
   - Generate proto files to `${CMAKE_BINARY_DIR}/generated/echo/`

3. **In `tests/CMakeLists.txt` (guarded by option):**
   - Add `bedrock_echo_sanity` executable
   - Link generated proto files + echo service implementation
   - Link protobuf + gRPC libraries
   - Add CTest entry: `bedrock_echo_rpc`

4. **Update CI workflow:**
   - Add `-DBEDROCK_WITH_TRANSPORT_DEPS=ON` to Linux CI configure step
   - Build `bedrock_echo_sanity` target
   - Run `ctest -R bedrock_echo_rpc`

### Key Decisions Needed

1. CMake option default (ON/OFF) — **Recommendation: OFF**
2. Library vs direct linking — **Recommendation: Direct linking**
3. Test architecture — **Recommendation: Keep in-process**
4. CI integration — **Recommendation: Enable in Linux CI**
5. Generated files location — **Recommendation: Build directory**
6. macOS/Windows CI — **Recommendation: Linux only for now**
7. libsodium handling — **Recommendation: Keep optional**
8. Existing generated files — **Recommendation: Delete**

---

**Next Steps:** Await Mark's decisions on open questions, then proceed with implementation (Chunk 2B).

