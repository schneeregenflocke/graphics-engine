#include "export_size.hpp"

#include <cmath>
#include <cstddef>
#include <expected>
#include <limits>

#include "png_writer.hpp"

namespace {

constexpr float kInchInMillimeters = 25.4F;

// The export holds the whole RGBA image, plus one tile, before libpng sees it
// (schneeregenflocke/decade#114); 4 GiB keeps that within a workstation's
// memory.
constexpr std::size_t kMaxImageBytes = std::size_t{1} << 32U;

std::expected<std::size_t, ExportSizeError> SidePixels(
    float millimetres, float dots_per_millimeter) {
  const float pixels = std::round(millimetres * dots_per_millimeter);
  // Written as `!(>= 1)` so a NaN side counts as empty.
  if (!(pixels >= 1.0F)) {
    return std::unexpected(ExportSizeError::kEmptySide);
  }
  if (!export_size_detail::FitsSizeT(pixels)) {
    return std::unexpected(ExportSizeError::kTooLarge);
  }
  return static_cast<std::size_t>(pixels);
}

}  // namespace

namespace export_size_detail {

float DotsPerInchToDotsPerMillimeter(float dpi) {
  return dpi / kInchInMillimeters;
}

bool FitsSizeT(float value) {
  const float limit =
      std::ldexp(1.0F, std::numeric_limits<std::size_t>::digits);
  return value >= 0.0F && value < limit;
}

}  // namespace export_size_detail

std::expected<ImageSize, ExportSizeError> ExportPixelSize(float width_mm,
                                                          float height_mm,
                                                          float dpi) {
  const float dots_per_millimeter =
      export_size_detail::DotsPerInchToDotsPerMillimeter(dpi);
  const auto width = SidePixels(width_mm, dots_per_millimeter);
  const auto height = SidePixels(height_mm, dots_per_millimeter);
  if (!width) {
    return std::unexpected(width.error());
  }
  if (!height) {
    return std::unexpected(height.error());
  }
  if (!png_io::FitsPngLimits(*width, *height) ||
      *width * *height > kMaxImageBytes / png_io::kBytesPerPixel) {
    return std::unexpected(ExportSizeError::kTooLarge);
  }
  return ImageSize{.width = *width, .height = *height};
}

double EffectiveDpi(std::size_t pixels, float millimetres) {
  return static_cast<double>(pixels) / static_cast<double>(millimetres) *
         static_cast<double>(kInchInMillimeters);
}
