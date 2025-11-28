# C++23 Upgrade Plan - Bedrock

**Date:** 2025-01-26  
**Sprint:** Post-4.5, Pre-Thermal Diffusion  
**Status:** Planning Document  
**Current Standard:** C++20  
**Target Standard:** C++23

---

## Executive Summary

This document outlines the plan for upgrading Bedrock from C++20 to C++23. The upgrade is planned as a dedicated sprint that must occur after Sprint 4.5 Fixup and before Thermal Diffusion implementation.

**Key Considerations:**
- C++23 provides significant improvements for error handling, ranges, and compile-time programming
- Compiler support varies across platforms (Clang/GCC/MSVC)
- Migration requires careful testing and CI updates
- Some features may require refactoring existing code

**Estimated Duration:** 1-2 weeks (40-80 hours) depending on feature adoption scope

---

## 1. Current Compiler Versions

### Crucible (macOS - Primary Development Machine)

**Detected Versions:**
- **Clang:** Apple clang version 17.0.0 (clang-1700.4.4.1)
- **Target:** arm64-apple-darwin25.1.0
- **C++23 Support:** ✅ Good (Clang 17 supports most C++23 features)

**Notes:**
- Apple Clang 17.0.0 is based on LLVM 17.x
- C++23 feature support is comprehensive but not 100% complete
- Some library features may require libc++ updates

### dev-01 (Deprecated System)

**Status:** ❌ **Deprecated** - No longer in active rotation

**Compiler Versions:** Not relevant for future upgrade planning

**Action:** Ignore dev-01 in C++23 upgrade planning

### Windows (MSVC)

**Status:** ⚠️ **TBD** - Requires verification during C++23 upgrade sprint

**Expected:** MSVC 19.30+ (Visual Studio 2022 17.0+) for full C++23 support

**Action:** Verify MSVC version and C++23 feature support during upgrade sprint

### Linux CI (GitHub Actions)

**Status:** ⚠️ **TBD** - Requires verification during upgrade sprint

**Expected:** GCC 13+ or Clang 17+ for comprehensive C++23 support

**Action:** Update CI runners to ensure C++23-compatible compilers

---

## 2. Desired C++23 Features

### High Priority Features

#### 2.1 `std::expected<T, E>` (Error Handling)

**Purpose:** Type-safe error handling without exceptions

**Relevance to Bedrock:**
- **Palantir IPC:** Replace error response patterns with `std::expected`
- **Compute Engine:** Return computation results or errors without exceptions
- **API Design:** Cleaner error propagation in public APIs

**Example Use Case:**
```cpp
// Current (C++20):
bool computeXYSine(const XYSineRequest& req, XYSineResponse& out, std::string& error);

// C++23:
std::expected<XYSineResponse, ErrorCode> computeXYSine(const XYSineRequest& req);
```

**Benefits:**
- Eliminates out-parameters for errors
- Type-safe error handling
- Better composability
- No exception overhead

**Migration Impact:** Medium - Requires refactoring error handling patterns

---

#### 2.2 `std::ranges` Improvements

**Purpose:** Enhanced range algorithms and views

**Relevance to Bedrock:**
- **Ray Processing:** More efficient range-based algorithms
- **Data Transformations:** Cleaner pipeline-style code
- **Performance:** Better compiler optimizations

**Key Features:**
- `std::ranges::to` - Convert ranges to containers
- `std::ranges::chunk_by` - Group elements
- `std::ranges::chunk` - Split into chunks
- `std::ranges::slide` - Sliding window view
- `std::ranges::join_with` - Join ranges with separator
- `std::ranges::repeat` - Generate repeated values
- `std::ranges::zip` - Zip multiple ranges

**Example Use Case:**
```cpp
// Process rays in chunks
auto rayChunks = rays | std::ranges::views::chunk(1000);
for (auto chunk : rayChunks) {
    processChunk(chunk);
}
```

**Benefits:**
- More expressive range operations
- Better performance through views
- Cleaner code

**Migration Impact:** Low - Can be adopted incrementally

---

#### 2.3 `if consteval` / `consteval` Improvements

**Purpose:** Better compile-time programming

**Relevance to Bedrock:**
- **Math Utilities:** Compile-time constant evaluation
- **Type Traits:** More efficient metaprogramming
- **Validation:** Compile-time checks

