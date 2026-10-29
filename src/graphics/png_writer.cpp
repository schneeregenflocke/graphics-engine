#include "png_writer.hpp"

#include <cstddef>
#include <cstdio>
#include <gsl/pointers>
#include <memory>
#include <span>

extern "C" {
#include <png.h>
#include <pnglibconf.h>
#include <setjmp.h>
#include <stddef.h>
#include <stdlib.h>
}

namespace png_io {
namespace {

// Deleter that lets a unique_ptr own a C FILE. Taking a gsl::owner makes the
// ownership transfer into fclose explicit for the analyser.
struct FileCloser {
  void operator()(gsl::owner<std::FILE*> file) const noexcept {
    (void)std::fclose(file);
  }
};

using UniqueFile = std::unique_ptr<std::FILE, FileCloser>;

// Opens `file_name` for binary writing. Returns an owning handle (null on
// failure) that the caller hands straight to a UniqueFile.
[[nodiscard]] gsl::owner<std::FILE*> OpenForWrite(const char* file_name) {
  gsl::owner<std::FILE*> file = nullptr;
#ifdef _MSC_VER
  (void)fopen_s(&file, file_name, "wb");
#else
  file = std::fopen(file_name, "wb");
#endif  // _MSC_VER
  return file;
}

}  // namespace

// The width and height tests of libpng's png_check_IHDR, which png_set_IHDR
// runs on writing as on reading. The user limits stay at libpng's build-time
// default, since nothing here calls png_set_user_limits.
bool FitsPngLimits(std::size_t width, std::size_t height) {
  // libpng's bound term by term: its row buffer margin of 48 bytes, the filter
  // byte, pixels of up to 8 bytes and one pixel of padding.
  constexpr std::size_t kMaxPaddedWidth = ((PNG_SIZE_MAX - 48 - 1) / 8) - 1;
  const std::size_t padded_width = (width + 7) & ~std::size_t{7};
  return width > 0 && height > 0 && width <= PNG_UINT_31_MAX &&
         height <= PNG_UINT_31_MAX && padded_width <= kMaxPaddedWidth &&
         width <= PNG_USER_WIDTH_MAX && height <= PNG_USER_HEIGHT_MAX;
}

bool WriteRgbaPng(const char* file_name, std::span<const unsigned char> pixels,
                  PngImageSize size, const RowProgress& progress) {
  // see https://sourceforge.net/p/libpng/code/ci/master/tree/example.c#l739

  const UniqueFile file(OpenForWrite(file_name));
  if (!file) {
    return false;
  }

  png_structp png =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (png == nullptr) {
    return false;
  }

  png_infop info = png_create_info_struct(png);
  if (info == nullptr) {
    png_destroy_write_struct(&png, nullptr);
    return false;
  }

  // The one construct libpng's contract forces on us and that has no in-code
  // fix: its default error handler longjmps back here. Suppression is scoped
  // to exactly these two checks (see AGENTS.md, "Warnings, the clang-tidy and
  // the sanitizer gate").
  // NOLINTNEXTLINE(cert-err52-cpp,modernize-avoid-setjmp-longjmp)
  if (setjmp(png_jmpbuf(png)) != 0) {
    png_destroy_write_struct(&png, &info);
    return false;
  }

  png_init_io(png, file.get());

  constexpr int kBitDepth = 8;  // bits per RGBA channel
  png_set_IHDR(png, info, static_cast<png_uint_32>(size.width),
               static_cast<png_uint_32>(size.height), kBitDepth,
               PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(png, info);

  // png_write_row reads through a const pointer, so the caller's image stays
  // untouched and needs no copy; png_write_rows would want writable rows.
  const std::size_t row_bytes = size.width * kBytesPerPixel;
  for (std::size_t row = 0; row < size.height; ++row) {
    png_write_row(png, pixels.subspan(row * row_bytes, row_bytes).data());
    if (progress && !progress(row + 1)) {
      png_destroy_write_struct(&png, &info);
      return false;
    }
  }

  png_write_end(png, info);
  png_destroy_write_struct(&png, &info);
  return true;
}

}  // namespace png_io
