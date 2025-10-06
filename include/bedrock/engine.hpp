#pragma once
#include "bedrock/som/types.hpp"
#include <functional>
#include <string>

namespace bedrock {

using SomChangedFn = std::function<void(int)>;

class Engine {
public:
    explicit Engine(SomChangedFn cb = nullptr);
    ~Engine() noexcept = default;  // ✅ destructor cannot throw

    // Creates a default Two-Surface Element, writes a STEP to out_dir,
    // bumps SOM version, notifies via callback, and returns the STEP path.
    // (May throw on I/O/OCCT errors; do NOT mark noexcept.)
    [[nodiscard]] std::string NewDesign_TSE_WriteSTEP(const std::string& out_dir);

    // Lightweight accessors: mark nodiscard + noexcept.
    [[nodiscard]] const som::SystemModel& Som() const noexcept { return som_; }
    [[nodiscard]] int SomVersion() const noexcept { return som_version_; }

private:
    som::SystemModel som_{};
    int som_version_{0};
    SomChangedFn on_som_changed_;
};

} // namespace bedrock
