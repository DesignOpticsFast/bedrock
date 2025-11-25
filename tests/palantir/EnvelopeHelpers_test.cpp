#ifdef BEDROCK_WITH_TRANSPORT_DEPS

#include <gtest/gtest.h>
#include "palantir/EnvelopeHelpers.hpp"
#include "palantir/capabilities.pb.h"
#include "palantir/xysine.pb.h"
#include "palantir/envelope.pb.h"

using namespace bedrock::palantir;

TEST(EnvelopeHelpersTest, MakeEnvelopeCapabilitiesRequest) {
    palantir::CapabilitiesRequest request;
    std::string error;
    
    auto envelope = makeEnvelope(palantir::MessageType::CAPABILITIES_REQUEST, request, {}, &error);
    
    ASSERT_TRUE(envelope.has_value());
    EXPECT_EQ(envelope->version(), 1u);
    EXPECT_EQ(envelope->type(), palantir::MessageType::CAPABILITIES_REQUEST);
    // Payload can be empty for empty messages (CapabilitiesRequest has no fields)
    EXPECT_TRUE(error.empty());
}

TEST(EnvelopeHelpersTest, MakeEnvelopeCapabilitiesResponse) {
    palantir::CapabilitiesResponse response;
    auto* caps = response.mutable_capabilities();
    caps->set_server_version("test-1.0");
    caps->add_supported_features("xy_sine");
    
    std::string error;
    auto envelope = makeEnvelope(palantir::MessageType::CAPABILITIES_RESPONSE, response, {}, &error);
    
    ASSERT_TRUE(envelope.has_value());
    EXPECT_EQ(envelope->version(), 1u);
    EXPECT_EQ(envelope->type(), palantir::MessageType::CAPABILITIES_RESPONSE);
    EXPECT_FALSE(envelope->payload().empty());
    EXPECT_TRUE(error.empty());
}

TEST(EnvelopeHelpersTest, MakeEnvelopeXYSineRequest) {
    palantir::XYSineRequest request;
    request.set_frequency(2.0);
    request.set_amplitude(1.5);
    request.set_phase(0.5);
    request.set_samples(100);
    
    std::string error;
    auto envelope = makeEnvelope(palantir::MessageType::XY_SINE_REQUEST, request, {}, &error);
    
    ASSERT_TRUE(envelope.has_value());
    EXPECT_EQ(envelope->version(), 1u);
    EXPECT_EQ(envelope->type(), palantir::MessageType::XY_SINE_REQUEST);
    EXPECT_FALSE(envelope->payload().empty());
    EXPECT_TRUE(error.empty());
}

TEST(EnvelopeHelpersTest, MakeEnvelopeXYSineResponse) {
    palantir::XYSineResponse response;
    response.add_x(0.0);
    response.add_x(1.0);
    response.add_y(0.0);
    response.add_y(1.0);
    response.set_status("OK");
    
    std::string error;
    auto envelope = makeEnvelope(palantir::MessageType::XY_SINE_RESPONSE, response, {}, &error);
    
    ASSERT_TRUE(envelope.has_value());
    EXPECT_EQ(envelope->version(), 1u);
    EXPECT_EQ(envelope->type(), palantir::MessageType::XY_SINE_RESPONSE);
    EXPECT_FALSE(envelope->payload().empty());
    EXPECT_TRUE(error.empty());
}

TEST(EnvelopeHelpersTest, MakeEnvelopeWithMetadata) {
    palantir::CapabilitiesRequest request;
    std::map<std::string, std::string> metadata;
    metadata["trace_id"] = "abc123";
    metadata["client_version"] = "phoenix-0.0.4";
    
    std::string error;
    auto envelope = makeEnvelope(palantir::MessageType::CAPABILITIES_REQUEST, request, metadata, &error);
    
    ASSERT_TRUE(envelope.has_value());
    EXPECT_EQ(envelope->metadata().size(), 2u);
    EXPECT_EQ(envelope->metadata().at("trace_id"), "abc123");
    EXPECT_EQ(envelope->metadata().at("client_version"), "phoenix-0.0.4");
    EXPECT_TRUE(error.empty());
}

