# cmake/Options.cmake
# Build options and feature flags for Bedrock

# ---------------------------------------
# Options
# ---------------------------------------
option(BUILD_TESTING "Build tests" ON)
option(BEDROCK_WITH_TRANSPORT_DEPS "Enable gRPC/protobuf/libsodium for transport" OFF)

