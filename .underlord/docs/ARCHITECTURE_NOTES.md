# Bedrock Architecture Notes

**Last Updated:** 2025-10-14

---

## System Architecture

### High-Level Design

Bedrock is a **modular C++ library** with layered architecture:

```
┌─────────────────────────────────────────┐
│         Application Layer               │
│      (Phoenix, future tools)            │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│         Optics Module                    │
│   (Ray tracing, lens analysis)          │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│       Geometry Module                    │
│   (OCCT integration, surfaces)          │
└─────────────────┬───────────────────────┘
                  │
┌─────────────────▼───────────────────────┐
│         Core Module                      │
│   (Math, data structures, utilities)    │
└──────────────────────────────────────────┘
```

---

## Module Breakdown

### Core Module (`bedrock/core/`)

**Purpose:** Fundamental data structures and mathematical utilities

**Key Components:**
- Math utilities (vectors, matrices, numerical methods)
- Data structures (Point, Vector3D, Matrix)
- Algorithms (sorting, searching, optimization)

**Design Principles:**
- Header-only where possible
- No dependencies on other Bedrock modules
- Fast, inline operations

---

### Geometry Module (`bedrock/geometry/`)

**Purpose:** Geometric modeling and OCCT integration

**Key Components:**
- Surface definitions (planes, spheres, aspheres)
- OCCT wrappers (type-safe C++ interfaces)
- Coordinate transformations
- Intersection algorithms

**OCCT Integration Pattern:**
```cpp
namespace bedrock::geometry {
  class Surface {
  public:
    Point3D evaluate(double u, double v) const;
    Vector3D normal_at(double u, double v) const;
    
  private:
    Handle(Geom_Surface) occt_surface_;  // Hidden
  };
}
```

**Key Insight:** OCCT types never appear in public headers.

---

### Optics Module (`bedrock/optics/`)

**Purpose:** Optical calculations and analysis

**Key Components:**
- Ray tracing engine
- Material properties
- Lens analysis
- Surface interactions

**Ray Tracing Architecture:**
```cpp
namespace bedrock::optics {
  class RayTracer {
  public:
    struct Ray {
      Point3D origin;
      Vector3D direction;
      double wavelength;
    };
    
    std::vector<Ray> trace(
      const Ray& input,
      const std::vector<Surface>& surfaces
    );
  };
}
```

---

## Data Flow

### Typical Calculation Flow

```
User Input (Phoenix)
    ↓
Surface Definitions (Geometry)
    ↓
Material Properties (Optics)
    ↓
Ray Tracing (Optics)
    ↓
Results Analysis (Optics)
    ↓
Visualization Data (back to Phoenix)
```

---

## Key Design Patterns

### Pattern 1: PIMPL for OCCT Isolation

**Problem:** OCCT types in headers create compilation dependencies

**Solution:** Pointer-to-implementation

```cpp
// surface.h
class Surface {
public:
  Point3D evaluate(double u, double v) const;
private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};
```

---

### Pattern 2: Value Semantics for Math Types

```cpp
struct Vector3D {
  double x, y, z;  // 24 bytes, cache-friendly
  
  Vector3D operator+(Vector3D other) const {
    return {x + other.x, y + other.y, z + other.z};
  }
};
```

---

## Performance Considerations

### Critical Paths

1. **Ray-Surface Intersection** - Most frequent operation
2. **Surface Evaluation** - Called per intersection
3. **Material Queries** - Refractive index lookup

### Memory Layout

```cpp
struct RayIntersection {
  Point3D location;      // 24 bytes
  Vector3D normal;       // 24 bytes
  double distance;       // 8 bytes
  uint32_t surface_id;   // 4 bytes
  uint32_t flags;        // 4 bytes
  // Total: 64 bytes = 1 cache line
};
```

---

## Thread Safety

### Current State
**Not thread-safe** - Single-threaded use per calculation

### Future Considerations
- OCCT has limited thread safety
- Geometry module needs mutex protection
- Optics module parallelizable at ray level

---

## Error Handling

### Philosophy
- **Runtime errors:** Exceptions
- **Logic errors:** Assertions (debug only)
- **Input validation:** Return codes or std::optional

---

## Integration with Phoenix

Phoenix depends on Bedrock, not vice versa:

```
Phoenix (Qt App)
    ↓ uses
Bedrock (Library)
    ↓ uses
OCCT (External)
```

Phoenix never includes OCCT headers, only Bedrock headers.

---

**This document should be updated when:**
- New modules added
- Major architectural changes
- Performance characteristics change
- Integration patterns evolve

**Next Review:** After Sprint 10 or major refactoring
