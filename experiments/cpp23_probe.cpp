#include <vector>
#include <string_view>
#include <iostream>

// Feature detection macros
#if __has_include(<print>)
#include <print>
#define HAS_PRINT 1
#endif

#if __has_include(<expected>)
#include <expected>
#define HAS_EXPECTED 1
#endif

#if __has_include(<ranges>)
#include <ranges>
#define HAS_RANGES 1
#endif

#if __has_include(<mdspan>)
#include <mdspan>
#define HAS_MDSPAN 1
#endif

int main() {
    std::cout << "C++23 Probe Target\n";
    std::cout << "=================\n";

    // Check for __cplusplus version
    std::cout << "__cplusplus: " << __cplusplus << "\n";

    // 1. Test std::expected (Critical)
#ifdef HAS_EXPECTED
    std::cout << "[PASS] <expected> found.\n";
    std::expected<int, std::string_view> e = 42;
    if (e) {
        std::cout << "  std::expected value: " << *e << "\n";
    }
#else
    std::cout << "[FAIL] <expected> NOT found.\n";
#endif

    // 2. Test std::ranges (Critical)
#ifdef HAS_RANGES
    std::cout << "[PASS] <ranges> found.\n";
    std::vector<int> nums = {1, 2, 3, 4, 5};
    auto view = nums | std::views::transform([](int i) { return i * 2; });
    std::cout << "  Ranges view transform: ";
    for (int i : view) std::cout << i << " ";
    std::cout << "\n";
#else
    std::cout << "[FAIL] <ranges> NOT found.\n";
#endif

    // 3. Test if consteval (Critical)
    std::cout << "Testing if consteval...\n";
    if consteval {
        // This block should be discarded at runtime, so we can't print from here efficiently without std::print
        // but compilation of this block confirms support.
    } else {
        std::cout << "[PASS] 'if consteval' compiled and runtime branch taken.\n";
    }

    // 4. Test std::print (Non-Critical)
#ifdef HAS_PRINT
    std::cout << "[PASS] <print> found.\n";
    std::print("  std::print works: Hello from {}!\n", "C++23");
#else
    std::cout << "[WARN] <print> NOT found.\n";
#endif

    // 5. Test std::mdspan (Non-Critical)
#ifdef HAS_MDSPAN
    std::cout << "[PASS] <mdspan> found.\n";
    // Minimal mdspan usage
    int data[] = {1, 2, 3, 4};
    std::mdspan<int, std::extents<size_t, 2, 2>> ms(data);
    std::cout << "  mdspan element [1,1]: " << ms[1, 1] << "\n";
#else
    std::cout << "[WARN] <mdspan> NOT found.\n";
#endif

    return 0;
}

