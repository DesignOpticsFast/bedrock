#include "bedrock/geom/step_export.hpp"

#include <filesystem>
#include <stdexcept>

// OpenCascade
#include <STEPControl_Writer.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <TopoDS_Shape.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <Interface_Static.hxx>

using namespace bedrock;

std::string geom::WriteTSEasSTEP(const som::TwoSurfaceElement& tse,
                                 const std::string& out_dir) {
  namespace fs = std::filesystem;
  fs::create_directories(out_dir);
  const auto out = fs::path(out_dir) / "tse.step";

  // MVP placeholder: simple cylinder (swap for revolved lens later).
  const Standard_Real r = (tse.s1.diameter > 0 ? tse.s1.diameter * 0.5 : 10.0);
  const Standard_Real h = (tse.s1.thickness + tse.s2.thickness > 0
                           ? tse.s1.thickness + tse.s2.thickness : 5.0);

  TopoDS_Shape solid = BRepPrimAPI_MakeCylinder(r, h).Shape();

  STEPControl_Writer writer;

  // Force AP242 (discrete) schema — OCCT recognizes this token
  Interface_Static::SetCVal("write.step.schema", "AP242DIS");

  writer.Transfer(solid, STEPControl_AsIs);
  if (writer.Write(out.string().c_str()) != IFSelect_RetDone) {
    throw std::runtime_error("STEP write failed");
  }
  return fs::weakly_canonical(out).string();
}
