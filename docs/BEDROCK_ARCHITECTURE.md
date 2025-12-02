# Bedrock Architecture

**Last Updated:** 2025-01-26 (Sprint 4.5)  
**Purpose:** High-level architecture overview of Bedrock

---

## Overview

Bedrock is a **headless, computationally efficient C++ library** that serves as the core compute engine for the Phoenix platform. It provides optical design calculations, ray tracing, and geometry operations without any GUI dependencies.

**Key Principles:**
- **Headless:** No GUI components; all UI lives in Phoenix
- **Modular:** Clean separation between transport, compute, and geometry layers
- **AI-first:** Designed for future AI-assisted workflows
- **Open:** Public APIs for extensibility

---

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Phoenix (GUI)                        │
│              (Qt-based IDE, separate repo)              │
└───────────────────────┬─────────────────────────────────┘
                        │
                        │ Palantir IPC
                        │ (QLocalSocket + Protobuf)
                        │
┌───────────────────────▼─────────────────────────────────┐
│              Palantir Transport Layer                   │
│  ┌─────────────────────────────────────────────────┐   │
│  │  PalantirServer (QLocalSocket + MessageEnvelope)│   │
│  │  - Capabilities RPC (returns Bedrock version)    │   │
│  │  - XY Sine RPC                                   │   │
│  │  - Error handling                                │   │
│  └─────────────────────────────────────────────────┘   │
└───────────────────────┬─────────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────────┐
│              Compute Engine Layer                        │
│  ┌─────────────────────────────────────────────────┐   │
│  │  Engine API                                     │   │
│  │  - XY Sine computation                          │   │
│  │  - Future: Ray tracing, wave optics, etc.      │   │
│  └─────────────────────────────────────────────────┘   │
│  ┌─────────────────────────────────────────────────┐   │
│  │  Core Utilities                                 │   │
│  │  - Math utilities                                │   │
│  │  - Threading (OpenMP/TBB)                        │   │
│  └─────────────────────────────────────────────────┘   │
└───────────────────────┬─────────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────────┐
│              SOM (System Object Model) Layer            │
│  ┌─────────────────────────────────────────────────┐   │
│  │  SOM v0                                          │   │
│  │  - Two-Surface Element (TSE)                     │   │
│  │  - Surface definitions                           │   │
│  │  - Material properties                           │   │
│  └─────────────────────────────────────────────────┘   │
└───────────────────────┬─────────────────────────────────┘
                        │
┌───────────────────────▼─────────────────────────────────┐
│              Geometry Layer                              │
│  ┌─────────────────────────────────────────────────┐   │
│  │  OpenCascade Integration (Planned)              │   │
│  │  - STEP export                                  │   │
│  │  - Surface geometry                             │   │
│  └─────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

---

## Major Components

### 1. Palantir Transport Layer

**Location:** `src/palantir/`

**Purpose:** IPC communication between Phoenix and Bedrock

**Key Components:**
- **PalantirServer:** Qt-based server using `QLocalSocket` for Unix domain sockets
- **MessageEnvelope:** Protobuf-based envelope protocol (see [ADR-0002](../../phoenix/docs/adr/ADR-0002-Envelope-Based-Palantir-Framing.md))
- **RPC Handlers:** `handleCapabilitiesRequest()`, `handleXYSineRequest()`
- **Error Handling:** Standardized `ErrorResponse` with error codes

**Current Implementation (Sprint 4.5):**
- Transport: `QLocalSocket` (Unix domain sockets)
- Protocol: `MessageEnvelope` with `[4-byte length][serialized MessageEnvelope]`
- RPCs: Capabilities, XY Sine
- Threading: All operations on Qt event loop thread (see [THREADING.md](THREADING.md))

**Future Plans:**
- Arrow Flight transport (planned)
- gRPC transport (validated Gate 0.5, not yet integrated)
- Stats sidecar (planned)

> **Note:** For detailed transport documentation, see [docs/palantir/README.md](palantir/README.md) and [docs/palantir/IMPLEMENTATION_STATUS.md](palantir/IMPLEMENTATION_STATUS.md).

### 2. Compute Engine Layer

**Location:** `src/engine/`, `core/`

**Purpose:** Core computation primitives for optical design

**Key Components:**
- **Engine API:** Core computation interface (see [engine-api.md](engine-api.md))
- **XY Sine Computation:** Current implementation for sine wave analysis
- **Math Utilities:** Mathematical operations and data structures
- **Threading Infrastructure:** OpenMP/TBB support for parallel computation

**Current Implementation (Sprint 4.5):**
- XY Sine computation (synchronous, runs on event loop thread)
- OpenMP infrastructure present but not yet used in production paths
- TBB available for future use

