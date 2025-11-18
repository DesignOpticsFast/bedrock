# Bedrock Transport Architecture

## Design

Bedrock provides analysis capabilities via RPC transport:
- Primary: gRPC over Unix Domain Socket (UDS)
- Fallback: LocalSocket (plain socket)
- Contracts: Palantir proto definitions

## Transport Modes

### gRPC UDS (Primary)
- High performance
- Type-safe via protobuf
- Unix Domain Socket for local communication

### LocalSocket (Fallback)
- Simpler, no gRPC dependency
- Plain socket communication
- Used when gRPC unavailable

## Current Implementation

### Echo RPC
- Basic echo service implemented
- `bedrock_echo_sanity` test validates functionality
- Proto generation working (`proto/bedrock_echo.proto`)

### Dependencies

Transport dependencies are optional:
- `BEDROCK_WITH_TRANSPORT_DEPS=ON` enables:
  - Protobuf
  - gRPC
  - libsodium (optional)

### Build Configuration

```cmake
option(BEDROCK_WITH_TRANSPORT_DEPS "Enable transport dependencies" OFF)
```

When enabled:
- Finds Protobuf (CONFIG or MODULE mode)
- Finds gRPC (CONFIG or MODULE mode)
- Finds libsodium (optional)
- Generates proto code
- Builds transport sanity targets

## Future Work

- Full feature RPCs (XY Sine, etc.)
- Streaming results
- Progress reporting
- Cancellation support

