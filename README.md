# Bedrock — AI-Native Optical Design Core Toolkit

**Bedrock** is the headless, computationally efficient core of the Phoenix platform.  
It owns the **System Object Model (.SOM)**, performs all calculations, and provides
fast, composable primitives for ray tracing, wave optics, tolerancing, optimization,
and future AI-assisted workflows.

> Philosophy: **open, modular, AI-first**. GUIs belong in `Phoenix` (separate repo).

---

## 🚀 Current Scope (Sprint 4 - Phase 0.5 Gate PASS)

**Status:** ✅ **Phase 0.5 Gate PASSED** (2025-10-18)  
**Foundation:** Qt 6.10.0 + Qt Graphs / gRPC UDS transport

### Phase 0.5 Gate Results
- **Graphics:** Qt Graphs 6.10.0 - 50 windows < 5% CPU, < 100 MB RAM per window, 35 FPS for 10k points
- **Transport:** gRPC UDS - 2.04% overhead vs LocalSocket, 45 MB footprint
- **Status:** Foundation validated and ready for Phase 1/2 development

### Current Implementation
- **Palantir Server:** LocalSocket + Protobuf (needs gRPC migration)
- **Engine API:** Core computation primitives
- **SOM v0:** System Object Model with Two-Surface Element (TSE)
- **STEP Export:** OpenCascade integration for CAD export

---

## 📂 Repo Layout (early)
bedrock/
├── include/bedrock/          # Public C++ headers
│    ├── engine.hpp           # Engine API
│    ├── som/types.hpp        # SOM v0 types
│    └── geom/step_export.hpp # STEP export
├── src/
│    ├── engine/              # Engine implementation
│    └── geom/                # STEP export implementation
├── proto/                    # .SOM schema (protobuf)
├── tests/                    # Unit + CI smoke tests
└── docs/                     # ADRs, specs, design notes

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
# Test commit for dev-01 runner
# Another test commit
# Test sudo fix
# Final Sprint 3 validation
