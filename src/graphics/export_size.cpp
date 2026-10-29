#include "export_size.hpp"

#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>

#include "png_writer.hpp"

namespace {

constexpr float kInchInMillimeters = 25.4F;

std::optional<std::size_t> SidePixels(float millimetres,
                                      float dots_per_millimeter) {
  const float pixels = std::round(millimetres * dots_per_millimeter);
  if (!export_size_detail::FitsSizeT(pixels) || pixels < 1.0F) {
    return std::nullopt;
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

std::optional<ImageSize> ExportPixelSize(float width_mm, float height_mm,
                                         float dpi) {
  const float dots_per_millimeter =
      export_size_detail::DotsPerInchToDotsPerMillimeter(dpi);
  const std::optional<std::size_t> width =
      SidePixels(width_mm, dots_per_millimeter);
  const std::optional<std::size_t> height =
      SidePixels(height_mm, dots_per_millimeter);
  if (!width || !height || !png_io::FitsPngLimits(*width, *height)) {
    return std::nullopt;
  }
  return ImageSize{.width = *width, .height = *height};
}

double EffectiveDpi(std::size_t pixels, float millimetres) {
  return static_cast<double>(pixels) / static_cast<double>(millimetres) *
         static_cast<double>(kInchInMillimeters);
}
