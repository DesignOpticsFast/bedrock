# Building Bedrock

## Requirements

- C++20-capable compiler (GCC 11+, Clang 14+, MSVC 2019+)
- CMake 3.20 or later
- (Optional) Protobuf, gRPC, libsodium for transport

## Quick Start

### Basic Build (No Transport)

```bash
git clone <repo-url>
cd bedrock
git submodule update --init --recursive  # For contracts/
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### Build with Transport Dependencies

```bash
cmake -S . -B build \
  -DBUILD_TESTING=ON \
  -DBEDROCK_WITH_TRANSPORT_DEPS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Build Options

- `BUILD_TESTING` - Enable tests (default: ON)
- `BEDROCK_WITH_TRANSPORT_DEPS` - Enable transport deps (default: OFF)

## Build Targets

- `bedrock_engine` - Core engine library (static)
- `bedrock_sanity` - Basic sanity test
- `bedrock_transport_deps_sanity` - Transport deps test (if enabled)
- `bedrock_echo_sanity` - Echo RPC test (if transport enabled)

## CMake Structure

Bedrock uses modular CMake:
- `cmake/Options.cmake` - Build options
- `cmake/Dependencies.cmake` - Dependency discovery
- `cmake/Targets.cmake` - Target definitions

## Notes

- Contract submodule must be initialized for proto generation
- Transport dependencies are optional (can build without)
- Tests run on dev-01 with BUILD_TESTING=ON

