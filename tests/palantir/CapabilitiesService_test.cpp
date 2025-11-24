#ifdef BEDROCK_WITH_TRANSPORT_DEPS

#include <gtest/gtest.h>
#include "palantir/capabilities.pb.h"
#include "bedrock/palantir/CapabilitiesService.hpp"

using namespace bedrock::palantir;

TEST(CapabilitiesServiceTest, GetCapabilities) {
    CapabilitiesService service;
    auto response = service.getCapabilities();
    
    // Verify response structure
    ASSERT_TRUE(response.has_capabilities());
    
    const auto& caps = response.capabilities();
    
    // Verify server version
    EXPECT_EQ(caps.server_version(), "bedrock-0.0.1");
    
    // Verify supported features
    EXPECT_EQ(caps.supported_features_size(), 1);
    EXPECT_EQ(caps.supported_features(0), "xy_sine");
}

#endif // BEDROCK_WITH_TRANSPORT_DEPS

