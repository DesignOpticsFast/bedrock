#pragma once
#include "bedrock/som/types.hpp"
#include <string>

namespace bedrock::geom {
// Writes a simple solid STEP for the given Two-Surface Element.
// Returns absolute path to the written .step; throws on failure.
std::string WriteTSEasSTEP(const bedrock::som::TwoSurfaceElement& tse,
                           const std::string& out_dir);
}
