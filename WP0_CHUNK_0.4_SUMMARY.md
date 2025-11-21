# WP0 Chunk 0.4: Protobuf, gRPC & libsodium Baseline - Completion Summary

**Date:** 2025-11-17  
**Status:** ✅ Complete  
**Build Directory:** `build/transport/`

---

## Summary

Successfully integrated protobuf, gRPC, and libsodium into the Bedrock build system with a minimal, non-invasive approach. Transport dependencies are now available on dev-01 via CMake option `BEDROCK_WITH_TRANSPORT_DEPS`, while CI on ubuntu-latest remains green with the option disabled by default.

---

## Changes Made

### 1. CMake Option Added

**Added `BEDROCK_WITH_TRANSPORT_DEPS` option:**
- **Default:** `OFF` (ensures CI remains green)
- **Purpose:** Enable gRPC/protobuf/libsodium for transport functionality
- **Location:** Top-level CMakeLists.txt, with other options

### 2. Transport Dependency Detection

**Protobuf Detection:**
- ✅ **Found:** Version 3.19.6
- **Method:** CONFIG mode attempted first, then MODULE mode fallback
- **Result:** Found via MODULE mode (`Protobuf_DIR` not found, but `Protobuf_LIBRARY_RELEASE` and `Protobuf_INCLUDE_DIR` set)
- **Variables:** `Protobuf_VERSION`, `Protobuf_LIBRARY_RELEASE`, `Protobuf_INCLUDE_DIR`, `Protobuf_PROTOC_EXECUTABLE`

**gRPC Detection:**
- ✅ **Found:** Version 1.60.2
- **Method:** CONFIG mode attempted first, then MODULE mode fallback
- **Result:** `gRPC_DIR` found at `/usr/lib64/cmake/grpc`, but used MODULE mode variables
- **Variables:** `gRPC_VERSION`, `gRPC_FOUND`, `gRPC_DIR`

**libsodium Detection:**
- ❌ **Not Found:** Not installed on dev-01
- **Method:** Attempted `find_package(sodium QUIET)` and `find_package(libsodium QUIET)`
- **Result:** Optional dependency - warnings emitted but build continues
- **Status:** Will be addressed in future chunks (install/vendor or alternative crypto)

### 3. Transport Deps Sanity Target

**Created `bedrock_transport_deps_sanity` executable:**
- **Source:** `tests/bedrock_transport_deps_sanity.cpp`
- **Purpose:** Verify protobuf, gRPC, and libsodium headers/libraries are accessible
- **Build Condition:** Only built when `BEDROCK_WITH_TRANSPORT_DEPS=ON` AND protobuf AND gRPC are found
- **Linking:** Links against protobuf and gRPC (libsodium optional)
- **CTest:** Registered as test if target builds successfully

**Source Code:**
```cpp
#include <google/protobuf/message.h>
#include <grpcpp/grpcpp.h>
#ifdef HAVE_LIBSODIUM
#include <sodium.h>
#endif

int main() {
#ifdef HAVE_LIBSODIUM
    if (sodium_init() < 0) {
        return 1;
    }
#endif
    return 0;
}
```

### 4. Flexible Linking Strategy

**Handles both CONFIG and MODULE modes:**
- Checks for imported targets first (`protobuf::libprotobuf`, `gRPC::grpc++`)
- Falls back to library variables (`Protobuf_LIBRARIES`, `gRPC_LIBRARIES`)
- Final fallback to simple library names (`protobuf`, `grpc++`)
- Ensures compatibility across different installation methods

---

## Verification Results

### On dev-01 with `BEDROCK_WITH_TRANSPORT_DEPS=ON`

**Configuration:**
```bash
cmake -S . -B build/transport -DBEDROCK_WITH_TRANSPORT_DEPS=ON
```

**Output:**
```
-- Transport dependencies requested (BEDROCK_WITH_TRANSPORT_DEPS=ON)
-- Protobuf found: 3.19.6
-- gRPC found: 1.60.2
CMake Warning: libsodium not found - will be optional for transport deps sanity
-- Configuring done
-- Generating done
```

**Build:**
```bash
cmake --build build/transport --target bedrock_transport_deps_sanity
```

**Result:**
- ✅ Target builds successfully
- ✅ Links against protobuf and gRPC
- ✅ No libsodium linking (not found, optional)

**Execution:**
```bash
./build/transport/bedrock_transport_deps_sanity
```

**Result:**
- ✅ Executable runs successfully
- ✅ Exit code: 0

**CTest:**
```bash
cd build/transport && ctest --output-on-failure
```

