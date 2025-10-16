#include "bedrock/plugin_interface.hpp"
#include "bedrock/engine.hpp"
#include <memory>

namespace bedrock {

// Plugin implementation that wraps Engine
class PluginImplementation : public IPluginInterface {
public:
    PluginImplementation() : engine_() {}
    
    std::string NewDesign_TSE_WriteSTEP(const std::string& out_dir) override {
        return engine_.NewDesign_TSE_WriteSTEP(out_dir);
    }
    
    int getSomVersion() const override {
        return engine_.SomVersion();
    }

private:
    Engine engine_;
};

// Factory function implementation
std::unique_ptr<IPluginInterface> createPluginInterface() {
    return std::make_unique<PluginImplementation>();
}

} // namespace bedrock
