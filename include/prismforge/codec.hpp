#pragma once

#include "prismforge/image.hpp"

#include <filesystem>

namespace prismforge {

Image load_image(const std::filesystem::path& path);
void save_image(const Image& image, const std::filesystem::path& path, int jpeg_quality = 90);
bool is_supported_image_extension(const std::filesystem::path& path);

} // namespace prismforge
