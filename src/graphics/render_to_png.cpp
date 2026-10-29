#include "render_to_png.hpp"

#include <epoxy/gl.h>

#include <algorithm>
#include <cstddef>
#include <expected>
#include <format>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "export_size.hpp"
#include "graphics_engine.hpp"
#include "graphics_log.hpp"
#include "mvp_matrices.hpp"
#include "png_writer.hpp"
#include "rect.hpp"
#include "render_to_texture.hpp"

namespace render_to_png_detail {

void PlaceTileFlipped(std::span<unsigned char> image, ImageSize image_size,
                      std::span<const unsigned char> tile, ImageSize tile_size,
                      TileOrigin origin) {
  const std::size_t image_row_bytes = image_size.width * png_io::kBytesPerPixel;
  const std::size_t tile_row_bytes = tile_size.width * png_io::kBytesPerPixel;
  const std::size_t left_bytes = origin.left * png_io::kBytesPerPixel;
  for (std::size_t tile_row = 0; tile_row < tile_size.height; ++tile_row) {
    const std::size_t image_row =
        image_size.height - 1 - (origin.bottom + tile_row);
    const auto source = tile.subspan(tile_row * tile_row_bytes, tile_row_bytes);
    const auto destination = image.subspan(
        (image_row * image_row_bytes) + left_bytes, tile_row_bytes);
    std::ranges::copy(source, destination.begin());
  }
}

}  // namespace render_to_png_detail

ImageComposer::ImageComposer(ImageSize image_size, const RectF& ortho_region_in,
                             GraphicsEngine& graphics_engine_in,
                             int msaa_samples_in)
    : width_(image_size.width),
      height_(image_size.height),
      ortho_region_(ortho_region_in),
      graphics_engine_(graphics_engine_in),
      tile_size_(kMaxTileSize),
      msaa_samples_(msaa_samples_in) {
  CalculatePixelRemainders();
  CalculateTileOrthoSize();
  CalculateRemainderOrthoSize();
  CalculateTileGrid();

  ConfigureTiles();
  RenderTiles();
}

bool ImageComposer::Rendered() const { return rendered_; }

ImageTile& ImageComposer::TileAt(size_t column, size_t row) {
  return tiles_[column + (row * tile_columns_)];
}

std::vector<unsigned char> ImageComposer::TakeImage() && {
  return std::move(image_);
}

size_t ImageComposer::TileCount() const { return tiles_.size(); }

size_t ImageComposer::TileColumns() const { return tile_columns_; }

size_t ImageComposer::TileRows() const { return tile_rows_; }

void ImageComposer::CalculatePixelRemainders() {
  width_remainder_ = width_ % tile_size_;
  height_remainder_ = height_ % tile_size_;
}

void ImageComposer::CalculateTileOrthoSize() {
  constexpr float kOne = 1.0F;
  float width_ratio = kOne;
  float height_ratio = kOne;

  if (width_ > tile_size_) {
    width_ratio = static_cast<float>(tile_size_) / static_cast<float>(width_);
  }

  if (height_ > tile_size_) {
    height_ratio = static_cast<float>(tile_size_) / static_cast<float>(height_);
  }

  tile_ortho_width_ = ortho_region_.Width() * width_ratio;
  tile_ortho_height_ = ortho_region_.Height() * height_ratio;
}

void ImageComposer::CalculateRemainderOrthoSize() {
  const float width_remainder_ratio =
      static_cast<float>(width_remainder_) / static_cast<float>(width_);
  const float height_remainder_ratio =
      static_cast<float>(height_remainder_) / static_cast<float>(height_);

  remainder_ortho_width_ = ortho_region_.Width() * width_remainder_ratio;
  remainder_ortho_height_ = ortho_region_.Height() * height_remainder_ratio;
}

void ImageComposer::CalculateTileGrid() {
  const size_t full_columns = width_ / tile_size_;
  const size_t full_rows = height_ / tile_size_;

  tile_columns_ = full_columns + (width_remainder_ > 0 ? 1 : 0);
  tile_rows_ = full_rows + (height_remainder_ > 0 ? 1 : 0);

  tiles_.resize(tile_columns_ * tile_rows_);
}