TEST(EnvelopeHelpersTest, ParseEnvelopeValid) {
    // Create a valid envelope
    palantir::CapabilitiesRequest request;
    auto envelope = makeEnvelope(palantir::MessageType::CAPABILITIES_REQUEST, request);
    ASSERT_TRUE(envelope.has_value());
    
    // Serialize it
    std::string serialized;
    ASSERT_TRUE(envelope->SerializeToString(&serialized));
    
    // Parse it back
    palantir::MessageEnvelope parsed;
    std::string error;
    
    EXPECT_TRUE(parseEnvelope(serialized, parsed, &error));
    EXPECT_EQ(parsed.version(), 1u);
    EXPECT_EQ(parsed.type(), palantir::MessageType::CAPABILITIES_REQUEST);
    EXPECT_TRUE(error.empty());
}

TEST(EnvelopeHelpersTest, ParseEnvelopeInvalidVersion) {
    // Create envelope with wrong version
    palantir::MessageEnvelope envelope;
    envelope.set_version(999); // Invalid version
    envelope.set_type(palantir::MessageType::CAPABILITIES_REQUEST);
    envelope.set_payload("test");
    
    std::string serialized;
    ASSERT_TRUE(envelope.SerializeToString(&serialized));
    
    palantir::MessageEnvelope parsed;
    std::string error;
    
    EXPECT_FALSE(parseEnvelope(serialized, parsed, &error));
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("Invalid protocol version"), std::string::npos);
}

TEST(EnvelopeHelpersTest, ParseEnvelopeInvalidType) {
    // Create envelope with UNSPECIFIED type (invalid)
    palantir::MessageEnvelope envelope;
    envelope.set_version(1);
    envelope.set_type(palantir::MessageType::MESSAGE_TYPE_UNSPECIFIED);
    envelope.set_payload("test");
    
    std::string serialized;
    ASSERT_TRUE(envelope.SerializeToString(&serialized));
    
    palantir::MessageEnvelope parsed;
    std::string error;
    
    EXPECT_FALSE(parseEnvelope(serialized, parsed, &error));
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("UNSPECIFIED"), std::string::npos);
}

TEST(EnvelopeHelpersTest, ParseEnvelopeTruncated) {
    // Create valid envelope
    palantir::CapabilitiesRequest request;
    auto envelope = makeEnvelope(palantir::MessageType::CAPABILITIES_REQUEST, request);
    ASSERT_TRUE(envelope.has_value());
    
    std::string serialized;
    ASSERT_TRUE(envelope->SerializeToString(&serialized));
    
    // Truncate the buffer significantly - remove enough to break parsing
    // Protobuf is lenient, so we need to remove a substantial portion
    size_t truncatedSize = serialized.size() > 20 ? serialized.size() / 2 : 1;
    std::string truncated = serialized.substr(0, truncatedSize);
    palantir::MessageEnvelope parsed;
    std::string error;
    
    // parseEnvelope may succeed on partial data (protobuf is lenient), but the envelope will be malformed
    // Check that either parsing fails OR the parsed envelope fails validation
    bool parseResult = parseEnvelope(truncated, parsed, &error);
    if (parseResult) {
        // If parsing succeeded, verify the envelope fails validation (version check should catch it)
        // For a severely truncated envelope, version field might be missing or wrong
        EXPECT_TRUE(parsed.version() != 1 || parsed.type() == palantir::MessageType::MESSAGE_TYPE_UNSPECIFIED);
    } else {
        // Parsing failed as expected
        EXPECT_FALSE(error.empty());
    }
}

TEST(EnvelopeHelpersTest, ParseEnvelopeEmptyBuffer) {
    std::string buffer;
    palantir::MessageEnvelope parsed;
    std::string error;
    
    EXPECT_FALSE(parseEnvelope(buffer, parsed, &error));
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("Empty buffer"), std::string::npos);
}

TEST(EnvelopeHelpersTest, ParseEnvelopeUnspecifiedType) {
    // Test that UNSPECIFIED type is rejected
    palantir::MessageEnvelope envelope;
    envelope.set_version(1);
    envelope.set_type(palantir::MessageType::MESSAGE_TYPE_UNSPECIFIED);
    envelope.set_payload("");
    
    std::string serialized;
    ASSERT_TRUE(envelope.SerializeToString(&serialized));
    
    palantir::MessageEnvelope parsed;
    std::string error;
    
    EXPECT_FALSE(parseEnvelope(serialized, parsed, &error));
    EXPECT_NE(error.find("UNSPECIFIED"), std::string::npos);
}

