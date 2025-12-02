# Bedrock Testing Guide

**Last Updated:** 2025-01-26 (Sprint 4.5)  
**Purpose:** Testing infrastructure and guidelines for Bedrock

---

## Overview

Bedrock uses **GoogleTest** for unit tests and a custom integration test framework for Palantir transport testing. All tests run in CI on Linux and macOS.

---

## Test Structure

### Unit Tests

**Location:** `tests/palantir/`, `tests/`

**Framework:** GoogleTest

**Examples:**
- `tests/palantir/EnvelopeHelpers_test.cpp` - Envelope encoding/decoding
- `tests/palantir/ErrorResponse_test.cpp` - Error response handling
- `tests/palantir/CapabilitiesService_test.cpp` - Capabilities service
- `tests/test_math.cpp` - Math utilities

**Build Target:** `bedrock_tests`

### Integration Tests

**Location:** `tests/integration/`

**Framework:** Custom integration test framework with `IntegrationTestServerFixture`

**Examples:**
- `tests/integration/CapabilitiesIntegrationTest.cpp` - Capabilities RPC end-to-end
- `tests/integration/XYSineIntegrationTest.cpp` - XY Sine RPC end-to-end
- `tests/integration/ErrorCasesIntegrationTest.cpp` - Error handling
- `tests/integration/EdgeCasesIntegrationTest.cpp` - Edge cases (concurrency, rapid connect/disconnect)

**Build Target:** `integration_tests`

**Requirements:** `BEDROCK_WITH_TRANSPORT_DEPS=ON` (requires Qt)

---

## Running Tests Locally

### Build Tests

```bash
# Configure with tests
cmake -S . -B build \
  -DBUILD_TESTING=ON \
  -DBEDROCK_WITH_TRANSPORT_DEPS=ON \
  -DCMAKE_PREFIX_PATH=$HOME/Qt/6.10.1/macos/lib/cmake

# Build
cmake --build build -j
```

### Run All Tests

```bash
cd build
ctest --output-on-failure
```

### Run Specific Test Suite

```bash
# Unit tests only
cd build
ctest -R bedrock_tests

# Integration tests only
cd build
ctest -R integration_tests
```

### Run Individual Test

```bash
# Run specific test executable
cd build
./tests/bedrock_tests --gtest_filter=EnvelopeHelpersTest.*

# Run specific integration test
cd build
./tests/integration/integration_tests --gtest_filter=CapabilitiesIntegrationTest.*
```

---

## Palantir Test Infrastructure

### IntegrationTestServerFixture

**Purpose:** In-process Palantir server for integration tests

**Location:** `tests/integration/IntegrationTestServerFixture.hpp/cpp`

**Usage:**
```cpp
#include "IntegrationTestServerFixture.hpp"

TEST_F(IntegrationTestServerFixture, CapabilitiesTest) {
    auto response = client_->getCapabilities();
    ASSERT_TRUE(response.has_value());
    // ... verify response
}
```

**Features:**
- Starts Palantir server in-process
- Provides `IntegrationTestClient` for RPC calls
- Automatic cleanup on test completion
- Thread-safe for concurrent tests

### IntegrationTestClient

**Purpose:** Client for Palantir RPC calls in tests

**Location:** `tests/integration/IntegrationTestClient.hpp/cpp`

**Methods:**
- `getCapabilities()` - Capabilities RPC
- `sendXYSineRequest()` - XY Sine RPC
- `connect()` / `disconnect()` - Connection management

---

## Test Coverage

### Coverage Measurement

**Tool:** gcov/lcov

**CI:** Coverage reports generated on Linux CI

**Location:** `coverage-html/` artifact in CI

**Focus Areas:**
- `src/palantir/*` - Palantir transport layer
- `src/engine/*` - Engine API
- `core/*` - Core utilities

**Target:** ~80% coverage for Palantir-related code

### Viewing Coverage

```bash
# Generate coverage report locally (Linux)
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_CXX_FLAGS="--coverage"

cmake --build build -j
cd build
ctest

# Generate HTML report
lcov --directory . --capture --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage-html

# Open in browser
open coverage-html/index.html
```

---

## CI Testing

### GitHub Actions

**Workflow:** `.github/workflows/ci.yml`

**Jobs:**
- **`build`:** Engine-only build (no transport)
- **`tests`:** Unit + integration tests on Linux and macOS

**Test Execution:**
```yaml
- name: Run tests
  run: |
    cmake --build build --config Release -j
    ctest --test-dir build --output-on-failure
```

**Coverage:**
- Generated on Linux CI
- Uploaded as `coverage-html` artifact
- Filtered to exclude system/test files

---

## Writing Tests

### Unit Test Example

```cpp
#include <gtest/gtest.h>
#include "EnvelopeHelpers.hpp"

TEST(EnvelopeHelpersTest, MakeEnvelope) {
    palantir::CapabilitiesRequest request;
    auto envelope = makeEnvelope(palantir::MessageType::CAPABILITIES_REQUEST, request);
    
    ASSERT_EQ(envelope.version(), 1);
    ASSERT_EQ(envelope.type(), palantir::MessageType::CAPABILITIES_REQUEST);
    ASSERT_FALSE(envelope.payload().empty());
}
```

### Integration Test Example

```cpp
#include "IntegrationTestServerFixture.hpp"

TEST_F(IntegrationTestServerFixture, XYSineRequest) {
    palantir::XYSineRequest request;
    request.set_samples(1000);
    request.set_frequency(1.0);
    request.set_amplitude(1.0);
    request.set_phase(0.0);
    
    auto response = client_->sendXYSineRequest(request);
    ASSERT_TRUE(response.has_value());
    ASSERT_EQ(response->x_size(), 1000);
    ASSERT_EQ(response->y_size(), 1000);
}
```

---

## Test Guidelines

### Unit Tests

- **Focus:** Single function or class
- **Isolation:** No external dependencies
- **Speed:** Fast execution (< 1 second per test)
- **Coverage:** All code paths, edge cases

### Integration Tests

- **Focus:** End-to-end RPC flows
- **Realism:** Use actual Palantir server
- **Scope:** Error cases, edge cases, concurrency
- **Reliability:** Deterministic, no flakiness

### Best Practices

1. **Test Naming:** `ComponentNameTest.TestName` (unit) or `FeatureIntegrationTest.TestName` (integration)
2. **Assertions:** Use `ASSERT_*` for fatal failures, `EXPECT_*` for non-fatal
3. **Cleanup:** Use fixtures for automatic cleanup
4. **Isolation:** Each test should be independent
5. **Documentation:** Add comments for complex test logic

---

## Related Documentation

- **[BEDROCK_BUILD.md](BEDROCK_BUILD.md)** - Build instructions
- **[VERSIONS.md](VERSIONS.md)** - Toolchain versions
- **[THREADING.md](THREADING.md)** - Threading model (relevant for concurrent tests)

---

**Last Updated:** 2025-01-26 (Sprint 4.5)

