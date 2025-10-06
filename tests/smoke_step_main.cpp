#include "bedrock/engine.hpp"
#include <filesystem>
#include <iostream>

int main() {
  bedrock::Engine eng;
  auto out = std::filesystem::temp_directory_path().string();
  auto p = eng.NewDesign_TSE_WriteSTEP(out);
  bool ok = std::filesystem::exists(p) && std::filesystem::file_size(p) > 0;
  std::cout << (ok ? "OK  " : "FAIL ") << p << "\n";
  return ok ? 0 : 1;
}
