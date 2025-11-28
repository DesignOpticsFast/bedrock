#ifdef BEDROCK_WITH_TRANSPORT_DEPS

#include <gtest/gtest.h>
#include "palantir/error.pb.h"

using namespace palantir;

// Test ErrorResponse basic construction and serialization
TEST(ErrorResponseTest, BasicConstruction) {
    ErrorResponse error;
    error.set_error_code(ErrorCode::INTERNAL_ERROR);
    error.set_message("Test error message");
    error.set_details("Test details");
    
    EXPECT_EQ(error.error_code(), ErrorCode::INTERNAL_ERROR);
    EXPECT_EQ(error.message(), "Test error message");
    EXPECT_EQ(error.details(), "Test details");
}

// Test ErrorResponse serialization/deserialization round-trip
TEST(ErrorResponseTest, SerializationRoundTrip) {
    ErrorResponse original;
    original.set_error_code(ErrorCode::MESSAGE_TOO_LARGE);
    original.set_message("Message exceeds size limit");
    original.set_details("Size: 11MB, Limit: 10MB");
    
    // Serialize
    std::string serialized;
    ASSERT_TRUE(original.SerializeToString(&serialized));
    
    // Deserialize
    ErrorResponse decoded;
    ASSERT_TRUE(decoded.ParseFromString(serialized));
    
    // Verify round-trip
    EXPECT_EQ(decoded.error_code(), ErrorCode::MESSAGE_TOO_LARGE);
    EXPECT_EQ(decoded.message(), "Message exceeds size limit");
    EXPECT_EQ(decoded.details(), "Size: 11MB, Limit: 10MB");
}

// Test all error codes used in Sprint 4.5
TEST(ErrorResponseTest, AllSprint45ErrorCodes) {
    // Test each error code used in Sprint 4.5
    std::vector<ErrorCode> codes = {
        ErrorCode::INTERNAL_ERROR,
        ErrorCode::MESSAGE_TOO_LARGE,
        ErrorCode::INVALID_MESSAGE_FORMAT,
        ErrorCode::PROTOBUF_PARSE_ERROR,
        ErrorCode::UNKNOWN_MESSAGE_TYPE,
        ErrorCode::INVALID_PARAMETER_VALUE
    };
    
    for (auto code : codes) {
        ErrorResponse error;
        error.set_error_code(code);
        error.set_message("Test message for " + std::to_string(static_cast<int>(code)));
        
        // Serialize and deserialize
        std::string serialized;
        ASSERT_TRUE(error.SerializeToString(&serialized));
        
        ErrorResponse decoded;
        ASSERT_TRUE(decoded.ParseFromString(serialized));
        
        // Verify code round-trips
        EXPECT_EQ(decoded.error_code(), code) 
            << "Error code " << static_cast<int>(code) << " failed to round-trip";
    }
}

// Test ErrorResponse with message only (no details)
TEST(ErrorResponseTest, MessageOnly) {
    ErrorResponse error;
    error.set_error_code(ErrorCode::INVALID_PARAMETER_VALUE);
    error.set_message("Invalid parameter: samples must be between 2 and 10000000");
    // No details set
    
    std::string serialized;
    ASSERT_TRUE(error.SerializeToString(&serialized));
    
    ErrorResponse decoded;
    ASSERT_TRUE(decoded.ParseFromString(serialized));
    
    EXPECT_EQ(decoded.error_code(), ErrorCode::INVALID_PARAMETER_VALUE);
    EXPECT_EQ(decoded.message(), "Invalid parameter: samples must be between 2 and 10000000");
    EXPECT_TRUE(decoded.details().empty());
}

// Test ErrorResponse with details only (edge case - message should still be set)
TEST(ErrorResponseTest, WithDetails) {
    ErrorResponse error;
    error.set_error_code(ErrorCode::PROTOBUF_PARSE_ERROR);
    error.set_message("Failed to parse request");
    error.set_details("Field 'samples' is not a valid integer");
    
    std::string serialized;
    ASSERT_TRUE(error.SerializeToString(&serialized));
    
    ErrorResponse decoded;
    ASSERT_TRUE(decoded.ParseFromString(serialized));
    
    EXPECT_EQ(decoded.error_code(), ErrorCode::PROTOBUF_PARSE_ERROR);
    EXPECT_EQ(decoded.message(), "Failed to parse request");
    EXPECT_EQ(decoded.details(), "Field 'samples' is not a valid integer");
}