**Key Features:**
- `if consteval` - Conditional compilation based on evaluation context
- Improved `consteval` function support
- Better constexpr/consteval interaction

**Example Use Case:**
```cpp
template<typename T>
constexpr T safe_divide(T a, T b) {
    if consteval {
        // Compile-time: static_assert
        static_assert(std::is_arithmetic_v<T>);
    } else {
        // Runtime: runtime check
        if (b == 0) throw std::runtime_error("Division by zero");
    }
    return a / b;
}
```

**Benefits:**
- Better compile-time/runtime code sharing
- More efficient metaprogramming
- Clearer intent

**Migration Impact:** Low - Can be adopted incrementally

---

### Medium Priority Features

#### 2.4 `std::print` and `std::println` (Formatting)

**Purpose:** Type-safe, efficient formatting

**Relevance to Bedrock:**
- **Logging:** Replace `std::cout` and `printf` patterns
- **Error Messages:** Better formatted error output
- **Debug Output:** Cleaner debug logging

**Example Use Case:**
```cpp
// Current:
std::cout << "Processing " << rayCount << " rays in " << threadCount << " threads\n";

// C++23:
std::println("Processing {} rays in {} threads", rayCount, threadCount);
```

**Benefits:**
- Type-safe formatting
- Better performance than iostreams
- Cleaner syntax

**Migration Impact:** Low - Can be adopted incrementally

**Note:** Requires `<print>` header and may need library updates

---

#### 2.5 `std::mdspan` (Multi-dimensional Array View)

**Purpose:** Multi-dimensional array access without ownership

**Relevance to Bedrock:**
- **Image Processing:** 2D/3D array views for wavefront maps, spot diagrams
- **Grid Computations:** Thermal diffusion grids, ray grids
- **Performance:** Zero-overhead abstraction

**Example Use Case:**
```cpp
// View 2D wavefront data
std::mdspan<double, std::extents<size_t, 512, 512>> wavefront(data.data());
for (size_t i = 0; i < wavefront.extent(0); ++i) {
    for (size_t j = 0; j < wavefront.extent(1); ++j) {
        process(wavefront[i, j]);
    }
}
```

**Benefits:**
- Zero-overhead multi-dimensional access
- Better abstraction than raw pointers
- Type-safe indexing

**Migration Impact:** Medium - Requires refactoring array access patterns

**Note:** May not be used initially but provides foundation for future features

---

#### 2.6 `std::optional` Improvements

**Purpose:** Enhanced optional type support

**Relevance to Bedrock:**
- **API Design:** Better optional parameter handling
- **Palantir:** Optional message fields

**Key Features:**
- `std::optional::transform` - Map over optional
- `std::optional::and_then` - Monadic bind
- `std::optional::or_else` - Alternative value

**Migration Impact:** Low - Can be adopted incrementally

---

### Lower Priority Features (Future Consideration)

#### 2.7 Modules (Experimental)

**Purpose:** Faster compilation, better encapsulation

**Relevance to Bedrock:**
- **Build Performance:** Faster compile times
- **Encapsulation:** Better module boundaries

**Status:** ⚠️ **Experimental** - Compiler support varies

**Migration Impact:** High - Requires significant refactoring

**Recommendation:** Defer to future sprint after C++23 adoption stabilizes

---

#### 2.8 `std::generator` (Coroutines)

**Purpose:** Lazy sequence generation

**Relevance to Bedrock:**
- **Ray Generation:** Lazy ray sequence generation
- **Data Streaming:** Efficient data pipelines

**Status:** ⚠️ **Library Feature** - May require library updates

**Migration Impact:** Medium - Requires coroutine infrastructure

**Recommendation:** Evaluate after core C++23 features are adopted

---

## 3. Risks & Migration Considerations

### 3.1 Compiler Support Differences

**Risk Level:** Medium

**Issues:**
- **Clang vs GCC vs MSVC:** Feature support varies
- **Library Features:** Some features require standard library updates
- **Platform-Specific:** macOS, Linux, Windows may have different support levels

**Mitigation:**
- Test on all target platforms early
- Use feature detection macros (`__cpp_*`)
- Provide fallback implementations for missing features
- Document platform-specific limitations

**Example:**
```cpp
#if __cpp_lib_expected >= 202202L
    // Use std::expected
#else
    // Fallback to custom expected-like type
#endif
```

