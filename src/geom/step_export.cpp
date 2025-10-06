#include "bedrock/geom/step_export.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

// OpenCascade
#include <STEPControl_Writer.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <TopoDS_Shape.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <Interface_Static.hxx>

namespace bedrock::geom {

[[nodiscard]] bool IsDirectoryWritable(const std::filesystem::path& out_dir) noexcept {
    namespace fs = std::filesystem;
    try {
        if (!fs::exists(out_dir) || !fs::is_directory(out_dir)) return false;

        const fs::path probe = out_dir / fs::path(".bedrock_write_test.tmp");
        {
            std::ofstream ofs(probe, std::ios::out | std::ios::trunc);
            if (!ofs.is_open()) return false;
            ofs << "ok";
        }
        (void)fs::remove(probe);
        return true;
    } catch (...) {
        return false;
    }
}

std::string WriteTSEasSTEP(const bedrock::som::TwoSurfaceElement& /*tse*/,
                           const std::string& out_dir)
{
    namespace fs = std::filesystem;
    const fs::path dir(out_dir);

    if (!IsDirectoryWritable(dir)) {
        throw std::runtime_error("Bedrock STEP export failed: directory not writable — " + out_dir);
    }

    const fs::path out = dir / fs::path("tse.step");

    // Minimal OCCT export (placeholder solid). Replace with real TSE → shape mapping.
    const double r = 10.0;
    const double h = 10.0;
    TopoDS_Shape solid = BRepPrimAPI_MakeCylinder(r, h).Shape();

    STEPControl_Writer writer;
    Interface_Static::SetCVal("write.step.schema", "AP242DIS"); // prefer AP242

    writer.Transfer(solid, STEPControl_AsIs);
    if (writer.Write(out.string().c_str()) != IFSelect_RetDone) {
        throw std::runtime_error("STEP write failed");
    }

    return fs::weakly_canonical(out).string();
}

} // namespace bedrock::geom
