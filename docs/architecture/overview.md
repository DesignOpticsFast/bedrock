# Bedrock Architecture Overview

## Purpose

Bedrock is a C++20 analysis engine that provides computational capabilities for Phoenix and other clients.

## High-Level Architecture

### Core Components

#### bedrock_engine Library
- Core analysis engine (static library)
- C++20 standard
- Minimal Phase 0 implementation

#### bedrock_sanity Executable
- Basic sanity test
- Validates engine initialization
- Part of CTest suite

#### Transport Support
- gRPC over Unix Domain Socket (UDS)
- LocalSocket fallback
- Palantir contracts for RPC definitions
- Conditional: `BEDROCK_WITH_TRANSPORT_DEPS` option

### Relationship to Phoenix

- Bedrock provides analysis capabilities
- Phoenix consumes Bedrock via transport layer
- Shared contracts ensure compatibility

### Relationship to Palantir Contracts

- `contracts/` submodule contains shared proto definitions
- Contract parity enforced in CI
- `.contract-version` tracks contract SHA

## Current Sprint 4.2 State

- ✅ C++20 reset complete
- ✅ Minimal engine library
- ✅ Sanity test passing
- ✅ Echo RPC implemented and tested
- ✅ Transport dependencies optional (BEDROCK_WITH_TRANSPORT_DEPS)
- ⏳ Full feature implementations planned (WP4+)

