#ifndef PNG_WRITER_HPP
#define PNG_WRITER_HPP

#include <cstddef>
#include <functional>
#include <span>

// Infrastructure: writes RGBA8 pixel buffers to PNG files via libpng.
//
// All of libpng's C API lives behind this component — the header names none of
// it, so no consumer sees png.h. libpng reports fatal errors through
// setjmp/longjmp, and a longjmp does *not* run C++ destructors as it unwinds —
// per [csetjmp] it is undefined behaviour if the jump would skip a non-trivial
// destructor. The unit respects that by constructing every object with a
// non-trivial destructor (the FILE owner) *before* the setjmp, so the jump can
// never bypass their cleanup. Raw stdio ownership is expressed with gsl::owner
// so the static analysis can verify it.
namespace png_io {

struct PngImageSize {
  std::size_t width{0};
  std::size_t height{0};
};

inline constexpr std::size_t kBytesPerPixel = 4;

// True if a width x height RGBA image stays within PNG's format and addressing
// limits. Anything larger cannot be written and must be rejected by the caller.
[[nodiscard]] bool FitsPngLimits(std::size_t width, std::size_t height);

// Hears after every row how many rows are written so far, and answers whether
// to go on; false abandons the file. It must not throw: it runs between
// libpng's setjmp and the rows it guards.
using RowProgress = std::function<bool(std::size_t rows_written)>;

// Writes the RGBA8 `pixels` (row-major, top-left origin) to `file_name`.
// Returns false if the file cannot be opened, libpng signals an error or
// `progress` asks to stop.
bool WriteRgbaPng(const char* file_name, std::span<const unsigned char> pixels,
                  PngImageSize size, const RowProgress& progress = {});

}  // namespace png_io

#endif  // PNG_WRITER_HPP
