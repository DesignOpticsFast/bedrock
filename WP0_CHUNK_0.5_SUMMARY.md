# WP0 Chunk 0.5: Bedrock Echo RPC + Standalone Client - Completion Summary

**Date:** 2025-11-17  
**Status:** ✅ Complete  
**Build Directory:** `build/transport/`

---

## Summary

Successfully implemented a minimal Echo RPC in Bedrock using gRPC + protobuf. The implementation includes a standalone sanity executable that starts an Echo service, calls it, verifies the response, and exits successfully. All echo-related code is guarded by `BEDROCK_WITH_TRANSPORT_DEPS` and does not affect CI when the option is OFF.

---

## Changes Made

### 1. Proto File Created

**Created `proto/bedrock_echo.proto`:**
- **Location:** `proto/bedrock_echo.proto` (root-level proto directory)
- **Package:** `bedrock.echo`
- **Service:** `EchoService` with single `Echo` RPC method
- **Messages:** `EchoRequest` and `EchoReply` (both contain single `string message` field)

**Rationale:** Palantir contracts submodule (`contracts/`) does not contain any `.proto` files yet (only documentation). Created local proto file for Phase 0 testing. Will migrate to Palantir contracts in future chunks.

### 2. CMake Proto Code Generation

**Added proto generation block (guarded by `BEDROCK_WITH_TRANSPORT_DEPS`):**
- **Generated directory:** `${CMAKE_BINARY_DIR}/generated/echo`
- **Tools:** Uses `protoc` and `grpc_cpp_plugin` (found at `/usr/bin/`)
- **Generated files:**
  - `bedrock_echo.pb.h` / `bedrock_echo.pb.cc` (protobuf messages)
  - `bedrock_echo.grpc.pb.h` / `bedrock_echo.grpc.pb.cc` (gRPC service stubs)
- **Method:** Explicit `add_custom_command` with protoc invocations
- **Dependencies:** Properly tracks proto file changes

### 3. Echo Server Implementation

**Created `src/echo/echo_service_impl.h` and `src/echo/echo_service_impl.cpp`:**
- **Class:** `EchoServiceImpl` inherits from generated `EchoService::Service`
- **Method:** Implements `Echo` RPC method (simple identity: returns same message)
- **Error handling:** Minimal (returns `grpc::Status::OK`)

### 4. Echo Sanity Executable

**Created `tests/bedrock_echo_sanity.cpp`:**
- **Architecture:** Single executable (Option B - server + client in one process)
- **Server:** Starts gRPC server on `localhost:50051` in background thread
- **Client:** Creates channel and stub, calls Echo RPC
- **Verification:** Checks response matches request message
- **Cleanup:** Shuts down server and exits with code 0 on success

**Implementation details:**
- Server runs in `std::thread` to allow client to connect
- Uses `grpc::InsecureServerCredentials()` and `grpc::InsecureChannelCredentials()`
- 100ms delay after server start to ensure readiness
- Minimal error handling (logs to `std::cerr` and returns non-zero on failure)

### 5. CMake Target Integration

**Added `bedrock_echo_sanity` executable target:**
- **Sources:** `tests/bedrock_echo_sanity.cpp`, `src/echo/echo_service_impl.cpp`, generated proto sources
- **Include directories:** Generated directory, source directories
- **Linking:** Protobuf and gRPC (handles both CONFIG and MODULE modes)
- **Guard:** Only built when `BEDROCK_WITH_TRANSPORT_DEPS=ON` AND protobuf/gRPC found

### 6. CTest Integration

**Added CTest entry:**
- **Test name:** `bedrock_echo_sanity`
- **Command:** `bedrock_echo_sanity` executable
- **Guard:** Only added if target exists

---

## Verification Results

### On dev-01 with `BEDROCK_WITH_TRANSPORT_DEPS=ON`

**Configuration:**
```bash
cmake -S . -B build/transport -DBEDROCK_WITH_TRANSPORT_DEPS=ON
```

**Output:**
```
-- Protobuf found: 3.19.6
-- gRPC found: 1.60.2
-- Echo RPC proto generation configured
-- Configuring done
-- Generating done
```

**Build:**
```bash
cmake --build build/transport --target bedrock_echo_sanity
```

**Result:**
- ✅ Proto files generated successfully
- ✅ Target builds successfully
- ✅ Links against protobuf and gRPC

**Execution:**
```bash
./build/transport/bedrock_echo_sanity
```

