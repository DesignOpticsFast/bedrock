#pragma once
#include "bedrock/som/types.hpp"
#include <filesystem>
#include <string>

namespace bedrock::geom {

// True only if out_dir exists, is a directory, and we can create & remove a temp file.
[[nodiscard]] bool IsDirectoryWritable(const std::filesystem::path& out_dir) noexcept;

// Writes a STEP file representing a Two-Surface Element into `out_dir` and
// returns the absolute path to the written file. Throws on failure.
[[nodiscard]] std::string WriteTSEasSTEP(const bedrock::som::TwoSurfaceElement& tse,
                                         const std::string& out_dir);

} // namespace bedrock::geom
