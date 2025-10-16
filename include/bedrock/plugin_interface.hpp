#pragma once
#include <string>
#include <memory>

namespace bedrock {

// Clean plugin interface that Phoenix can use without OCCT dependencies
class IPluginInterface {
public:
    virtual ~IPluginInterface() = default;
    
    // Creates a default Two-Surface Element, writes a STEP to out_dir,
    // and returns the STEP path. May throw on I/O errors.
    virtual std::string NewDesign_TSE_WriteSTEP(const std::string& out_dir) = 0;
    
    // Get current SOM version
    virtual int getSomVersion() const = 0;
};

// Factory function to create plugin instances
std::unique_ptr<IPluginInterface> createPluginInterface();

} // namespace bedrock
