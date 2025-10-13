#include <gtest/gtest.h>
#include "bedrock/identity.h"

TEST(IdentityTest, ReturnsInput) {
  const double vals[] = {-1.0, -0.0, 0.0, 0.5, 1.2345, 42.0};
  for (double v : vals) {
    EXPECT_DOUBLE_EQ(bedrock::identity(v), v);
  }
}
