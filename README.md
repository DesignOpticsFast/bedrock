# Bedrock — AI-Native Optical Design Core Toolkit

[![CI](https://github.com/DesignOpticsFast/bedrock/actions/workflows/ci.yml/badge.svg)](https://github.com/DesignOpticsFast/bedrock/actions/workflows/ci.yml)
[![CodeQL](https://github.com/DesignOpticsFast/bedrock/actions/workflows/codeql.yml/badge.svg)](https://github.com/DesignOpticsFast/bedrock/actions/workflows/codeql.yml)
[![PR Guard](https://github.com/DesignOpticsFast/bedrock/actions/workflows/pr-guard.yml/badge.svg)](https://github.com/DesignOpticsFast/bedrock/actions/workflows/pr-guard.yml)
[![Auto-merge Dependabot](https://github.com/DesignOpticsFast/bedrock/actions/workflows/auto-merge-dependabot.yml/badge.svg)](https://github.com/DesignOpticsFast/bedrock/actions/workflows/auto-merge-dependabot.yml)

📋 **[Workflow Dependencies](docs/workflow-dependencies.md)** | 🔧 **[Development Setup](docs/dev-setup.md)** | 📚 **[API Documentation](docs/engine-api.md)** | 🏗️ **[Architecture](docs/BEDROCK_ARCHITECTURE.md)** | 🧵 **[Threading](docs/THREADING.md)**

**Bedrock** is the headless, computationally efficient core of the Phoenix platform.  
It owns the **System Object Model (.SOM)**, performs all calculations, and provides
fast, composable primitives for ray tracing, wave optics, tolerancing, optimization,
and future AI-assisted workflows.

> Philosophy: **open, modular, AI-first**. GUIs belong in `Phoenix` (separate repo).

---

## 🚀 Current Scope (Sprint 4.5)

**Status:** ✅ **Sprint 4.5 Complete** (2025-01-26)  
**Foundation:** C++20, LocalSocket + Protobuf (envelope-based Palantir protocol)

> **Note:** For current toolchain versions, see [docs/VERSIONS.md](docs/VERSIONS.md).

### Current Implementation (Sprint 4.5)

**Transport:**
- **Palantir Server:** LocalSocket + Protobuf with envelope-based protocol
- **Protocol:** `MessageEnvelope` with `[4-byte length][serialized MessageEnvelope]`
- **RPCs:** Capabilities, XY Sine
- **Error Handling:** Standardized error codes and responses

**Compute:**
- **Engine API:** Core computation primitives
- **XY Sine:** Synchronous computation (runs on event loop thread)
- **OpenMP:** Infrastructure present for future parallelization

**Data Model:**
- **SOM v0:** System Object Model with Two-Surface Element (TSE)
- **Surface Definitions:** Basic optical surface structures
- **Material Properties:** Material definitions

**Geometry:**
- **OpenCascade:** Build infrastructure exists (integration planned)
- **STEP Export:** Planned for future sprints

### Planned Enhancements

**Transport:**
- **gRPC:** Validated in Gate 0.5, not yet integrated (planned)
- **Arrow Flight:** High-performance data transfer (planned)

**Compute:**
- **Ray Tracing:** Optical ray tracing engine (planned)
- **Wave Optics:** Wavefront calculations (planned)
- **Tolerancing:** Statistical analysis (planned)

**Geometry:**
- **OpenCascade Integration:** Full OCCT integration (planned)
- **STEP Export:** CAD file export (planned)

> **Note:** For detailed implementation status, see [docs/palantir/IMPLEMENTATION_STATUS.md](docs/palantir/IMPLEMENTATION_STATUS.md).

---

## 📂 Repository Structure

**Key Directories:**
- **`include/bedrock/`** - Public C++ headers (Engine API, SOM, geometry)
- **`src/`** - Implementation (engine, palantir transport, geometry)
- **`core/`** - Core utilities (threading, math)
- **`som/`** - System Object Model definitions
- **`tests/`** - Unit and integration tests
- **`docs/`** - Documentation (architecture, build, testing, etc.)

> **Note:** For detailed repository structure, see [docs/REPOSITORY_STRUCTURE.md](docs/REPOSITORY_STRUCTURE.md).

---

## 🌐 Relationship to Phoenix & Rosetta

- **Phoenix** — Qt GUI/IDE that calls Bedrock APIs. Bedrock never runs a GUI.  
- **Rosetta** — translators for legacy formats (Zemax/CodeV/Seq) will arrive later, once `.SOM` is stable.  
- **Gaia** — community hub (docs, tutorials, RFCs for `.SOM`, contribution guides).  

Bedrock is the **foundation layer**: Phoenix and Rosetta depend on it.

---

## 📈 Roadmap

1. **Phase 1 (Now)**: SOM v0, TSE, STEP export, Phoenix integration.  
2. **Phase 2**: Extend SOM (assemblies, materials, wavelengths).  
3. **Phase 3**: Core primitives — ray tracing, aberrations, tolerancing.  
4. **Phase 4**: Python bindings + CLI tools.  
5. **Phase 5**: GPU/cluster scalability and AI-agent workflows.

---

## 🧪 Status

- CI + branch protection enabled (PRs required, build must pass).  
- `.SOM` schema under active design.  
- First deliverable: `NewDesign_TSE_WriteSTEP()` + smoke tests.  

---

## 🤝 Contributing

We welcome early feedback and proposals:

- Open issues for `.SOM`, Rosetta, and primitive APIs.  
- RFCs and design docs will live in [`DesignOpticsFast/gaia`](https://github.com/DesignOpticsFast/gaia).  
- License: **Apache 2.0**  

---

## 📜 License

Apache License 2.0 © Design Optics Fast LLC
# Trigger validation with infra-ci label
# CI runs on GitHub Actions with integration tests
# Another test commit
# Test sudo fix
# Final Sprint 3 validation
# Smoke test for PR Guard workflow