TEST(EnvelopeHelpersTest, RoundTripCapabilitiesRequest) {
    palantir::CapabilitiesRequest original;
    
    // Encode
    auto envelope = makeEnvelope(palantir::MessageType::CAPABILITIES_REQUEST, original);
    ASSERT_TRUE(envelope.has_value());
    
    // Serialize envelope
    std::string serialized;
    ASSERT_TRUE(envelope->SerializeToString(&serialized));
    
    // Parse envelope
    palantir::MessageEnvelope parsed;
    ASSERT_TRUE(parseEnvelope(serialized, parsed));
    
    // Extract payload
    palantir::CapabilitiesRequest decoded;
    ASSERT_TRUE(decoded.ParseFromString(parsed.payload()));
    
    // Verify round-trip (CapabilitiesRequest is empty, so just verify it parses)
    EXPECT_TRUE(decoded.IsInitialized());
}

TEST(EnvelopeHelpersTest, RoundTripCapabilitiesResponse) {
    palantir::CapabilitiesResponse original;
    auto* caps = original.mutable_capabilities();
    caps->set_server_version("test-1.0");
    caps->add_supported_features("xy_sine");
    caps->add_supported_features("heat_diffusion");
    
    // Encode
    auto envelope = makeEnvelope(palantir::MessageType::CAPABILITIES_RESPONSE, original);
    ASSERT_TRUE(envelope.has_value());
    
    // Serialize and parse
    std::string serialized;
    ASSERT_TRUE(envelope->SerializeToString(&serialized));
    palantir::MessageEnvelope parsed;
    ASSERT_TRUE(parseEnvelope(serialized, parsed));
    
    // Extract and verify
    palantir::CapabilitiesResponse decoded;
    ASSERT_TRUE(decoded.ParseFromString(parsed.payload()));
    EXPECT_EQ(decoded.capabilities().server_version(), "test-1.0");
    EXPECT_EQ(decoded.capabilities().supported_features_size(), 2);
    EXPECT_EQ(decoded.capabilities().supported_features(0), "xy_sine");
    EXPECT_EQ(decoded.capabilities().supported_features(1), "heat_diffusion");
}

TEST(EnvelopeHelpersTest, RoundTripXYSineRequest) {
    palantir::XYSineRequest original;
    original.set_frequency(2.5);
    original.set_amplitude(1.8);
    original.set_phase(0.3);
    original.set_samples(500);
    
    // Encode
    auto envelope = makeEnvelope(palantir::MessageType::XY_SINE_REQUEST, original);
    ASSERT_TRUE(envelope.has_value());
    
    // Serialize and parse
    std::string serialized;
    ASSERT_TRUE(envelope->SerializeToString(&serialized));
    palantir::MessageEnvelope parsed;
    ASSERT_TRUE(parseEnvelope(serialized, parsed));
    
    // Extract and verify
    palantir::XYSineRequest decoded;
    ASSERT_TRUE(decoded.ParseFromString(parsed.payload()));
    EXPECT_DOUBLE_EQ(decoded.frequency(), 2.5);
    EXPECT_DOUBLE_EQ(decoded.amplitude(), 1.8);
    EXPECT_DOUBLE_EQ(decoded.phase(), 0.3);
    EXPECT_EQ(decoded.samples(), 500);
}

TEST(EnvelopeHelpersTest, RoundTripXYSineResponse) {
    palantir::XYSineResponse original;
    original.add_x(0.0);
    original.add_x(1.57);
    original.add_x(3.14);
    original.add_y(0.0);
    original.add_y(1.0);
    original.add_y(0.0);
    original.set_status("OK");
    
    // Encode
    auto envelope = makeEnvelope(palantir::MessageType::XY_SINE_RESPONSE, original);
    ASSERT_TRUE(envelope.has_value());
    
    // Serialize and parse
    std::string serialized;
    ASSERT_TRUE(envelope->SerializeToString(&serialized));
    palantir::MessageEnvelope parsed;
    ASSERT_TRUE(parseEnvelope(serialized, parsed));
    
    // Extract and verify
    palantir::XYSineResponse decoded;
    ASSERT_TRUE(decoded.ParseFromString(parsed.payload()));
    EXPECT_EQ(decoded.x_size(), 3);
    EXPECT_EQ(decoded.y_size(), 3);
    EXPECT_DOUBLE_EQ(decoded.x(0), 0.0);
    EXPECT_DOUBLE_EQ(decoded.x(1), 1.57);
    EXPECT_DOUBLE_EQ(decoded.y(0), 0.0);
    EXPECT_DOUBLE_EQ(decoded.y(1), 1.0);
    EXPECT_EQ(decoded.status(), "OK");
}

#endif // BEDROCK_WITH_TRANSPORT_DEPS

