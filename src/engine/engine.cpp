#include "bedrock/engine.hpp"
#include "bedrock/geom/step_export.hpp"

using namespace bedrock;

Engine::Engine(SomChangedFn cb)
  : on_som_changed_(std::move(cb)) {}

std::string Engine::NewDesign_TSE_WriteSTEP(const std::string& out_dir) {
  som::TwoSurfaceElement tse;
  tse.s1 = {"S1", +50.0, 5.0, 25.0, false};
  tse.s2 = {"S2", -100.0, 0.0, 25.0, false};

  som_ = {};
  som_.elements = {tse};

  auto path = geom::WriteTSEasSTEP(tse, out_dir);
  ++som_version_;
  if (on_som_changed_) on_som_changed_(som_version_);
  return path;
}
