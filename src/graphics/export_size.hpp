#ifndef EXPORT_SIZE_HPP
#define EXPORT_SIZE_HPP

#include <cstddef>
#include <cstdint>
#include <expected>

// The pixel size of an export, apart from any GL, so a form can show what the
// export will produce before anything gets rendered.
struct ImageSize {
  std::size_t width{0};
  std::size_t height{0};
};

// Why a page cannot become an image: a side rounds to no pixel at all (or is
// no number), or the image outgrows what libpng writes.
enum class ExportSizeError : std::uint8_t { kEmptySide, kTooLarge };

namespace export_size_detail {

[[nodiscard]] float DotsPerInchToDotsPerMillimeter(float dpi);

// Whether `value` survives a static_cast to size_t. That cast is undefined for
// anything outside the target's range, so the range gets tested while the value
// is still a float — FitsPngLimits sees the number after the cast and therefore
// too late. The page size comes out of a project file unvalidated
// (`value_serialization.hpp`, no clamp in `SetSize`), so an absurd extent times
// the dpi does reach here.
//
// 2^64 is the first value a 64-bit size_t cannot hold; NaN fails the lower
// test, which is the point of writing it as `>= 0` rather than `!(< 0)`.
[[nodiscard]] bool FitsSizeT(float value);

}  // namespace export_size_detail

// Each side rounds on its own, so the effective resolution differs slightly
// per axis. The image may hold no more than libpng writes; memory sets no
// bound here.
[[nodiscard]] std::expected<ImageSize, ExportSizeError> ExportPixelSize(
    float width_mm, float height_mm, float dpi);

// The resolution a side really gets after rounding to whole pixels.
[[nodiscard]] double EffectiveDpi(std::size_t pixels, float millimetres);

#endif  // EXPORT_SIZE_HPP