// Test ErrorResponse with empty message (edge case)
TEST(ErrorResponseTest, EmptyMessage) {
    ErrorResponse error;
    error.set_error_code(ErrorCode::INTERNAL_ERROR);
    error.set_message(""); // Empty message
    
    std::string serialized;
    ASSERT_TRUE(error.SerializeToString(&serialized));
    
    ErrorResponse decoded;
    ASSERT_TRUE(decoded.ParseFromString(serialized));
    
    EXPECT_EQ(decoded.error_code(), ErrorCode::INTERNAL_ERROR);
    EXPECT_TRUE(decoded.message().empty());
}

// Test ErrorResponse with long message and details
TEST(ErrorResponseTest, LongMessageAndDetails) {
    ErrorResponse error;
    error.set_error_code(ErrorCode::MESSAGE_TOO_LARGE);
    
    // Long message
    std::string longMessage(1000, 'A');
    error.set_message(longMessage);
    
    // Long details
    std::string longDetails(2000, 'B');
    error.set_details(longDetails);
    
    std::string serialized;
    ASSERT_TRUE(error.SerializeToString(&serialized));
    
    ErrorResponse decoded;
    ASSERT_TRUE(decoded.ParseFromString(serialized));
    
    EXPECT_EQ(decoded.error_code(), ErrorCode::MESSAGE_TOO_LARGE);
    EXPECT_EQ(decoded.message(), longMessage);
    EXPECT_EQ(decoded.details(), longDetails);
}

// Test ErrorResponse with special characters in message
TEST(ErrorResponseTest, SpecialCharactersInMessage) {
    ErrorResponse error;
    error.set_error_code(ErrorCode::INVALID_MESSAGE_FORMAT);
    error.set_message("Error: Invalid format\nLine 1\tTab\tLine 2");
    error.set_details("Details: \"quoted\" 'single' @special#chars$");
    
    std::string serialized;
    ASSERT_TRUE(error.SerializeToString(&serialized));
    
    ErrorResponse decoded;
    ASSERT_TRUE(decoded.ParseFromString(serialized));
    
    EXPECT_EQ(decoded.error_code(), ErrorCode::INVALID_MESSAGE_FORMAT);
    EXPECT_EQ(decoded.message(), "Error: Invalid format\nLine 1\tTab\tLine 2");
    EXPECT_EQ(decoded.details(), "Details: \"quoted\" 'single' @special#chars$");
}

// Test ErrorResponse with UNSPECIFIED code (should be valid for proto, but not used in production)
TEST(ErrorResponseTest, UnspecifiedCode) {
    ErrorResponse error;
    error.set_error_code(ErrorCode::ERROR_CODE_UNSPECIFIED);
    error.set_message("Unspecified error");
    
    std::string serialized;
    ASSERT_TRUE(error.SerializeToString(&serialized));
    
    ErrorResponse decoded;
    ASSERT_TRUE(decoded.ParseFromString(serialized));
    
    EXPECT_EQ(decoded.error_code(), ErrorCode::ERROR_CODE_UNSPECIFIED);
    EXPECT_EQ(decoded.message(), "Unspecified error");
}

// Test ErrorResponse with job_id field (for future use)
TEST(ErrorResponseTest, WithJobId) {
    ErrorResponse error;
    error.set_error_code(ErrorCode::JOB_NOT_FOUND);
    error.set_message("Job not found");
    error.set_job_id("job-12345");
    
    std::string serialized;
    ASSERT_TRUE(error.SerializeToString(&serialized));
    
    ErrorResponse decoded;
    ASSERT_TRUE(decoded.ParseFromString(serialized));
    
    EXPECT_EQ(decoded.error_code(), ErrorCode::JOB_NOT_FOUND);
    EXPECT_EQ(decoded.message(), "Job not found");
    EXPECT_EQ(decoded.job_id(), "job-12345");
}

#endif // BEDROCK_WITH_TRANSPORT_DEPS

