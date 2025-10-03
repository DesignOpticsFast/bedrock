#pragma once
#include "bedrock/som/types.hpp"
#include <functional>
#include <string>

namespace bedrock {

using SomChangedFn = std::function<void(int)>;

class Engine {
public:
  explicit Engine(SomChangedFn cb = nullptr);
  // Creates a default Two-Surface Element, writes a STEP to out_dir,
  // bumps SOM version, notifies via callback, and returns the STEP path.
  std::string NewDesign_TSE_WriteSTEP(const std::string& out_dir);

  const som::SystemModel& Som() const { return som_; }
  int SomVersion() const { return som_version_; }

private:
  som::SystemModel som_{};
  int som_version_{0};
  SomChangedFn on_som_changed_;
};

} // namespace bedrock
