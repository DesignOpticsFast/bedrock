#include <bedrock/engine.hpp>
#include <iostream>
#include <string>
#include <bedrock/geom/step_export.hpp>

// STUB for missing OCCT functionality to allow linking
namespace bedrock {
namespace geom {
    std::string WriteTSEasSTEP(const som::TwoSurfaceElement& tse, const std::string& filename) {
        std::cout << "[STUB] WriteTSEasSTEP called for " << filename << "\n";
        return filename;
    }
}
}

// Simple job: Instantiate Engine, create a design (which is dummy logic for now),
// and verify it runs without crashing.
int main() {
    std::cout << "Running Bedrock Engine Job...\n";
    
    try {
        bedrock::Engine engine([](uint64_t ver) {
            std::cout << "SOM changed! Version: " << ver << "\n";
        });
        
        std::cout << "Engine instantiated successfully.\n";
        
        // Now we CAN call this because we provided a stub for the missing symbol.
        engine.NewDesign_TSE_WriteSTEP("dummy_output.step");
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }

    std::cout << "Job Complete.\n";
    return 0;
}
