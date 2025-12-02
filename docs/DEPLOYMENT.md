# Bedrock Deployment Guide

**Last Updated:** 2025-01-26 (Sprint 4.5)  
**Purpose:** Deployment overview for Bedrock in Phoenix workflows

---

## Overview

Bedrock is deployed as a **headless server** that Phoenix connects to via Palantir IPC (Unix domain sockets). Bedrock never runs a GUI; all user interaction happens in Phoenix.

---

## Deployment Model

### Current Implementation (Sprint 4.5)

**Architecture:**
```
Phoenix (GUI)  ←→  Bedrock (Server)
   (Qt)              (Headless)
```

**Communication:**
- **Transport:** Unix domain sockets (`QLocalSocket`)
- **Protocol:** Palantir `MessageEnvelope` protocol
- **Socket Name:** `palantir_bedrock` (default, configurable via `PALANTIR_SOCKET`)

**Deployment:**
- Bedrock runs as a separate process
- Phoenix spawns/manages Bedrock server
- Bedrock listens on Unix domain socket
- Phoenix connects to Bedrock for RPC calls

---

## Binary Locations

### Development

**Build Output:**
- **Library:** `build/lib/libbedrock_palantir_server.a` (static library)
- **Executable:** `build/bin/bedrock_server` (when transport enabled)

**Installation:**
- Not yet installed system-wide
- Phoenix references Bedrock build directory or bundled binary

### Production (Planned)

**Future Locations:**
- **macOS:** `/Applications/Phoenix.app/Contents/Frameworks/bedrock_server`
- **Linux:** `/usr/local/bin/bedrock_server` or bundled with Phoenix
- **Windows:** `C:\Program Files\Phoenix\bedrock_server.exe` (TBD)

---

## Phoenix Integration

### Current State

**Phoenix Responsibilities:**
- Spawns Bedrock server process
- Manages Bedrock lifecycle (start/stop)
- Connects to Bedrock via Palantir IPC
- Handles Bedrock crashes/restarts

**Bedrock Responsibilities:**
- Listens on Unix domain socket
- Processes RPC requests
- Returns computation results
- Manages compute resources

### Connection Flow

```
1. Phoenix starts Bedrock server
   └─> Spawns bedrock_server process
   └─> Bedrock listens on socket

2. Phoenix connects to Bedrock
   └─> LocalSocketChannel connects to socket
   └─> Sends Capabilities request
   └─> Receives capabilities response

3. Phoenix uses Bedrock
   └─> Sends RPC requests (XY Sine, etc.)
   └─> Receives computation results
   └─> Displays results in UI

4. Phoenix shuts down
   └─> Disconnects from Bedrock
   └─> Terminates Bedrock process
```

---

## Packaging

### Current State (Sprint 4.5)

**Status:** Not yet packaged for distribution

**Development:**
- Bedrock built from source
- Phoenix references local Bedrock build
- No installer or package manager integration

### Future Plans

**Packaging Options:**
- **Bundled with Phoenix:** Bedrock binary included in Phoenix app bundle
- **Separate Package:** Bedrock as standalone package (Homebrew, apt, etc.)
- **Shared Library:** Bedrock as shared library (`.so`, `.dylib`, `.dll`)

**Distribution:**
- **macOS:** App bundle with embedded Bedrock
- **Linux:** DEB/RPM package or AppImage
- **Windows:** MSI installer (TBD)

---

## Configuration

### Environment Variables

**`PALANTIR_SOCKET`:** Socket name for Palantir IPC
- **Default:** `"palantir_bedrock"`
- **Usage:** `PALANTIR_SOCKET=/tmp/custom_socket ./phoenix_app`

**`BEDROCK_SERVER_PATH`:** Path to Bedrock server executable (future)
- **Default:** Auto-detected or bundled path
- **Usage:** `BEDROCK_SERVER_PATH=/usr/local/bin/bedrock_server ./phoenix_app`

### Socket Location

**Unix Domain Sockets:**
- **macOS/Linux:** `/tmp/palantir_bedrock` (or custom path)
- **Windows:** Named pipe (TBD)

**Permissions:**
- Socket file created by Bedrock server
- Phoenix must have read/write access
- Socket cleaned up on server shutdown

---

## Deployment Automation

### Current State (Sprint 4.5)

**Status:** Manual deployment

**Process:**
1. Build Bedrock from source
2. Build Phoenix from source
3. Configure Phoenix to reference Bedrock
4. Run Phoenix (spawns Bedrock)

### Future Plans

**CI/CD Integration:**
- Automated builds in GitHub Actions
- Artifact generation (binaries, libraries)
- Automated testing before deployment
- Release packaging

**Deployment Scripts:**
- Build scripts for all platforms
- Installer generation
- Package manager integration
- Update mechanisms

---

## Monitoring and Observability

### Current State (Sprint 4.5)

**Status:** Basic error handling

**Features:**
- Error responses via Palantir protocol
- Connection state management in Phoenix
- Basic logging (qDebug)

### Future Plans

**Stats Sidecar (Planned):**
- Performance metrics
- Request tracing
- Error tracking
- Resource usage monitoring

**Logging:**
- Structured logging
- Log aggregation
- Debug vs production logging levels

---

## Related Documentation

- **[BEDROCK_ARCHITECTURE.md](BEDROCK_ARCHITECTURE.md)** - Architecture overview
- **[palantir/README.md](palantir/README.md)** - Palantir transport protocol
- **[THREADING.md](THREADING.md)** - Threading model

---

**Last Updated:** 2025-01-26 (Sprint 4.5)