void ImageComposer::ConfigureTiles() {
  const bool width_has_remainder = width_remainder_ > 0;
  const bool height_has_remainder = height_remainder_ > 0;

  for (size_t row = 0; row < tile_rows_; ++row) {
    for (size_t column = 0; column < tile_columns_; ++column) {
      GLsizei tile_pixel_width = 0;
      GLsizei tile_pixel_height = 0;

      const auto column_float = static_cast<float>(column);
      const auto row_float = static_cast<float>(row);

      RectF tile_region;

      if ((column == tile_columns_ - 1) && width_has_remainder) {
        tile_pixel_width = static_cast<GLsizei>(width_remainder_);
        tile_region.SetLeft(ortho_region_.Left() +
                            (column_float * tile_ortho_width_));
        tile_region.SetRight(tile_region.Left() + remainder_ortho_width_);
      } else {
        tile_pixel_width = static_cast<GLsizei>(tile_size_);
        tile_region.SetLeft(ortho_region_.Left() +
                            (column_float * tile_ortho_width_));
        tile_region.SetRight(tile_region.Left() + tile_ortho_width_);
      }

      if ((row == tile_rows_ - 1) && height_has_remainder) {
        tile_pixel_height = static_cast<GLsizei>(height_remainder_);
        tile_region.SetBottom(ortho_region_.Bottom() +
                              (row_float * tile_ortho_height_));
        tile_region.SetTop(tile_region.Bottom() + remainder_ortho_height_);
      } else {
        tile_pixel_height = static_cast<GLsizei>(tile_size_);
        tile_region.SetBottom(ortho_region_.Bottom() +
                              (row_float * tile_ortho_height_));
        tile_region.SetTop(tile_region.Bottom() + tile_ortho_height_);
      }

      TileAt(column, row).pixel_dimensions[0] = tile_pixel_width;
      TileAt(column, row).pixel_dimensions[1] = tile_pixel_height;
      TileAt(column, row).ortho_region = tile_region;
    }
  }
}

void ImageComposer::RenderTiles() {
  MVP mvp;
  mvp.SetView(glm::translate(glm::mat4(1.0F), glm::vec3(0.0F, 0.0F, 0.0F)));
  image_.resize(kBytesPerPixel * width_ * height_);

  for (size_t row = 0; row < TileRows(); ++row) {
    for (size_t column = 0; column < TileColumns(); ++column) {
      auto& tile = TileAt(column, row);

      RenderToTexture render_texture(tile.pixel_dimensions[0],
                                     tile.pixel_dimensions[1], msaa_samples_);
      // Without a complete framebuffer the render goes nowhere and the
      // read-back hands back a tile of zeros — a black page, silently. The
      // caller has to hear about it instead of writing the file.
      if (!render_texture.Valid()) {
        rendered_ = false;
        image_ = {};
        return;
      }
      if (graphics_log::LogEnabled() && row == 0 && column == 0) {
        std::cout << "--dump-png: " << render_texture.Samples()
                  << " samples per pixel (asked for " << msaa_samples_ << ")\n";
      }

      render_texture.BeginRender();
      mvp.SetProjection(
          glm::ortho(tile.ortho_region.Left(), tile.ortho_region.Right(),
                     tile.ortho_region.Bottom(), tile.ortho_region.Top()));

      graphics_engine_.SetMVP(mvp);
      graphics_engine_.Render(GraphicsEngine::Backdrop::kPaper);

      render_texture.EndRender();

      const ImageSize tile_size{
          .width = static_cast<size_t>(tile.pixel_dimensions[0]),
          .height = static_cast<size_t>(tile.pixel_dimensions[1]),
      };
      render_to_png_detail::PlaceTileFlipped(
          image_, {.width = width_, .height = height_},
          render_texture.CopyImage(), tile_size,
          {.left = column * tile_size_, .bottom = row * tile_size_});
    }
  }
}

std::expected<RgbaImage, std::string> RenderPage(
    const RectF& ortho_region, float dpi, GraphicsEngine& graphics_engine,
    int msaa_samples) {
  const auto image_size =
      ExportPixelSize(ortho_region.Width(), ortho_region.Height(), dpi);
  if (!image_size) {
    if (image_size.error() == ExportSizeError::kEmptySide) {
      return std::unexpected(
          std::format("The page of {} x {} mm has an empty side.",
                      ortho_region.Width(), ortho_region.Height()));
    }
    return std::unexpected(std::format(
        "The image of {} x {} mm at {} dpi exceeds the export limit of 4 GiB.",
        ortho_region.Width(), ortho_region.Height(), dpi));
  }

  ImageComposer composer(*image_size, ortho_region, graphics_engine,
                         msaa_samples);
  if (!composer.Rendered()) {
    return std::unexpected(
        "The off-screen framebuffer did not come up; nothing written.");
  }
  return RgbaImage{
      .size = *image_size,
      .pixels = std::move(composer).TakeImage(),
  };
}

std::optional<std::string> WritePng(const std::string& file_path,
                                    const RgbaImage& image,
                                    const png_io::RowProgress& progress) {
  const png_io::PngImageSize png_size{
      .width = image.size.width,
      .height = image.size.height,
  };
  if (!png_io::WriteRgbaPng(file_path.c_str(), image.pixels, png_size,
                            progress)) {
    return "Writing " + file_path + " failed.";
  }
  return std::nullopt;
}
