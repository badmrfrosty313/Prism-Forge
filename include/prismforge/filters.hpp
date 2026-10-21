#pragma once

#include "prismforge/image.hpp"

#include <vector>

namespace prismforge {

std::vector<double> gaussian_kernel(unsigned radius, double sigma);
Image gaussian_blur(const Image& image, unsigned radius, double sigma);
Image sobel_edges(const Image& image);
Image color_adjust(const Image& image, double brightness, double contrast, double saturation);
Image grayscale(const Image& image);
Image sharpen(const Image& image, double amount = 1.0);
Image invert(const Image& image);
Image resize_nearest(const Image& image, std::size_t width, std::size_t height);
Image resize_bilinear(const Image& image, std::size_t width, std::size_t height);

} // namespace prismforge
