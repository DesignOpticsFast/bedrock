#pragma once

// Include generated proto headers (only when BEDROCK_WITH_TRANSPORT_DEPS=ON)
#ifdef BEDROCK_WITH_TRANSPORT_DEPS
#include "palantir/capabilities.pb.h"
#endif

namespace bedrock {
namespace palantir {

// CapabilitiesService - In-process API for generating CapabilitiesResponse
// Capabilities service providing server capability information
// Future: Will be integrated into Palantir IPC server
class CapabilitiesService {
public:
#ifdef BEDROCK_WITH_TRANSPORT_DEPS
    // Get server capabilities
    // Returns a CapabilitiesResponse with server version and supported features
    ::palantir::CapabilitiesResponse getCapabilities() const;
#else
    // When transport deps are OFF, this class is a no-op
    // (proto types not available)
#endif
};

} // namespace palantir
} // namespace bedrock

