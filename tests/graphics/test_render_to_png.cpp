#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <optional>
#include <vector>

#include "graphics/render_to_png.hpp"

namespace {

constexpr std::size_t kBytesPerPixel = 4;

// Rows of pixels, top row first, every channel alike, so a test reads in
// pixels.
std::vector<unsigned char> Image(
    const std::vector<std::vector<unsigned char>>& rows) {
  std::vector<unsigned char> pixels;
  for (const auto& row : rows) {
    for (const unsigned char value : row) {
      pixels.insert(pixels.end(), kBytesPerPixel, value);
    }
  }
  return pixels;
}

}  // namespace

// GL reads a tile back bottom row first; the image runs top row first.
TEST(PlaceTileFlippedTest, PutsTheBottomTileRowIntoTheBottomImageRow) {
  std::vector<unsigned char> image = Image({{0, 0, 0}, {0, 0, 0}, {0, 0, 0}});
  const std::vector<unsigned char> tile = Image({{1, 2}, {3, 4}});

  render_to_png_detail::PlaceTileFlipped(image, {.width = 3, .height = 3}, tile,
                                         {.width = 2, .height = 2},
                                         {.left = 1, .bottom = 0});

  EXPECT_EQ(image, Image({{0, 0, 0}, {0, 3, 4}, {0, 1, 2}}));
}

TEST(PlaceTileFlippedTest, ATileAboveTheBottomLandsHigherUp) {
  std::vector<unsigned char> image = Image({{0, 0}, {0, 0}, {0, 0}});
  const std::vector<unsigned char> tile = Image({{5, 6}});

  render_to_png_detail::PlaceTileFlipped(image, {.width = 2, .height = 3}, tile,
                                         {.width = 2, .height = 1},
                                         {.left = 0, .bottom = 2});

  EXPECT_EQ(image, Image({{5, 6}, {0, 0}, {0, 0}}));
}

TEST(WritePngTest, WritesTheImageAndForwardsTheRows) {
  const auto file =
      std::filesystem::temp_directory_path() / "write_png_written.png";
  const RgbaImage image{
      .size = {.width = 2, .height = 3},
      .pixels = Image({{1, 2}, {3, 4}, {5, 6}}),
  };
  std::size_t rows = 0;

  const auto error = WritePng(file, image, [&rows](std::size_t rows_written) {
    rows = rows_written;
    return true;
  });

  EXPECT_EQ(error, std::nullopt);
  EXPECT_EQ(rows, 3U);
  EXPECT_TRUE(std::filesystem::exists(file));
  std::filesystem::remove(file);
}
