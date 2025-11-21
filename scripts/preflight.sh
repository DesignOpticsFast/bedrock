#!/bin/bash
set -euo pipefail

# Bedrock preflight script - checks environment for basic build requirements
# Exit codes: 0 = success, 1 = critical failure

errors=0
warnings=0

echo "=== Bedrock Preflight Check ==="
echo ""

# Check CMake (critical)
if command -v cmake >/dev/null 2>&1; then
    cmake_version=$(cmake --version | head -1)
    echo "✅ CMake: $cmake_version"
else
    echo "❌ CMake: NOT FOUND (required)"
    errors=$((errors + 1))
fi

# Check C++ compiler (critical)
if command -v g++ >/dev/null 2>&1; then
    gcc_version=$(g++ --version | head -1)
    echo "✅ C++ Compiler: $gcc_version"
elif command -v c++ >/dev/null 2>&1; then
    cxx_version=$(c++ --version | head -1)
    echo "✅ C++ Compiler: $cxx_version"
else
    echo "❌ C++ Compiler: NOT FOUND (required)"
    errors=$((errors + 1))
fi

# Check contracts submodule (critical)
if [ -d "contracts" ] && [ -f "contracts/.git" ]; then
    submodule_status=$(git submodule status contracts 2>/dev/null || echo "")
    if [ -n "$submodule_status" ]; then
        echo "✅ Contracts submodule: initialized"
        echo "   $submodule_status"
    else
        echo "⚠️  Contracts submodule: directory exists but not initialized"
        echo "   Run: git submodule update --init --recursive"
        errors=$((errors + 1))
    fi
else
    echo "❌ Contracts submodule: NOT FOUND (required)"
    echo "   Run: git submodule update --init --recursive"
    errors=$((errors + 1))
fi

# Check protoc (optional)
if command -v protoc >/dev/null 2>&1; then
    protoc_version=$(protoc --version 2>&1)
    echo "✅ protoc: $protoc_version"
else
    echo "⚠️  protoc: NOT FOUND (optional, needed for transport deps)"
    warnings=$((warnings + 1))
fi

# Check grpc_cpp_plugin (optional)
if command -v grpc_cpp_plugin >/dev/null 2>&1; then
    echo "✅ grpc_cpp_plugin: found"
else
    echo "⚠️  grpc_cpp_plugin: NOT FOUND (optional, needed for transport deps)"
    warnings=$((warnings + 1))
fi

echo ""
if [ $errors -eq 0 ]; then
    if [ $warnings -eq 0 ]; then
        echo "✅ Preflight OK - All checks passed"
    else
        echo "✅ Preflight OK - Critical checks passed ($warnings optional tool(s) missing)"
    fi
    exit 0
else
    echo "❌ Preflight FAILED - $errors critical check(s) failed"
    exit 1
fi

