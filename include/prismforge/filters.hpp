#pragma once

#include "prismforge/image.hpp"

#include <vector>

namespace prismforge {

std::vector<double> gaussian_kernel(unsigned radius, double sigma);
Image gaussian_blur(const Image& image, unsigned radius, double sigma);
Image sobel_edges(const Image& image);
Image color_adjust(const Image& image, double brightness, double contrast, double saturation);

} // namespace prismforge