---

### 3.2 Required CI Updates

**Risk Level:** Low-Medium

**Issues:**
- **CI Runners:** May need updated compiler versions
- **Build Matrix:** Need to test C++23 on all platforms
- **Dependency Updates:** May need updated build dependencies

**Actions Required:**
1. Update GitHub Actions runners to C++23-compatible compilers
2. Add C++23 build configuration to CI matrix
3. Update CMake minimum version if needed (likely 3.20+ is sufficient)
4. Test build on all platforms before migration

**Estimated Effort:** 4-8 hours

---

### 3.3 Interaction with Protobuf

**Risk Level:** Low

**Issues:**
- **Protobuf Compatibility:** Protobuf generated code must compile with C++23
- **API Compatibility:** Protobuf APIs should remain compatible

**Mitigation:**
- Test Protobuf compilation with C++23 early
- Verify Protobuf version compatibility
- Update Protobuf if needed (likely not required)

**Verification:**
- Build Palantir protobuf code with C++23
- Run integration tests
- Verify no ABI issues

---

### 3.4 Potential Need for Refactoring

**Risk Level:** Medium

**Issues:**
- **Error Handling:** Migration to `std::expected` requires refactoring
- **Range Code:** Adoption of new range features may require changes
- **API Changes:** Public APIs may need updates

**Mitigation:**
- Adopt features incrementally
- Start with new code, migrate existing code gradually
- Maintain backward compatibility where possible
- Comprehensive testing after each refactoring step

**Estimated Refactoring Effort:**
- Error handling migration: 20-40 hours
- Range improvements: 10-20 hours
- Other features: 10-20 hours
- **Total:** 40-80 hours depending on scope

---

### 3.5 OpenMP Compatibility

**Risk Level:** Low

**Issues:**
- **OpenMP + C++23:** Verify OpenMP works correctly with C++23
- **Compiler Flags:** May need updated OpenMP flags

**Mitigation:**
- Test OpenMP parallel regions with C++23
- Verify no performance regressions
- Update OpenMP version if needed

---

### 3.6 Qt Compatibility (for Palantir)

**Risk Level:** Low

**Issues:**
- **Qt 6.10.1 + C++23:** Verify Qt compiles and works with C++23
- **Signal/Slot:** Ensure Qt's meta-object system works

**Mitigation:**
- Test Qt compilation with C++23
- Run Palantir integration tests
- Verify no Qt-specific issues

---

## 4. Sprint Estimate & Sequencing

### 4.1 Prerequisites

**Must Complete Before C++23 Upgrade:**
1. ✅ **Sprint 4.5 Fixup** - Foundation hardening must be complete
2. ✅ **CI Stability** - CI must be stable and reliable
3. ✅ **Test Coverage** - Good test coverage for regression detection

**Dependencies:**
- Stable C++20 codebase
- Working CI infrastructure
- Comprehensive test suite

---

### 4.2 Sprint Duration Estimate

**Conservative Estimate:** 2 weeks (80 hours)
**Optimistic Estimate:** 1 week (40 hours)

**Breakdown:**
- **Planning & Setup:** 4-8 hours
  - Compiler verification
  - CI updates
  - Feature detection setup
- **Core Migration:** 20-40 hours
  - CMake updates
  - Compiler flag updates
  - Basic C++23 adoption
- **Feature Adoption:** 20-40 hours
  - `std::expected` migration
  - Range improvements
  - Other selected features
- **Testing & Validation:** 8-16 hours
  - Full test suite
  - Performance validation
  - Platform verification
- **Documentation:** 4-8 hours
  - Update coding standards
  - Document new patterns
  - Update build docs

**Total:** 56-112 hours (1.4-2.8 weeks at 40 hours/week)

---

### 4.3 Sequencing

**Must Occur Before:**
- **Thermal Diffusion Implementation** - C++23 upgrade should complete before Thermal Diffusion work begins

**Recommended Timeline:**
1. **Sprint 4.5 Fixup** (Current) - Foundation hardening
2. **C++23 Upgrade Sprint** (Next) - Language upgrade
3. **Thermal Diffusion** (After C++23) - Feature implementation on C++23 foundation

**Rationale:**
- C++23 features (especially `std::expected` and ranges) will benefit Thermal Diffusion implementation
- Clean foundation before major feature work
- Reduces technical debt

