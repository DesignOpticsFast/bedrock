# Bedrock Build Guide

**Last Updated:** 2025-01-26 (Sprint 4.5)  
**Purpose:** Build instructions and CMake configuration for Bedrock

---

## Quick Start

### Basic Build (Engine Only)

```bash
# Clone repository
git clone https://github.com/DesignOpticsFast/bedrock.git
cd bedrock

# Configure
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release -j
```

### Build with Palantir Transport

```bash
# Configure with transport dependencies
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBEDROCK_WITH_TRANSPORT_DEPS=ON \
  -DCMAKE_PREFIX_PATH=$HOME/Qt/6.10.1/macos/lib/cmake

# Build
cmake --build build --config Release -j
```

> **Note:** For current toolchain versions (Qt, C++ standard, CMake, Protobuf, etc.), see [VERSIONS.md](VERSIONS.md).

---

## CMake Options

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_TESTING` | `ON` | Build unit and integration tests |
| `BEDROCK_WITH_TRANSPORT_DEPS` | `OFF` | Enable Palantir transport (requires Qt) |
| `BEDROCK_WITH_OPENCASCADE` | `OFF` | Enable OpenCASCADE geometry backend (optional) |
| `BEDROCK_WITH_OCCT` | `OFF` | Legacy alias for BEDROCK_WITH_OPENCASCADE (deprecated) |

### Build Types

- **Release:** Optimized build for production
- **Debug:** Debug symbols, no optimization
- **RelWithDebInfo:** Optimized with debug symbols (recommended for development)

---

## Dependencies

### Required

- **C++20 compiler:** GCC 11+, Clang 14+, or MSVC 2019+
- **CMake 3.20+:** Build system
- **OpenMP:** Multithreading support (system-provided)
- **Protobuf 6.33+:** Palantir protocol serialization

### Optional (when `BEDROCK_WITH_TRANSPORT_DEPS=ON`)

- **Qt 6.10.1:** Palantir server (Core, Network components)
  - Set `CMAKE_PREFIX_PATH` to Qt installation
  - Example: `-DCMAKE_PREFIX_PATH=$HOME/Qt/6.10.1/macos/lib/cmake`

### Optional (when `BEDROCK_WITH_OCCT=ON`)

- **OpenCascade (OCCT):** Geometry operations
  - Set `OpenCASCADE_DIR` or `CMAKE_PREFIX_PATH` to OCCT installation
  - Example: `-DCMAKE_PREFIX_PATH=/opt/homebrew/opt/opencascade:${CMAKE_PREFIX_PATH}`

---

## Build Examples

### Crucible (macOS)

```bash
# Configure with transport deps
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBEDROCK_WITH_TRANSPORT_DEPS=ON \
  -DCMAKE_PREFIX_PATH=$HOME/Qt/6.10.1/macos/lib/cmake

# Build
cmake --build build --config RelWithDebInfo -j

# Run tests
cd build
ctest --output-on-failure
```

### Linux CI

```bash
# Install dependencies
sudo apt-get update
sudo apt-get install -y \
  cmake \
  ninja-build \
  g++ \
  libtbb-dev \
  libgtest-dev \
  libomp-dev \
  lcov

# Configure with transport deps
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBEDROCK_WITH_TRANSPORT_DEPS=ON \
  -DCMAKE_PREFIX_PATH=/opt/Qt/6.10.1/gcc_64/lib/cmake

# Build
cmake --build build --config RelWithDebInfo -j

# Run tests
cd build
ctest --output-on-failure
```

## macOS (Apple Silicon) OpenMP Notes

Bedrock requires OpenMP for parallel execution in its compute kernel.

macOS does not ship OpenMP with AppleClang.  
Install via Homebrew:

    brew install libomp

**Qt Creator users:** Select a CMake preset:

- macOS Debug (Apple Silicon)

- macOS Release (Apple Silicon)

OpenMP configuration is automatic via:

1. CMakePresets.json (for IDE/CLI builds)

2. Fallback auto-detection block in CMakeLists.txt

---

## BEDROCK_WITH_TRANSPORT_DEPS Explained

### Engine-Only Build (No Transport)

```bash
# Configure without transport
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DBEDROCK_WITH_TRANSPORT_DEPS=OFF \
  -DBEDROCK_WITH_OCCT=OFF

