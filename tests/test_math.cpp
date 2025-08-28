#include <gtest/gtest.h>
#include "bedrock/bedrock_math.hpp"

TEST(BedrockMath, Add) {
  EXPECT_EQ(bedrock::add(2, 3), 5);
  EXPECT_EQ(bedrock::add(-1, 1), 0);
  EXPECT_EQ(bedrock::add(0, 0), 0);
}
