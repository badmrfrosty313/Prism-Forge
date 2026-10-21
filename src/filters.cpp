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
            horizontal.at(x, y) = {clamp_byte(channels[0]), clamp_byte(channels[1]), clamp_byte(channels[2]), image.at(x, y).a};
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
            output.at(x, y) = {clamp_byte(channels[0]), clamp_byte(channels[1]), clamp_byte(channels[2]), image.at(x, y).a};
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
            output.at(x, y) = {magnitude, magnitude, magnitude, image.at(x, y).a};
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
        adjusted.a = p.a;
        output.at(x, y) = adjusted;
    }
    return output;
}

Image grayscale(const Image& image) {
    Image output(image.width(), image.height());
    for (std::size_t y = 0; y < image.height(); ++y) for (std::size_t x = 0; x < image.width(); ++x) {
        const auto source = image.at(x, y);
        const auto gray = clamp_byte(luminance(source));
        output.at(x, y) = {gray, gray, gray, source.a};
    }
    return output;
}

Image sharpen(const Image& image, double amount) {
    if (!std::isfinite(amount) || amount < 0.0 || amount > 10.0)
        throw std::invalid_argument("sharpen amount must be finite and in 0..10");
    Image output(image.width(), image.height());
    for (std::size_t y = 0; y < image.height(); ++y) for (std::size_t x = 0; x < image.width(); ++x) {
        const auto center = image.at(x, y);
        const auto left = image.at(clamp_index(static_cast<long long>(x) - 1, image.width()), y);
        const auto right = image.at(clamp_index(static_cast<long long>(x) + 1, image.width()), y);
        const auto top = image.at(x, clamp_index(static_cast<long long>(y) - 1, image.height()));
        const auto bottom = image.at(x, clamp_index(static_cast<long long>(y) + 1, image.height()));
        const std::uint8_t centers[3]{center.r, center.g, center.b};
        const int neighbors[3]{static_cast<int>(left.r) + right.r + top.r + bottom.r,
                               static_cast<int>(left.g) + right.g + top.g + bottom.g,
                               static_cast<int>(left.b) + right.b + top.b + bottom.b};
        Pixel result;
        std::uint8_t* channels[3]{&result.r, &result.g, &result.b};
        for (int channel = 0; channel < 3; ++channel)
            *channels[channel] = clamp_byte(static_cast<double>(centers[channel]) + amount *
                                            (4.0 * centers[channel] - neighbors[channel]));
        result.a = center.a;
        output.at(x, y) = result;
    }
    return output;
}

Image invert(const Image& image) {
    Image output(image.width(), image.height());
    for (std::size_t y = 0; y < image.height(); ++y) for (std::size_t x = 0; x < image.width(); ++x) {
        const auto source = image.at(x, y);
        output.at(x, y) = {static_cast<std::uint8_t>(255U - source.r),
                           static_cast<std::uint8_t>(255U - source.g),
                           static_cast<std::uint8_t>(255U - source.b), source.a};
    }
    return output;
}

Image resize_nearest(const Image& image, std::size_t width, std::size_t height) {
    Image output(width, height);
    const double width_scale = static_cast<double>(image.width()) / static_cast<double>(width);
    const double height_scale = static_cast<double>(image.height()) / static_cast<double>(height);
    for (std::size_t y = 0; y < height; ++y) for (std::size_t x = 0; x < width; ++x) {
        const auto sx = std::min(image.width() - 1, static_cast<std::size_t>((static_cast<double>(x) + 0.5) * width_scale));
        const auto sy = std::min(image.height() - 1, static_cast<std::size_t>((static_cast<double>(y) + 0.5) * height_scale));
        output.at(x, y) = image.at(sx, sy);
    }
    return output;
}

Image resize_bilinear(const Image& image, std::size_t width, std::size_t height) {
    Image output(width, height);
    const double width_scale = static_cast<double>(image.width()) / static_cast<double>(width);
    const double height_scale = static_cast<double>(image.height()) / static_cast<double>(height);
    const auto interpolate = [](double a, double b, double c, double d, double fx, double fy) {
        const double top = a + (b - a) * fx;
        const double bottom = c + (d - c) * fx;
        return clamp_byte(top + (bottom - top) * fy);
    };
    for (std::size_t y = 0; y < height; ++y) for (std::size_t x = 0; x < width; ++x) {
        const double source_x = std::clamp((static_cast<double>(x) + 0.5) * width_scale - 0.5,
                                           0.0, static_cast<double>(image.width() - 1));
        const double source_y = std::clamp((static_cast<double>(y) + 0.5) * height_scale - 0.5,
                                           0.0, static_cast<double>(image.height() - 1));
        const auto x0 = static_cast<std::size_t>(source_x), y0 = static_cast<std::size_t>(source_y);
        const auto x1 = std::min(x0 + 1, image.width() - 1), y1 = std::min(y0 + 1, image.height() - 1);
        const double fx = source_x - static_cast<double>(x0), fy = source_y - static_cast<double>(y0);
        const auto p00 = image.at(x0, y0), p10 = image.at(x1, y0), p01 = image.at(x0, y1), p11 = image.at(x1, y1);
        output.at(x, y) = {interpolate(p00.r,p10.r,p01.r,p11.r,fx,fy),
                           interpolate(p00.g,p10.g,p01.g,p11.g,fx,fy),
                           interpolate(p00.b,p10.b,p01.b,p11.b,fx,fy),
                           interpolate(p00.a,p10.a,p01.a,p11.a,fx,fy)};
    }
    return output;
}

} // namespace prismforge
