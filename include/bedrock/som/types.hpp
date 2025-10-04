#pragma once
#include <string>
#include <vector>

namespace bedrock::som {

enum class LengthUnits { mm, cm, inch, m };

struct Surface {
  std::string name;
  double radius{0.0};      // mm
  double thickness{0.0};   // mm to next surface
  double diameter{0.0};    // mm clear aperture
  bool   is_stop{false};
};

struct TwoSurfaceElement {
  std::string name{"TSE"};
  Surface s1{};
  Surface s2{};
  std::string material{"N-BK7"};
};

struct SystemModel {
  LengthUnits units{LengthUnits::mm};
  std::vector<TwoSurfaceElement> elements;
};

} // namespace bedrock::som
