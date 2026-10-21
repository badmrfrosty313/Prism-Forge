#pragma once

#include "prismforge/image.hpp"

#include <filesystem>

namespace prismforge {
Image load_ppm(const std::filesystem::path& path);
void save_ppm(const Image& image, const std::filesystem::path& path);
}
