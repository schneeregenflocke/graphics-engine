#include <gtest/gtest.h>
#include <png.h>

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include "graphics/png_writer.hpp"

namespace {

std::filesystem::path TempPng(const std::string& name) {
  return std::filesystem::temp_directory_path() / name;
}

std::vector<unsigned char> ReadRgba(const std::filesystem::path& file) {
  png_image image{};
  image.version = PNG_IMAGE_VERSION;
  if (png_image_begin_read_from_file(&image, file.c_str()) == 0) {
    return {};
  }
  image.format = PNG_FORMAT_RGBA;
  std::vector<unsigned char> pixels(PNG_IMAGE_SIZE(image));
  if (png_image_finish_read(&image, nullptr, pixels.data(), 0, nullptr) == 0) {
    return {};
  }
  return pixels;
}

// Two rows of three pixels, each byte distinct, so a swapped row shows.
std::vector<unsigned char> TwoRows() {
  std::vector<unsigned char> pixels(std::size_t{2} * 3 *
                                    png_io::kBytesPerPixel);
  for (std::size_t index = 0; index < pixels.size(); ++index) {
    pixels[index] = static_cast<unsigned char>(index);
  }
  return pixels;
}

}  // namespace

TEST(PngWriterTest, WritesTheRowsTopDownAsGiven) {
  const auto file = TempPng("png_writer_round_trip.png");
  const std::vector<unsigned char> pixels = TwoRows();

  ASSERT_TRUE(png_io::WriteRgbaPng(file.c_str(), std::span(pixels),
                                   {.width = 3, .height = 2}));

  EXPECT_EQ(ReadRgba(file), pixels);
  std::filesystem::remove(file);
}

TEST(PngWriterTest, ReportsEveryWrittenRow) {
  const auto file = TempPng("png_writer_progress.png");
  const std::vector<unsigned char> pixels = TwoRows();
  std::vector<std::size_t> reported;

  ASSERT_TRUE(png_io::WriteRgbaPng(file.c_str(), std::span(pixels),
                                   {.width = 3, .height = 2},
                                   [&reported](std::size_t rows_written) {
                                     reported.push_back(rows_written);
                                     return true;
                                   }));

  EXPECT_EQ(reported, (std::vector<std::size_t>{1, 2}));
  std::filesystem::remove(file);
}

TEST(PngWriterTest, AnObserverSayingStopAbandonsTheFile) {
  const auto file = TempPng("png_writer_abandoned.png");
  const std::vector<unsigned char> pixels = TwoRows();

  EXPECT_FALSE(png_io::WriteRgbaPng(file.c_str(), std::span(pixels),
                                    {.width = 3, .height = 2},
                                    [](std::size_t) { return false; }));
  std::filesystem::remove(file);
}
