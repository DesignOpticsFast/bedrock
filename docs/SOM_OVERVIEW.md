# SOM (System Object Model) Overview

**Last Updated:** 2025-01-26 (Sprint 4.5)  
**Purpose:** Overview of Bedrock's System Object Model (SOM)

---

## What is SOM?

**SOM (System Object Model)** is Bedrock's data model for representing optical systems. It provides a structured, versioned representation of optical designs that can be:

- **Serialized:** Stored as protobuf messages
- **Computed:** Used as input for ray tracing, analysis, and optimization
- **Exchanged:** Shared between Phoenix, Bedrock, and future tools
- **Versioned:** Tracked with version numbers for change management

---

## Role in Bedrock's Architecture

SOM serves as the **central data model** for all optical design operations:

```
┌─────────────────────────────────────────┐
│         Phoenix (GUI)                   │
│  - Creates/edits SOM                   │
│  - Sends SOM to Bedrock                 │
└─────────────────┬───────────────────────┘
                  │
                  │ SOM (protobuf)
                  │
┌─────────────────▼───────────────────────┐
│         Bedrock (Compute)               │
│  - Receives SOM                         │
│  - Performs calculations                │
│  - Returns results                      │
└─────────────────────────────────────────┘
```

**Key Roles:**
1. **Input:** Phoenix sends SOM to Bedrock for computation
2. **Computation:** Bedrock uses SOM to perform optical analysis
3. **Output:** Results reference SOM elements (surfaces, materials, etc.)
4. **Versioning:** SOM changes tracked for undo/redo and collaboration

---

## Current Implementation (Sprint 4.5)

### SOM v0

**Status:** ✅ **Implemented**

**Components:**
- **Two-Surface Element (TSE):** Basic optical element with two surfaces
- **Surface:** Individual optical surface with radius, thickness, diameter, stop flag
- **Material:** Optical material (currently string-based, e.g., "N-BK7")
- **System Model:** Container for multiple TSE elements

**Location:**
- **Headers:** `include/bedrock/som/types.hpp`
- **Implementation:** `som/` directory

**Data Structures:**
```cpp
namespace bedrock::som {

enum class LengthUnits { mm, cm, inch, m };

struct Surface {
  std::string name;
  double radius{0.0};      // mm
  double thickness{0.0};   // mm to next surface
  double diameter{0.0};    // mm clear aperture
  bool   is_stop{false};
};

struct TwoSurfaceElement {
  std::string name{"TSE"};
  Surface s1{};
  Surface s2{};
  std::string material{"N-BK7"};
};

struct SystemModel {
  LengthUnits units{LengthUnits::mm};
  std::vector<TwoSurfaceElement> elements;
};

} // namespace bedrock::som
```

**Current Usage:**
- Basic optical system representation
- Foundation for future extensions
- Used in STEP export (planned)

---

## Integration with Compute and Geometry

### Compute Integration

**Current (Sprint 4.5):**
- SOM structures defined and available
- Not yet used in compute paths (XY Sine is parameter-based)
- Foundation for future ray tracing and analysis

**Future:**
- Ray tracing will use SOM to define optical surfaces
- Analysis will operate on SOM structures
- Optimization will modify SOM and recompute

### Geometry Integration

**Current (Sprint 4.5):**
- SOM structures compatible with geometry operations
- STEP export will use SOM to generate CAD files
- OpenCascade integration (planned) will use SOM

**Future:**
- Full geometry operations on SOM surfaces
- CAD import/export based on SOM
- Surface geometry validation

---

## Future Backends

### Ray Tracing

**Planned:** Ray tracing engine will use SOM to:
- Define optical surfaces (from SOM surfaces)
- Trace rays through the system
- Compute spot diagrams, MTF, wavefront maps
- Return results referencing SOM elements

**Integration:**
- Ray tracer receives SOM as input
- Traces rays through SOM-defined surfaces
- Returns analysis results

### Thermal Diffusion

**Planned:** Thermal analysis will use SOM to:
- Define material properties (from SOM materials)
- Model thermal behavior
- Compute thermal effects on optical performance
- Return thermal analysis results

**Integration:**
- Thermal solver receives SOM with material properties
- Computes thermal distribution
- Returns thermal analysis results

### Optimization

**Planned:** Optimization will use SOM to:
- Define optimization variables (surface parameters)
- Evaluate objective functions (ray tracing results)
- Modify SOM parameters
- Track optimization history

**Integration:**
- Optimizer receives SOM as starting point
- Modifies SOM parameters during optimization
- Returns optimized SOM

---

## SOM Evolution

### Versioning Strategy

**Current:** SOM v0 (basic structures)

**Future Versions:**
- **SOM v1:** Extended with assemblies, materials library
- **SOM v2:** Wavelength support, coatings
- **SOM v3:** Full optical system modeling

**Migration:**
- Version numbers tracked in SOM structures
- Backward compatibility maintained where possible
- Migration tools for version upgrades

---

## Related Documentation

- **[BEDROCK_ARCHITECTURE.md](BEDROCK_ARCHITECTURE.md)** - Overall architecture
- **[engine-api.md](engine-api.md)** - Engine API (uses SOM)
- **[REPOSITORY_STRUCTURE.md](REPOSITORY_STRUCTURE.md)** - Directory layout

---

**Last Updated:** 2025-01-26 (Sprint 4.5)