---

### 4.4 Incremental Adoption Strategy

**Phase 1: Foundation (Week 1)**
- Update CMake to C++23
- Verify compiler support
- Update CI
- Basic compilation with C++23

**Phase 2: Core Features (Week 1-2)**
- Adopt `std::expected` for new code
- Migrate error handling patterns incrementally
- Adopt range improvements where beneficial

**Phase 3: Advanced Features (Week 2+)**
- Evaluate `std::mdspan` for future use
- Adopt `std::print` for logging
- Consider other features based on value

**Phase 4: Documentation & Cleanup (Week 2)**
- Update coding standards
- Document new patterns
- Clean up any temporary workarounds

---

## 5. Success Criteria

### 5.1 Technical Criteria

- ✅ Bedrock compiles with C++23 on all target platforms
- ✅ All existing tests pass
- ✅ No performance regressions
- ✅ CI green on all platforms
- ✅ Protobuf integration works correctly
- ✅ OpenMP parallelization works correctly
- ✅ Qt integration (Palantir) works correctly

### 5.2 Adoption Criteria

- ✅ At least one high-priority feature (`std::expected` or ranges) adopted in production code
- ✅ Coding standards updated to reflect C++23 patterns
- ✅ Documentation updated
- ✅ Team trained on new features (if applicable)

---

## 6. Rollback Plan

**If Issues Arise:**
1. **Immediate Rollback:** Revert CMake to C++20
2. **Git-Based:** Use feature branch, merge only when stable
3. **Incremental:** Adopt features incrementally to minimize risk

**Rollback Triggers:**
- Critical bugs that can't be resolved quickly
- Performance regressions > 10%
- CI failures that block development
- Protobuf/Qt/OpenMP incompatibilities

---

## 7. References

### Compiler Support

- **Clang C++23 Status:** https://clang.llvm.org/cxx_status.html
- **GCC C++23 Status:** https://gcc.gnu.org/projects/cxx-status.html
- **MSVC C++23 Status:** https://learn.microsoft.com/en-us/cpp/overview/cpp23-features

### C++23 Standard

- **ISO C++23 Draft:** https://www.open-std.org/jtc1/sc22/wg21/docs/papers/
- **cppreference.com:** https://en.cppreference.com/w/cpp/23

### Bedrock-Specific

- **Current CMakeLists.txt:** `bedrock/CMakeLists.txt` (C++20)
- **Coding Standards:** `bedrock/docs/CPP_CODING_STANDARDS.md`
- **Build Docs:** `bedrock/docs/dev-setup.md`

---

## 8. Verification

### Document Verification

✅ **Compiler Versions Documented:**
- Crucible: Apple Clang 17.0.0 (detected)
- dev-01: Marked as deprecated
- Windows: Marked as TBD

✅ **Desired Features Listed:**
- High priority: `std::expected`, ranges improvements, `if consteval`
- Medium priority: `std::print`, `std::mdspan`, optional improvements
- Lower priority: Modules, generators (deferred)

✅ **Risks & Considerations Covered:**
- Compiler support differences
- CI updates
- Protobuf compatibility
- Refactoring needs
- OpenMP compatibility
- Qt compatibility

✅ **Sprint Estimate Provided:**
- Duration: 1-2 weeks (40-80 hours)
- Breakdown by phase
- Sequencing relative to Sprint 4.5 and Thermal Diffusion

✅ **Success Criteria Defined:**
- Technical criteria
- Adoption criteria

✅ **Rollback Plan Included:**
- Rollback triggers
- Rollback procedure

---

## Conclusion

This C++23 upgrade plan provides a roadmap for modernizing Bedrock's codebase. The upgrade should occur as a dedicated sprint after Sprint 4.5 Fixup and before Thermal Diffusion implementation.

**Key Recommendations:**
1. Start with compiler verification and CI updates
2. Adopt features incrementally (start with `std::expected`)
3. Maintain backward compatibility where possible
4. Comprehensive testing at each phase
5. Update documentation as features are adopted

**Next Steps:**
- Await Sprint 4.5 Fixup completion
- Schedule C++23 upgrade sprint
- Begin with compiler verification phase

---

**Document Created By:** UnderLord  
**Review Status:** Awaiting Mark/Lyra approval  
**Next Chunk:** 0.4 - ADR: Envelope-Based Palantir Framing

