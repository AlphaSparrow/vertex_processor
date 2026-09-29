#ifndef RASTERIZER_HPP
#define RASTERIZER_HPP

#include "framebuffer.hpp"
#include "vertex_processor.hpp"
#include <algorithm>
#include <cmath>

namespace vpu {

// Software Rasterizer (Verifies Vertex Processor Outputs)
class SoftwareRasterizer {
private:
    static float edgeFunction(const Vec3& a, const Vec3& b, float px, float py) {
        return (px - a.x) * (b.y - a.y) - (py - a.y) * (b.x - a.x);
    }

public:
    static void drawTriangle(Framebuffer& fb, const TrianglePrimitive& tri) {
        const ProcessedVertex& v0 = tri.v[0];
        const ProcessedVertex& v1 = tri.v[1];
        const ProcessedVertex& v2 = tri.v[2];

        // 2D screen coordinates
        const Vec3& p0 = v0.screenPos;
        const Vec3& p1 = v1.screenPos;
        const Vec3& p2 = v2.screenPos;

        // Bounding box of triangle in screen pixels
        float minX = std::floor(std::min({p0.x, p1.x, p2.x}));
        float maxX = std::ceil(std::max({p0.x, p1.x, p2.x}));
        float minY = std::floor(std::min({p0.y, p1.y, p2.y}));
        float maxY = std::ceil(std::max({p0.y, p1.y, p2.y}));

        int xStart = clamp(static_cast<int>(minX), 0, static_cast<int>(fb.getWidth() - 1));
        int xEnd   = clamp(static_cast<int>(maxX), 0, static_cast<int>(fb.getWidth() - 1));
        int yStart = clamp(static_cast<int>(minY), 0, static_cast<int>(fb.getHeight() - 1));
        int yEnd   = clamp(static_cast<int>(maxY), 0, static_cast<int>(fb.getHeight() - 1));

        // Area of triangle (twice the signed area)
        float area = edgeFunction(p0, p1, p2.x, p2.y);
        if (std::abs(area) < 1e-6f) return; // Degenerate sliver
        float invArea = 1.0f / area;

        // Precompute reciprocal w for perspective-correct attribute interpolation
        float invW0 = 1.0f / v0.clipPos.w;
        float invW1 = 1.0f / v1.clipPos.w;
        float invW2 = 1.0f / v2.clipPos.w;

        Vec4 c0_over_w = v0.color * invW0;
        Vec4 c1_over_w = v1.color * invW1;
        Vec4 c2_over_w = v2.color * invW2;

        (void)v0; (void)v1; (void)v2; // UV and custom varyings available for fragment stage

        // Scan across bounding box pixels
        for (int y = yStart; y <= yEnd; ++y) {
            float py = y + 0.5f; // Pixel center
            for (int x = xStart; x <= xEnd; ++x) {
                float px = x + 0.5f;

                // Evaluate edge functions
                float w0 = edgeFunction(p1, p2, px, py);
                float w1 = edgeFunction(p2, p0, px, py);
                float w2 = edgeFunction(p0, p1, px, py);

                // Check if sample point is inside triangle (handling both CW and CCW if not culled)
                bool inside;
                if (area > 0.0f) {
                    inside = (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f);
                } else {
                    inside = (w0 <= 0.0f && w1 <= 0.0f && w2 <= 0.0f);
                }

                if (inside) {
                    // Barycentric weights normalized
                    float lambda0 = w0 * invArea;
                    float lambda1 = w1 * invArea;
                    float lambda2 = w2 * invArea;

                    // Interpolate depth in screen space
                    float z = lambda0 * p0.z + lambda1 * p1.z + lambda2 * p2.z;

                    // Early depth test (Z-buffer)
                    if (fb.depthTestAndSet(x, y, z)) {
                        // Perspective-correct attribute interpolation
                        float interpolatedInvW = lambda0 * invW0 + lambda1 * invW1 + lambda2 * invW2;
                        float w = (interpolatedInvW > 1e-8f) ? (1.0f / interpolatedInvW) : 1.0f;

                        Vec4 color = (lambda0 * c0_over_w + lambda1 * c1_over_w + lambda2 * c2_over_w) * w;
                        // Clamp color channels
                        color.x = clamp(color.x, 0.0f, 1.0f);
                        color.y = clamp(color.y, 0.0f, 1.0f);
                        color.z = clamp(color.z, 0.0f, 1.0f);

                        fb.setPixel(x, y, Framebuffer::ColorRGB::fromVec4(color), z);
                    }
                }
            }
        }
    }

    static void render(Framebuffer& fb, const std::vector<TrianglePrimitive>& primitives) {
        for (const auto& tri : primitives) {
            drawTriangle(fb, tri);
        }
    }
};

} // namespace vpu

#endif // RASTERIZER_HPP
