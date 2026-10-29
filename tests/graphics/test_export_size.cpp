#include <gtest/gtest.h>

#include <limits>
#include <optional>

#include "graphics/export_size.hpp"

TEST(ExportSizeTest, A4LandscapeAt300DpiRoundsEachSide) {
  const std::optional<ImageSize> size = ExportPixelSize(297.0F, 210.0F, 300.0F);

  ASSERT_TRUE(size.has_value());
  EXPECT_EQ(size->width, 3508U);
  EXPECT_EQ(size->height, 2480U);
}

TEST(ExportSizeTest, A4LandscapeAt200Dpi) {
  const std::optional<ImageSize> size = ExportPixelSize(297.0F, 210.0F, 200.0F);

  ASSERT_TRUE(size.has_value());
  EXPECT_EQ(size->width, 2339U);
  EXPECT_EQ(size->height, 1654U);
}

TEST(ExportSizeTest, BeyondThePngLimitsHasNoSize) {
  EXPECT_FALSE(ExportPixelSize(2000.0F, 2000.0F, 1e9F).has_value());
}

TEST(ExportSizeTest, ANonFiniteSideHasNoSize) {
  EXPECT_FALSE(
      ExportPixelSize(std::numeric_limits<float>::quiet_NaN(), 210.0F, 300.0F)
          .has_value());
}

TEST(ExportSizeTest, AnEmptySideHasNoSize) {
  EXPECT_FALSE(ExportPixelSize(0.0F, 210.0F, 300.0F).has_value());
}

TEST(ExportSizeTest, EffectiveDpiFollowsTheRoundedPixels) {
  EXPECT_NEAR(EffectiveDpi(3508U, 297.0F), 300.0108, 1e-3);
  EXPECT_NEAR(EffectiveDpi(2480U, 210.0F), 299.9619, 1e-3);
}
