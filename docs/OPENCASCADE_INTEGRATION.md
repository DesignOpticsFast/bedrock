# OpenCascade Integration

**Last Updated:** 2025-01-26 (Sprint 4.5)  
**Purpose:** OpenCascade (OCCT) integration status and roadmap

---

## Overview

**OpenCascade Technology (OCCT)** is planned for integration into Bedrock to provide geometric modeling capabilities for optical surfaces and CAD operations.

**Status:** 🔮 **Planned** (not yet implemented)

---

## Purpose

### Planned Use Cases

1. **STEP Export:** Export optical systems to STEP CAD files
2. **Surface Geometry:** Geometric representation of optical surfaces
3. **CAD Interoperability:** Import/export with CAD tools
4. **Geometry Operations:** Surface intersection, transformation, validation

---

## Current Implementation (Sprint 4.5)

### Infrastructure

**Status:** ✅ **Build infrastructure exists**

**CMake Option:** `BEDROCK_WITH_OPENCASCADE` (default: `OFF` - optional until Sprint 4.6)

**Location:**
- **Headers:** `include/bedrock/geom/step_export.hpp`
- **Implementation:** `src/geom/step_export.cpp`
- **Library:** `bedrock_geom` (when OCCT enabled)

**Build Configuration:**
```cmake
option(BEDROCK_WITH_OPENCASCADE "Enable OpenCASCADE geometry backend" OFF)

if(BEDROCK_WITH_OPENCASCADE)
  find_package(OpenCASCADE REQUIRED)
  # ... build bedrock_geom library
endif()
```

### Current State

**What Exists:**
- CMake configuration for OCCT
- Header file structure (`step_export.hpp`)
- Build target (`bedrock_geom`)

**What's Missing:**
- Full OCCT integration implementation
- STEP export functionality
- Surface geometry operations
- CAD import/export

---

## Dependencies

### OpenCascade (OCCT)

**Version:** TBD (to be determined during integration)

**Installation:**

**macOS (Homebrew):**
```bash
brew install opencascade
# Sets OpenCASCADE_DIR automatically
```

**Linux:**
```bash
# Install from package manager or build from source
# Set OpenCASCADE_DIR to installation path
```

**CMake Configuration:**
```bash
cmake -S . -B build \
  -DBEDROCK_WITH_OCCT=ON \
  -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/opencascade:${CMAKE_PREFIX_PATH}
```

---

## Integration Roadmap

### Phase 1: Basic STEP Export (Planned)

**Goal:** Export simple optical systems to STEP files

**Components:**
- OCCT surface creation from SOM surfaces
- STEP file generation
- Basic geometry operations

**Status:** 🔮 Planned

### Phase 2: Surface Geometry (Planned)

**Goal:** Full geometric representation of optical surfaces

**Components:**
- Surface intersection
- Coordinate transformations
- Geometry validation

**Status:** 🔮 Planned

### Phase 3: CAD Import (Planned)

**Goal:** Import optical systems from CAD files

**Components:**
- STEP file parsing
- Surface extraction
- SOM conversion

**Status:** 🔮 Planned

---

## Geometry Handling Roadmap

### Current (Sprint 4.5)

**Status:** No geometry operations

**SOM Structures:**
- Surface definitions (radius, thickness, diameter)
- Material properties
- System model

**No Geometry:**
- No surface geometry representation
- No CAD operations
- No geometric validation

### Future

**Planned Geometry Operations:**
- Surface geometry from SOM definitions
- STEP export for CAD tools
- Surface intersection and validation
- Coordinate system transformations
- Assembly geometry

---

## Non-Goals (Sprint 4.5)

The following are explicitly **not** goals for Sprint 4.5:

- ❌ Full OCCT integration
- ❌ STEP export implementation
- ❌ Surface geometry operations
- ❌ CAD import/export

**Focus:** Sprint 4.5 is a hardening sprint focused on:
- Envelope-based protocol
- Error handling
- Input validation
- Testing infrastructure

OCCT integration will be addressed in future sprints.

---

## Notes for macOS Developers

OpenMP is required for Bedrock's compute kernel on macOS.

macOS cannot auto-detect OpenMP when using AppleClang. Bedrock includes:

- CMakePresets.json with explicit OpenMP hints (for Qt Creator and CLI)

- Fallback detection logic in CMakeLists.txt for Homebrew libomp

No manual configuration is required beyond:

    brew install libomp

---

## Related Documentation

- **[BEDROCK_ARCHITECTURE.md](BEDROCK_ARCHITECTURE.md)** - Overall architecture
- **[SOM_OVERVIEW.md](SOM_OVERVIEW.md)** - System Object Model (geometry source)
- **[BEDROCK_BUILD.md](BEDROCK_BUILD.md)** - Build instructions (OCCT configuration)

---

**Last Updated:** 2025-01-26 (Sprint 4.5)