# Build
cmake --build build --config Release -j
```

---

## BEDROCK_WITH_TRANSPORT_DEPS Explained

**Purpose:** Controls whether Palantir transport layer is built

**When `ON`:**
- Builds `bedrock_palantir_server` library
- Builds `bedrock_server` executable
- Enables Palantir unit and integration tests
- Requires Qt 6.10.1 (Core, Network)

**When `OFF`:**
- Only builds core engine and math utilities
- No Qt dependency
- No Palantir transport code
- Smaller build footprint

**Default:** `OFF` (to allow engine-only builds without Qt)

**Usage:**
```bash
# Enable transport
cmake -S . -B build -DBEDROCK_WITH_TRANSPORT_DEPS=ON

# Disable transport (default)
cmake -S . -B build -DBEDROCK_WITH_TRANSPORT_DEPS=OFF
```

---

## Build Targets

### Libraries

- **`bedrock_core`:** Core utilities and threading
- **`bedrock_engine`:** Engine API implementation
- **`bedrock_som`:** System Object Model (header-only)
- **`bedrock_geom`:** Geometry operations (when OCCT enabled)
- **`bedrock_palantir_proto`:** Protobuf message definitions
- **`bedrock_palantir_server`:** Palantir server (when transport enabled)
- **`bedrock_capabilities_service`:** Capabilities service (when transport enabled)

### Executables

- **`bedrock_server`:** Palantir server executable (when transport enabled)
- **`bedrock_tests`:** Unit test executable
- **`integration_tests`:** Integration test executable (when transport enabled)

---

## Testing

### Build Tests

```bash
# Configure with tests
cmake -S . -B build -DBUILD_TESTING=ON

# Build
cmake --build build -j

# Run tests
cd build
ctest --output-on-failure
```

### Run Specific Tests

```bash
# Unit tests only
cd build
ctest -R bedrock_tests

# Integration tests only
cd build
ctest -R integration_tests
```

> **Note:** For detailed testing documentation, see [TESTING.md](TESTING.md).

---

## Troubleshooting

### CMake Cannot Find Qt

**Problem:** `Could not find a package configuration file provided by "Qt6"`

**Solution:**
```bash
# Set CMAKE_PREFIX_PATH to Qt installation
cmake -S . -B build \
  -DCMAKE_PREFIX_PATH=$HOME/Qt/6.10.1/macos/lib/cmake
```

### OpenMP Not Found

**Problem:** `OpenMP not found - required for multithreading support`

**Solution:**
```bash
# Linux
sudo apt-get install -y libomp-dev

# macOS (Homebrew)
brew install libomp
```

### Protobuf Not Found

**Problem:** `Could not find a package configuration file provided by "Protobuf"`

**Solution:**
```bash
# Install Protobuf 6.33+
# Linux
sudo apt-get install -y libprotobuf-dev protobuf-compiler

# macOS (Homebrew)
brew install protobuf
```

### Build Fails with C++20 Errors

**Problem:** Compiler doesn't support C++20

**Solution:**
- Ensure compiler is C++20-capable (GCC 11+, Clang 14+, MSVC 2019+)
- Check `CMAKE_CXX_STANDARD` is set to 20 (default in Bedrock)

---

## Related Documentation

- **[VERSIONS.md](VERSIONS.md)** - Toolchain versions
- **[dev-setup.md](dev-setup.md)** - Development environment setup
- **[TESTING.md](TESTING.md)** - Testing guide
- **[REPOSITORY_STRUCTURE.md](REPOSITORY_STRUCTURE.md)** - Directory layout

---

**Last Updated:** 2025-01-26 (Sprint 4.5)

