
## Stage 0 Infrastructure Gate (Hard Requirement)

**Definition:** Infrastructure is NOT certified until `.underlord/preflight.sh` 
exits **0 in BOTH repositories** (phoenix, bedrock) on the target build host.

**Enforcement:**
- No sprint may be marked "complete" without a passing preflight in both repos.
- The full preflight stdout/stderr must be logged under `.underlord/logs/`.
- Each run must append a one-line summary to `METRICS.md`.
- Any preflight failure = infrastructure incomplete = sprint continues.

**What preflight validates:**
1. Git auth + remotes + recursive submodules
2. Toolchain: cmake, ninja, c/c++ compiler (versions recorded)
3. Third-party dependencies: OpenCASCADE >= 7.9.1, TBB >= 2021.5
4. CMake **configure** succeeds
5. Minimal **build** target compiles
6. (Optional) smoke tests run and pass

**Output artifacts:**
- `.underlord/logs/cmake-config-<timestamp>.log`
- `.underlord/logs/build-<timestamp>.log`
- `METRICS.md` line item with host, repo, durations, result

**Example METRICS entry:**
```
| 2025-10-15T22:01:29Z | ip-10-0-8-68 | bedrock | Stage0 | PASS | OCCT=7.9.1 TBB=2021.5 | Build certified |
```