**Result:**
- ✅ Executable runs successfully
- ✅ Server starts on localhost:50051
- ✅ Client connects and calls Echo RPC
- ✅ Response matches request ("hello" → "hello")
- ✅ Exit code: 0

**CTest:**
```bash
cd build/transport && ctest -R bedrock_echo_sanity --output-on-failure
```

**Result:**
- ✅ Test passes
- ✅ Execution time: 0.12 seconds

### CI Verification (Option OFF - Default)

**Configuration:**
```bash
cmake -S . -B build/ci -DCMAKE_BUILD_TYPE=Release
```

**Result:**
- ✅ Configuration succeeds (no echo targets generated)
- ✅ Build succeeds (bedrock_engine + bedrock_sanity only)
- ✅ CTest passes (bedrock_sanity test only)
- ✅ No regressions introduced
- ✅ CI remains green

---

## File Structure

```
bedrock/
├── proto/
│   └── bedrock_echo.proto          # NEW: Echo service proto definition
├── src/
│   └── echo/                       # NEW: Echo service implementation
│       ├── echo_service_impl.h
│       └── echo_service_impl.cpp
├── tests/
│   └── bedrock_echo_sanity.cpp     # NEW: Echo sanity executable (server + client)
└── build/transport/
    └── generated/echo/              # Generated proto code (build-time)
        ├── bedrock_echo.pb.h
        ├── bedrock_echo.pb.cc
        ├── bedrock_echo.grpc.pb.h
        └── bedrock_echo.grpc.pb.cc
```

---

## Implementation Details

### Proto Definition

```protobuf
syntax = "proto3";
package bedrock.echo;

service EchoService {
  rpc Echo(EchoRequest) returns (EchoReply);
}

message EchoRequest {
  string message = 1;
}

message EchoReply {
  string message = 1;
}
```

### Server Implementation

```cpp
grpc::Status EchoServiceImpl::Echo(grpc::ServerContext* context,
                                     const EchoRequest* request,
                                     EchoReply* reply) {
    reply->set_message(request->message());
    return grpc::Status::OK;
}
```

### Endpoint

- **Type:** TCP
- **Address:** `localhost:50051`
- **Credentials:** Insecure (for Phase 0 testing)

### Architecture

- **Single executable:** `bedrock_echo_sanity`
- **Server:** Runs in background thread
- **Client:** Creates stub and calls RPC in main thread
- **Verification:** Checks response matches request
- **Cleanup:** Shuts down server before exit

---

## Verification Checklist

- [x] Local proto file created (`proto/bedrock_echo.proto`)
- [x] CMake proto generation set up (generates .pb.cc/.pb.h and .grpc.pb.cc/.grpc.pb.h)
- [x] Echo server implemented (`EchoServiceImpl` class)
- [x] Echo sanity executable created (server + client in one process)
- [x] Single executable approach (Option B)
- [x] CTest entry added (`bedrock_echo_sanity`)
- [x] All targets guarded by `BEDROCK_WITH_TRANSPORT_DEPS`
- [x] Verified on dev-01 with option ON (builds and runs successfully)
- [x] CI verified still works with option OFF (no regressions)
- [x] Endpoint: TCP localhost:50051
- [x] Request/response verification works correctly

---

## Notes

- **Proto location:** Local `proto/bedrock_echo.proto` (not in Palantir contracts yet)
- **Architecture:** Single executable for simplicity (can split later for Phoenix ↔ Bedrock)
- **Endpoint:** TCP localhost:50051 (standard gRPC port, easy to debug)
- **Generated code:** Stored in `build/transport/generated/echo/` (keeps source tree clean)
- **Error handling:** Minimal (just enough to verify it works)
- **CI safety:** All echo code guarded by `BEDROCK_WITH_TRANSPORT_DEPS`, defaults to OFF
- **Ready for:** Future integration with Phoenix and Palantir contracts

---

## Next Steps (Future Chunks)

1. **Chunk 0.6:** Preflight + README + `.contract-version` + CI parity check
2. **Future:** Migrate to Palantir contracts when `.proto` files are added
3. **Future:** Split server/client for Phoenix ↔ Bedrock communication
4. **Future:** Add proper error handling and status codes
5. **Future:** Add Unix domain socket support (UDS) for production

---

**Chunk completed by:** UnderLord (AI Assistant)  
**Verification:** All checks passed  
**Ready for:** WP0 Chunk 0.6 (Preflight + README + .contract-version + CI parity)