**Future Plans:**
- Ray tracing engine
- Wave optics calculations
- Tolerancing analysis
- Optimization algorithms

### 3. SOM (System Object Model) Layer

**Location:** `som/`, `include/bedrock/som/`

**Purpose:** System Object Model for optical design data

**Key Components:**
- **SOM v0:** Initial version with Two-Surface Element (TSE)
- **Surface Definitions:** Surface geometry and properties
- **Material Properties:** Optical material definitions
- **System Model:** Complete optical system representation

**Current Implementation (Sprint 4.5):**
- SOM v0 with Two-Surface Element (TSE)
- Basic surface and material definitions
- System model structure

**Future Plans:**
- Extended SOM with assemblies
- Material library expansion
- Wavelength support
- Full optical system modeling

> **Note:** For detailed SOM documentation, see [SOM_OVERVIEW.md](SOM_OVERVIEW.md).

### 4. Geometry Layer

**Location:** `src/geom/`, `include/bedrock/geom/`

**Purpose:** Geometric modeling and CAD integration

**Key Components:**
- **OpenCascade Integration:** Planned integration with OCCT for geometry operations
- **STEP Export:** CAD file export functionality
- **Surface Geometry:** Geometric surface definitions

**Current Implementation (Sprint 4.5):**
- OpenCascade integration planned (not yet implemented)
- STEP export infrastructure exists but not fully integrated

**Future Plans:**
- Full OpenCascade integration
- STEP import/export
- Surface geometry operations
- CAD interoperability

> **Note:** For detailed OpenCascade documentation, see [OPENCASCADE_INTEGRATION.md](OPENCASCADE_INTEGRATION.md).

---

## Flow of a Remote Compute Call

### Example: XY Sine Request

```
1. Phoenix (Client)
   └─> LocalSocketChannel::sendXYSineRequest()
       └─> Builds XYSineRequest protobuf
       └─> Wraps in MessageEnvelope
       └─> Sends over QLocalSocket

2. Bedrock (Server)
   └─> PalantirServer::onClientReadyRead()
       └─> parseIncomingData()
           └─> extractMessage() (parses MessageEnvelope)
           └─> handleXYSineRequest()
               └─> Validates parameters (samples, frequency, amplitude, phase)
               └─> computeXYSine() (synchronous computation)
               └─> Builds XYSineResponse
               └─> sendMessage() (wraps in MessageEnvelope, sends response)

3. Phoenix (Client)
   └─> Receives MessageEnvelope
       └─> Parses XYSineResponse
       └─> Returns result to UI
```

**Key Points:**
- All communication uses `MessageEnvelope` protocol
- All operations run on Qt event loop thread (no worker threads currently)
- Input validation happens at RPC boundary
- Error responses use standardized error codes

---

## Threading Model

**Current State (Sprint 4.5):**
- **PalantirServer:** All operations on Qt event loop thread
- **Compute:** Synchronous (XY Sine runs on event loop thread)
- **OpenMP:** Infrastructure present but not used in production
- **Worker Threads:** Infrastructure exists but disabled

**Future Plans:**
- OpenMP parallelization for heavy compute
- Worker threads for long-running jobs
- TBB for task parallelism

> **Note:** For detailed threading documentation, see [THREADING.md](THREADING.md).

---

## Dependencies

### Required
- **C++20:** Core language standard
- **CMake 3.20+:** Build system
- **Protobuf 6.33+:** Palantir protocol serialization
- **OpenMP:** Multithreading support

### Optional (when `BEDROCK_WITH_TRANSPORT_DEPS=ON`)
- **Qt 6.10.1:** Palantir server (Core, Network)
- **TBB:** Threading Building Blocks (available but not required)

### Planned
- **OpenCascade (OCCT):** Geometry operations
- **Arrow:** High-performance data transfer (future transport)

> **Note:** For detailed toolchain versions, see [VERSIONS.md](VERSIONS.md).

---

## Related Documentation

- **[THREADING.md](THREADING.md)** - Threading and concurrency model
- **[SOM_OVERVIEW.md](SOM_OVERVIEW.md)** - System Object Model details
- **[engine-api.md](engine-api.md)** - Engine API documentation
- **[palantir/README.md](palantir/README.md)** - Palantir transport protocol
- **[VERSIONS.md](VERSIONS.md)** - Toolchain versions
- **[REPOSITORY_STRUCTURE.md](REPOSITORY_STRUCTURE.md)** - Directory layout

---

**Last Updated:** 2025-01-26 (Sprint 4.5)

