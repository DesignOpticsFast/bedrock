# Bedrock Project Overview

**Project Name:** Bedrock  
**Type:** C++ Library  
**Purpose:** Core optical calculations and ray tracing engine for design optics applications

---

## Mission

Bedrock provides the fundamental optical calculation capabilities required for lens design, surface analysis, and ray tracing. It serves as the computational foundation for the Phoenix desktop application and future optical design tools.

---

## Architecture

### System Design
Bedrock is organized as a modular C++ library with clean interfaces and minimal dependencies. It integrates with Open CASCADE Technology (OCCT) for geometric modeling and provides optical-specific calculations on top of that foundation.

### Key Modules

#### Core (`bedrock/core/`)
- Basic data structures
- Mathematical utilities
- Common algorithms

#### Optics (`bedrock/optics/`)
- Ray tracing engine
- Surface calculations
- Material properties
- Lens analysis

#### Geometry (`bedrock/geometry/`)
- OCCT integration layer
- Optical surface definitions
- Coordinate transformations

---

## Technology Stack

### Language & Standards
- **C++ 17** - Primary implementation language
- **CMake 3.22+** - Build system
- **Ninja** - Preferred build generator

### Key Dependencies
- **OpenCASCADE (OCCT)** - Geometric modeling kernel
- **GoogleTest** - Unit testing framework

### Development Tools
- **GCC 11+** or **Clang 14+** - Compilers
- **gdb** / **lldb** - Debugging
- **Cursor** - AI-assisted IDE
- **GitHub Actions** - CI/CD

---

## Project Structure

```
bedrock/
├── include/bedrock/    # Public headers
│   ├── core/
│   ├── optics/
│   └── geometry/
├── src/                # Implementation files
│   ├── core/
│   ├── optics/
│   └── geometry/
├── tests/              # Unit tests
├── docs/               # Documentation
├── .underlord/         # UnderLord development system
├── .github/            # CI workflows
└── CMakeLists.txt      # Build configuration
```

---

## Development Workflow

### Branch Strategy
- **main** - Stable, production-ready code
- **sprint-NNN-slug** - Feature branches following sprint pattern
- PR required for all changes to main
- Squash merge to keep history clean

### Build Process
```bash
# Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build -j

# Test
ctest --test-dir build --output-on-failure
```

### CI Pipeline
1. **validate** - Quick syntax and format checks
2. **Ubuntu build** - Full build with OCCT integration
3. **CodeQL Analysis** - Security scanning

All checks must pass before merge.

---

## Code Standards

### Naming Conventions
- **Namespace:** `bedrock::`
- **Classes:** `PascalCase`
- **Functions:** `snake_case`
- **Variables:** `snake_case`
- **Constants:** `UPPER_SNAKE_CASE`
- **Private members:** `trailing_underscore_`

### File Organization
- One class per file
- Header files in `include/bedrock/`
- Implementation in `src/`
- Tests in `tests/` with `test_` prefix

---

## Integration Points

### Used By
- **Phoenix** - Desktop optical design application
- Future tools and applications

### Depends On
- **OpenCASCADE** - Geometric modeling
- **Standard library** - C++ STL

### Design Philosophy
- Header-only where possible for templates
- Minimal dependencies
- Clean interfaces
- Performance-critical paths optimized

---

## Testing Strategy

### Unit Tests
- All public API functions have tests
- Edge cases and error conditions covered
- Fast execution (< 1 second per test)

### Integration Tests
- Test OCCT integration points
- Verify cross-module interactions

---

## Common Tasks

### Adding a New Feature
1. Start sprint: `.underlord/sprint-init.sh NNN feature-name`
2. Implement in appropriate module
3. Add unit tests
4. Update documentation
5. Create PR and pass CI
6. Merge and close sprint

---

**Last Updated:** 2025-10-14  
**Maintained By:** UnderLord development system