**Result:**
- ✅ `bedrock_transport_deps_sanity` test passes
- ✅ Test registered and executable

### CI Verification (Option OFF - Default)

**Configuration:**
```bash
cmake -S . -B build/ci -DCMAKE_BUILD_TYPE=Release
```

**Result:**
- ✅ Configuration succeeds (no transport deps requested)
- ✅ Build succeeds (bedrock_engine + bedrock_sanity)
- ✅ CTest passes (bedrock_sanity test)
- ✅ No regressions introduced

---

## Dependency Status on dev-01

| Dependency | Status | Version | Detection Method | Notes |
|------------|--------|--------|------------------|-------|
| **protobuf** | ✅ Found | 3.19.6 | MODULE mode | CONFIG mode not available |
| **gRPC** | ✅ Found | 1.60.2 | MODULE mode | CONFIG mode not available |
| **libsodium** | ❌ Not Found | N/A | Not installed | Optional for now |

### Package Details

**Installed packages (via rpm/dnf):**
- `protobuf-3.19.6-1.amzn2023.0.1.x86_64`
- `protobuf-devel-3.19.6-1.amzn2023.0.1.x86_64`
- `protobuf-compiler-3.19.6-1.amzn2023.0.1.x86_64`
- `grpc-1.60.2-10.amzn2023.0.1.x86_64`
- `grpc-devel-1.60.2-10.amzn2023.0.1.x86_64`
- `grpc-cpp-1.60.2-10.amzn2023.0.1.x86_64`
- `grpc-plugins-1.60.2-10.amzn2023.0.1.x86_64`

**Headers available:**
- `/usr/include/google/protobuf/message.h`
- `/usr/include/grpcpp/grpcpp.h`

**pkg-config:**
- `protobuf`: 3.19.6
- `grpc++`: 1.60.2
- `libsodium`: Not found

---

## CMake Implementation Details

### Option Definition

```cmake
option(BEDROCK_WITH_TRANSPORT_DEPS "Enable gRPC/protobuf/libsodium for transport" OFF)
```

### Dependency Detection Pattern

```cmake
if(BEDROCK_WITH_TRANSPORT_DEPS)
    # Try CONFIG mode first
    find_package(Protobuf CONFIG QUIET)
    if(NOT Protobuf_FOUND)
        # Fall back to MODULE mode
        find_package(Protobuf QUIET)
    endif()
    # Similar pattern for gRPC and libsodium
endif()
```

### Target Creation

```cmake
if(HAVE_PROTOBUF AND HAVE_GRPC)
    add_executable(bedrock_transport_deps_sanity ...)
    # Flexible linking handles CONFIG/MODULE modes
    target_link_libraries(bedrock_transport_deps_sanity PRIVATE ...)
    add_test(NAME bedrock_transport_deps_sanity COMMAND bedrock_transport_deps_sanity)
endif()
```

---

## Verification Checklist

- [x] `BEDROCK_WITH_TRANSPORT_DEPS` option added (default OFF)
- [x] `find_package` calls for protobuf, gRPC, libsodium added (CONFIG first, MODULE fallback)
- [x] `bedrock_transport_deps_sanity` target created and links successfully
- [x] CTest entry added (when target builds)
- [x] Verified on dev-01 with option ON (protobuf + gRPC work)
- [x] libsodium documented as not installed (optional for now)
- [x] CI verified still works with option OFF (default)
- [x] No old/legacy targets re-enabled
- [x] Detection method documented (MODULE mode for protobuf/gRPC)

---

## Notes

- **Detection Mode:** Protobuf found via MODULE mode (`Protobuf_DIR` not found). gRPC has `gRPC_DIR` set but used MODULE mode variables for linking.
- **libsodium:** Not installed on dev-01 - optional dependency for now, will be addressed in future chunks
- **CI Safety:** Option defaults to OFF, ensuring CI on ubuntu-latest remains green
- **Flexible Linking:** Handles both CONFIG and MODULE mode targets for maximum compatibility
- **Error Handling:** Warnings emitted for missing deps, but CMake does not fail (exploratory mode)
- **Ready for Echo RPC:** Transport dependencies are now available for Chunk 0.5 (Echo RPC implementation)

---

## Next Steps (Future Chunks)

1. **Chunk 0.5:** Implement Echo RPC using protobuf + gRPC
2. **libsodium:** Install/vendor libsodium or choose alternative crypto approach
3. **Stricter Mode:** Consider making deps required when option is ON (after stability confirmed)

---

**Chunk completed by:** UnderLord (AI Assistant)  
**Verification:** All checks passed  
**Ready for:** WP0 Chunk 0.5 (Echo RPC + standalone client)

