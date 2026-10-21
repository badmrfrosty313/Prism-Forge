#include "prismforge/filters.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace prismforge {
namespace {
std::uint8_t clamp_byte(double value) {
    if (!std::isfinite(value)) throw std::invalid_argument("filter parameters must be finite");
    return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0, 255.0)));
}
std::size_t clamp_index(long long value, std::size_t limit) {
    if (value < 0) return 0;
    const auto converted = static_cast<unsigned long long>(value);
    return converted >= static_cast<unsigned long long>(limit) ? limit - 1 : static_cast<std::size_t>(converted);
}
double luminance(Pixel p) {
    return 0.299 * static_cast<double>(p.r) + 0.587 * static_cast<double>(p.g) + 0.114 * static_cast<double>(p.b);
}
}

std::vector<double> gaussian_kernel(unsigned radius, double sigma) {
    if (radius > 128) throw std::invalid_argument("Gaussian radius must be at most 128");
    if (!std::isfinite(sigma) || sigma <= 0.0) throw std::invalid_argument("sigma must be finite and positive");
    const auto extent = static_cast<std::size_t>(radius) * 2U + 1U;
    std::vector<double> kernel(extent);
    if (sigma < 1e-150) {
        kernel[radius] = 1.0;
        return kernel;
    }
    double sum = 0.0;
    const double denominator = 2.0 * sigma * sigma;
    for (std::size_t i = 0; i < extent; ++i) {
        const auto offset = static_cast<double>(i) - static_cast<double>(radius);
        kernel[i] = std::exp(-(offset * offset) / denominator);
        sum += kernel[i];
    }
    for (auto& value : kernel) value /= sum;
    return kernel;
}

Image gaussian_blur(const Image& image, unsigned radius, double sigma) {
    const auto kernel = gaussian_kernel(radius, sigma);
    if (radius == 0) return image;
    Image horizontal(image.width(), image.height());
    Image output(image.width(), image.height());
    for (std::size_t y = 0; y < image.height(); ++y) {
        for (std::size_t x = 0; x < image.width(); ++x) {
            double channels[3]{0.0, 0.0, 0.0};
            for (std::size_t k = 0; k < kernel.size(); ++k) {
                const auto offset = static_cast<long long>(k) - static_cast<long long>(radius);
                const auto sx = clamp_index(static_cast<long long>(x) + offset, image.width());
                const auto p = image.at(sx, y);
                channels[0] += kernel[k] * p.r; channels[1] += kernel[k] * p.g; channels[2] += kernel[k] * p.b;
            }
            horizontal.at(x, y) = {clamp_byte(channels[0]), clamp_byte(channels[1]), clamp_byte(channels[2])};
        }
    }
    for (std::size_t y = 0; y < image.height(); ++y) {
        for (std::size_t x = 0; x < image.width(); ++x) {
            double channels[3]{0.0, 0.0, 0.0};
            for (std::size_t k = 0; k < kernel.size(); ++k) {
                const auto offset = static_cast<long long>(k) - static_cast<long long>(radius);
                const auto sy = clamp_index(static_cast<long long>(y) + offset, image.height());
                const auto p = horizontal.at(x, sy);
                channels[0] += kernel[k] * p.r; channels[1] += kernel[k] * p.g; channels[2] += kernel[k] * p.b;
            }
            output.at(x, y) = {clamp_byte(channels[0]), clamp_byte(channels[1]), clamp_byte(channels[2])};
        }
    }
    return output;
}

Image sobel_edges(const Image& image) {
    Image output(image.width(), image.height());
    constexpr int gx[3][3]{{-1,0,1},{-2,0,2},{-1,0,1}};
    constexpr int gy[3][3]{{-1,-2,-1},{0,0,0},{1,2,1}};
    for (std::size_t y = 0; y < image.height(); ++y) {
        for (std::size_t x = 0; x < image.width(); ++x) {
            double dx = 0.0, dy = 0.0;
            for (int ky = -1; ky <= 1; ++ky) for (int kx = -1; kx <= 1; ++kx) {
                const auto sx = clamp_index(static_cast<long long>(x) + kx, image.width());
                const auto sy = clamp_index(static_cast<long long>(y) + ky, image.height());
                const double lum = luminance(image.at(sx, sy));
                dx += lum * gx[ky + 1][kx + 1]; dy += lum * gy[ky + 1][kx + 1];
            }
            const auto magnitude = clamp_byte(std::hypot(dx, dy));
            output.at(x, y) = {magnitude, magnitude, magnitude};
        }
    }
    return output;
}

Image color_adjust(const Image& image, double brightness, double contrast, double saturation) {
    if (!std::isfinite(brightness) || !std::isfinite(contrast) || !std::isfinite(saturation))
        throw std::invalid_argument("color parameters must be finite");
    if (contrast < 0.0 || saturation < 0.0) throw std::invalid_argument("contrast and saturation must be nonnegative");
    Image output(image.width(), image.height());
    for (std::size_t y = 0; y < image.height(); ++y) for (std::size_t x = 0; x < image.width(); ++x) {
        const auto p = image.at(x, y);
        const double lum = luminance(p);
        const double values[3]{static_cast<double>(p.r), static_cast<double>(p.g), static_cast<double>(p.b)};
        Pixel adjusted;
        std::uint8_t* channels[3]{&adjusted.r, &adjusted.g, &adjusted.b};
        for (int c = 0; c < 3; ++c) {
            const double saturated = lum + saturation * (values[c] - lum);
            *channels[c] = clamp_byte((saturated - 127.5) * contrast + 127.5 + brightness);
        }
        output.at(x, y) = adjusted;
    }
    return output;
}

} // namespace prismforge
