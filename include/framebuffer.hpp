#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include "math3d.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdint>
#include <algorithm>

namespace vpu {

// Framebuffer with Color and Depth (Z-Buffer)
class Framebuffer {
public:
    struct ColorRGB {
        uint8_t r{0};
        uint8_t g{0};
        uint8_t b{0};

        ColorRGB() = default;
        ColorRGB(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}
        static ColorRGB fromVec4(const Vec4& c) {
            auto clampByte = [](float v) -> uint8_t {
                int val = static_cast<int>(v * 255.0f + 0.5f);
                return static_cast<uint8_t>(clamp(val, 0, 255));
            };
            return {clampByte(c.x), clampByte(c.y), clampByte(c.z)};
        }
    };

private:
    uint32_t width;
    uint32_t height;
    std::vector<ColorRGB> colorBuffer;
    std::vector<float> depthBuffer;

public:
    Framebuffer(uint32_t w, uint32_t h)
        : width(w), height(h), colorBuffer(w * h, ColorRGB(0, 0, 0)), depthBuffer(w * h, 1.0f) {}

    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }

    void clear(const ColorRGB& clearColor = {15, 15, 20}, float clearDepth = 1.0f) {
        std::fill(colorBuffer.begin(), colorBuffer.end(), clearColor);
        std::fill(depthBuffer.begin(), depthBuffer.end(), clearDepth);
    }

    void setPixel(int x, int y, const ColorRGB& c, float depth) {
        if (x < 0 || x >= static_cast<int>(width) || y < 0 || y >= static_cast<int>(height)) return;
        size_t idx = y * width + x;
        colorBuffer[idx] = c;
        depthBuffer[idx] = depth;
    }

    ColorRGB getPixel(int x, int y) const {
        if (x < 0 || x >= static_cast<int>(width) || y < 0 || y >= static_cast<int>(height)) return {0, 0, 0};
        return colorBuffer[y * width + x];
    }

    float getDepth(int x, int y) const {
        if (x < 0 || x >= static_cast<int>(width) || y < 0 || y >= static_cast<int>(height)) return 1.0f;
        return depthBuffer[y * width + x];
    }

    bool depthTestAndSet(int x, int y, float depth) {
        if (x < 0 || x >= static_cast<int>(width) || y < 0 || y >= static_cast<int>(height)) return false;
        size_t idx = y * width + x;
        if (depth < depthBuffer[idx]) {
            depthBuffer[idx] = depth;
            return true;
        }
        return false;
    }

    // C++ BMP File Exporter (Standard Uncompressed 24-bit BMP)
    bool saveBMP(const std::string& filename) const {
        std::ofstream out(filename, std::ios::binary);
        if (!out.is_open()) return false;

        uint32_t rowStride = (width * 3 + 3) & ~3; // 4-byte aligned scanline
        uint32_t imageSize = rowStride * height;
        uint32_t fileSize = 54 + imageSize;

        // BMP Header (14 bytes)
        uint8_t fileHeader[14] = {
            'B', 'M',                               // Signature
            static_cast<uint8_t>(fileSize),         // File size (little endian)
            static_cast<uint8_t>(fileSize >> 8),
            static_cast<uint8_t>(fileSize >> 16),
            static_cast<uint8_t>(fileSize >> 24),
            0, 0, 0, 0,                             // Reserved
            54, 0, 0, 0                             // Pixel data offset
        };

        // DIB Header / BITMAPINFOHEADER (40 bytes)
        uint8_t dibHeader[40] = {
            40, 0, 0, 0,                            // DIB header size
            static_cast<uint8_t>(width),            // Width
            static_cast<uint8_t>(width >> 8),
            static_cast<uint8_t>(width >> 16),
            static_cast<uint8_t>(width >> 24),
            static_cast<uint8_t>(height),           // Height (positive = bottom-to-top)
            static_cast<uint8_t>(height >> 8),
            static_cast<uint8_t>(height >> 16),
            static_cast<uint8_t>(height >> 24),
            1, 0,                                   // Planes = 1
            24, 0,                                  // Bits per pixel = 24
            0, 0, 0, 0,                             // Compression (BI_RGB = 0)
            static_cast<uint8_t>(imageSize),        // Image data size
            static_cast<uint8_t>(imageSize >> 8),
            static_cast<uint8_t>(imageSize >> 16),
            static_cast<uint8_t>(imageSize >> 24),
            0x13, 0x0B, 0, 0,                       // Horizontal resolution (2835 ppm ~ 72 DPI)
            0x13, 0x0B, 0, 0,                       // Vertical resolution
            0, 0, 0, 0,                             // Color palette count
            0, 0, 0, 0                              // Important colors
        };

        out.write(reinterpret_cast<const char*>(fileHeader), sizeof(fileHeader));
        out.write(reinterpret_cast<const char*>(dibHeader), sizeof(dibHeader));

        std::vector<uint8_t> rowBuffer(rowStride, 0);

        // BMP stores bottom scanlines first
        for (int y = static_cast<int>(height) - 1; y >= 0; --y) {
            for (uint32_t x = 0; x < width; ++x) {
                const ColorRGB& c = colorBuffer[y * width + x];
                rowBuffer[x * 3 + 0] = c.b; // BMP format uses BGR
                rowBuffer[x * 3 + 1] = c.g;
                rowBuffer[x * 3 + 2] = c.r;
            }
            out.write(reinterpret_cast<const char*>(rowBuffer.data()), rowStride);
        }

        return true;
    }

    // ASCII Console Visualizer
    // Downsamples the framebuffer to fit in a standard terminal
    std::string toASCII(int targetCols = 80, int targetRows = 30) const {
        static const char ramp[] = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
        constexpr size_t rampLen = sizeof(ramp) - 2;

        std::string ascii;
        ascii.reserve((targetCols + 1) * targetRows);

        float xStep = static_cast<float>(width) / static_cast<float>(targetCols);
        float yStep = static_cast<float>(height) / static_cast<float>(targetRows);

        for (int r = 0; r < targetRows; ++r) {
            for (int c = 0; c < targetCols; ++c) {
                int px = clamp(static_cast<int>(c * xStep), 0, static_cast<int>(width - 1));
                int py = clamp(static_cast<int>(r * yStep), 0, static_cast<int>(height - 1));
                
                float depth = getDepth(px, py);
                if (depth >= 0.999f) {
                    ascii.push_back(' '); // Background
                } else {
                    const ColorRGB& rgb = getPixel(px, py);
                    // Perceived luminance
                    float lum = (0.2126f * rgb.r + 0.7152f * rgb.g + 0.0722f * rgb.b) / 255.0f;
                    int charIdx = static_cast<int>(lum * rampLen);
                    charIdx = clamp(charIdx, 0, static_cast<int>(rampLen));
                    ascii.push_back(ramp[charIdx]);
                }
            }
            ascii.push_back('\n');
        }
        return ascii;
    }
};

} // namespace vpu

#endif // FRAMEBUFFER_HPP
