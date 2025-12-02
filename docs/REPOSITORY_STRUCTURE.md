# Bedrock Repository Structure

**Last Updated:** 2025-01-26 (Sprint 4.5)  
**Purpose:** Directory layout and component organization

---

## Overview

Bedrock is organized as a modular C++ library with clear separation between transport, compute, geometry, and documentation layers.

---

## Directory Layout

```
bedrock/
├── CMakeLists.txt              # Top-level build configuration
├── README.md                   # Project overview
│
├── include/                    # Public C++ headers
│   └── bedrock/
│       ├── engine.hpp          # Engine API
│       ├── som/
│       │   └── types.hpp       # SOM v0 types
│       ├── geom/
│       │   └── step_export.hpp # STEP export (planned)
│       └── threading.hpp       # OpenMP utilities
│
├── src/                        # Implementation files
│   ├── engine/
│   │   └── engine.cpp          # Engine API implementation
│   ├── palantir/               # Palantir transport layer
│   │   ├── PalantirServer.hpp/cpp
│   │   ├── CapabilitiesService.hpp/cpp
│   │   ├── EnvelopeHelpers.hpp/cpp
│   │   └── bedrock_server.cpp  # Server executable
│   ├── geom/
│   │   └── step_export.cpp     # STEP export (planned)
│   └── plugin/
│       └── plugin.cpp           # Plugin infrastructure
│
├── core/                       # Core utilities
│   ├── CMakeLists.txt
│   ├── include/bedrock/
│   │   └── threading.hpp       # Threading utilities
│   └── src/
│       └── threading.cpp       # OpenMP implementation
│
├── som/                        # System Object Model
│   └── (SOM definitions - header-only)
│
├── tests/                      # Test suite
│   ├── CMakeLists.txt
│   ├── test_math.cpp           # Math unit tests
│   ├── palantir/               # Palantir unit tests
│   │   ├── EnvelopeHelpers_test.cpp
│   │   ├── ErrorResponse_test.cpp
│   │   └── CapabilitiesService_test.cpp
│   └── integration/            # Integration tests
│       ├── CMakeLists.txt
│       ├── IntegrationTestServerFixture.hpp/cpp
│       ├── IntegrationTestClient.hpp/cpp
│       ├── CapabilitiesIntegrationTest.cpp
│       ├── XYSineIntegrationTest.cpp
│       ├── ErrorCasesIntegrationTest.cpp
│       └── EdgeCasesIntegrationTest.cpp
│
├── docs/                       # Documentation
│   ├── BEDROCK_ARCHITECTURE.md
│   ├── SOM_OVERVIEW.md
│   ├── BEDROCK_BUILD.md
│   ├── TESTING.md
│   ├── DEPLOYMENT.md
│   ├── OPENCASCADE_INTEGRATION.md
│   ├── REPOSITORY_STRUCTURE.md (this file)
│   ├── THREADING.md
│   ├── VERSIONS.md
│   ├── dev-setup.md
│   ├── engine-api.md
│   ├── palantir/               # Palantir documentation
│   │   ├── README.md
│   │   ├── IMPLEMENTATION_STATUS.md
│   │   └── proto/              # Protobuf definitions
│   └── sprint4.5/              # Sprint 4.5 documentation
│
├── scripts/                    # Build and utility scripts
│   ├── auto-merge.sh
│   └── doc-review.sh
│
├── .github/                    # GitHub Actions workflows
│   └── workflows/
│       ├── ci.yml              # CI/CD
│       ├── codeql.yml          # Security scanning
│       └── pr-guard.yml        # PR validation
│
└── build/                      # Build output (gitignored)
    ├── lib/                    # Libraries
    ├── bin/                    # Executables
    └── generated/              # Generated files (protobuf)
```

---

## Key Components

### Transport Layer

**Location:** `src/palantir/`

**Components:**
- **PalantirServer:** Qt-based IPC server
- **CapabilitiesService:** Capabilities RPC implementation
- **EnvelopeHelpers:** MessageEnvelope utilities
- **bedrock_server:** Server executable

**Dependencies:** Qt 6.10.1 (when `BEDROCK_WITH_TRANSPORT_DEPS=ON`)

### Compute Engine

**Location:** `src/engine/`, `core/`

**Components:**
- **Engine API:** Core computation interface
- **Math Utilities:** Mathematical operations
- **Threading:** OpenMP/TBB support

**Dependencies:** OpenMP (required)

### System Object Model (SOM)

**Location:** `som/`, `include/bedrock/som/`

**Components:**
- **SOM v0:** Two-Surface Element (TSE)
- **Surface Definitions:** Optical surface structures
- **Material Properties:** Material definitions

**Dependencies:** None (header-only)

### Geometry

**Location:** `src/geom/`, `include/bedrock/geom/`

**Components:**
- **STEP Export:** CAD export (planned)
- **Surface Geometry:** Geometric operations (planned)

**Dependencies:** OpenCascade (when `BEDROCK_WITH_OCCT=ON`)

### Tests

**Location:** `tests/`

**Components:**
- **Unit Tests:** GoogleTest-based unit tests
- **Integration Tests:** End-to-end Palantir tests

**Dependencies:** GoogleTest, Qt (for integration tests)

### Documentation

**Location:** `docs/`

**Components:**
- **Architecture:** System design and components
- **Build:** Build instructions and CMake options
- **Testing:** Test infrastructure and guidelines
- **Palantir:** Transport protocol documentation
- **Sprint Docs:** Sprint-specific documentation

---

## Build Tree Structure

### Build Output

```
build/
├── lib/                        # Static libraries
│   ├── libbedrock_core.a
│   ├── libbedrock_engine.a
│   ├── libbedrock_palantir_proto.a
│   ├── libbedrock_palantir_server.a
│   └── libbedrock_capabilities_service.a
│
├── bin/                        # Executables
│   └── bedrock_server
│
├── generated/                  # Generated files
│   └── palantir/              # Protobuf generated code
│       ├── capabilities.pb.h/cpp
│       ├── envelope.pb.h/cpp
│       ├── error.pb.h/cpp
│       └── xysine.pb.h/cpp
│
└── tests/                     # Test executables
    ├── bedrock_tests
    └── integration/
        └── integration_tests
```

---

## Component Dependencies

```
bedrock_core
  └─> (no dependencies)

bedrock_engine
  └─> bedrock_core

bedrock_som
  └─> (header-only, no dependencies)

bedrock_geom
  └─> bedrock_som
  └─> OpenCascade (optional)

bedrock_palantir_proto
  └─> Protobuf

bedrock_capabilities_service
  └─> bedrock_palantir_proto

bedrock_palantir_server
  └─> bedrock_palantir_proto
  └─> bedrock_capabilities_service
  └─> bedrock_engine
  └─> Qt 6.10.1 (optional)
```

---

## Related Documentation

- **[BEDROCK_ARCHITECTURE.md](BEDROCK_ARCHITECTURE.md)** - Architecture overview
- **[BEDROCK_BUILD.md](BEDROCK_BUILD.md)** - Build instructions
- **[SOM_OVERVIEW.md](SOM_OVERVIEW.md)** - SOM details

---

**Last Updated:** 2025-01-26 (Sprint 4.5)

