# ADR 0002: Bedrock owns Execution Copies

**Status:** Accepted  
**Date:** 2025-10-03

## Decision
- Execution Copies (EC) are created from the canonical SOM snapshot and reside only in Bedrock.
- API supports: `createExec`, `applyEdits`, `runAnalysis`, `commit`, `discard`.
- `commit` promotes EC → canonical SOM and emits `SomChanged(version)` to Phoenix; `discard` drops it.
- Cancellation tokens are checked between batches to keep the UI responsive.

## Rationale
Centralizing copy semantics and computation in Bedrock simplifies the UI and enforces consistent lifecycle and licensing.

## Consequences
- Bedrock must provide robust job and EC registries, progress, and error signaling.
