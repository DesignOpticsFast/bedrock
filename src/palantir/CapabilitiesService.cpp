#include "CapabilitiesService.hpp"

#ifdef BEDROCK_WITH_TRANSPORT_DEPS

namespace bedrock {
namespace palantir {

::palantir::CapabilitiesResponse CapabilitiesService::getCapabilities() const {
    ::palantir::CapabilitiesResponse response;
    auto* caps = response.mutable_capabilities();
    
    // Hard-coded capabilities (future: may be dynamic based on available features)
    // Future: May read from configuration or detect dynamically
    caps->set_server_version("bedrock-0.0.1");
    caps->add_supported_features("xy_sine");
    
    return response;
}

} // namespace palantir
} // namespace bedrock

#endif // BEDROCK_WITH_TRANSPORT_DEPS

