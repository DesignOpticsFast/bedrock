# Bedrock — AI-Native Optical Design Core Toolkit

**Bedrock** is a headless, computationally efficient toolkit for optical design.
It exposes fast, composable primitives (ray tracing, wave optics, tolerancing, optimization)
for scripting, batch workflows, and AI agents.

- **.SOM** — *System Object Model*: the canonical data contract for optical systems
- **Rosetta** — translators for legacy formats (Zemax/CodeV/etc.)
- **APIs** — CLI + Python bindings (C++ core; Python interface)

> Philosophy: **open, modular, AI-first**. GUIs belong in `Phoenix` (a separate project).

---

## Why Bedrock?

Legacy optical tools were designed around GUIs and closed formats. Bedrock flips the model:
- **AI-native**: algorithms callable directly by agents, CI, and pipelines
- **Scriptable**: first-class CLI/Python interfaces for automation
- **Interoperable**: `.SOM` as the shared schema; Rosetta for legacy translation
- **Scalable**: designed for parallel execution (GPU/cluster-friendly)

---

## Project Layout (planned)
bedrock/
core/                 # C++ kernels (ray, wave, tolerancing, optimization)
api/python/           # Python bindings and high-level helpers
cli/                  # Command-line tools exposing core primitives
som/                  # .SOM schema, validators, fixtures
rosetta/              # Import/export: Zemax, CodeV, etc.
tests/                # Unit and baseline tests
docs/                 # Specs, guides, design notes

---

## Relationship to Phoenix & Gaia

- **Phoenix** — a Qt-based GUI/IDE that *uses Bedrock* (separate repo; may be closed-source / paid)  
- **Gaia** — community hub (docs, tutorials, RFCs for `.SOM`, contribution guides)  

Bedrock aims to be the *foundation layer*; GUIs and workflows can evolve independently on top.

---

## Status

Early days. We’re laying the **.SOM** schema and API contracts first, then bringing up core primitives.

_This is a PR sanity test. CI + branch protection should block direct pushes to main._

---

## Contributing

We welcome early discussion on `.SOM`, Rosetta, and primitive APIs:

- Open issues and proposals here  
- Docs & RFCs will live in [`DesignOpticsFast/gaia`](https://github.com/DesignOpticsFast/gaia)  
- License: **Apache 2.0**

---

## License

Apache License 2.0 © Design Optics Fast LLC
