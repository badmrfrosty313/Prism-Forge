#pragma once

#include "prismforge/image.hpp"

#include <filesystem>

namespace prismforge {
// PPM reader budget: at most 16,384 pixels in either dimension and 16 Mi pixels total.
inline constexpr std::size_t ppm_max_dimension = max_image_dimension;
inline constexpr std::size_t ppm_max_pixels = max_image_pixels;
Image load_ppm(const std::filesystem::path& path);
void save_ppm(const Image& image, const std::filesystem::path& path);
}
