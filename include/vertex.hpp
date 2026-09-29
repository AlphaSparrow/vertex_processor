#ifndef VERTEX_HPP
#define VERTEX_HPP

#include "math3d.hpp"
#include <vector>
#include <cstdint>
#include <cstring>

namespace vpu {

// Maximum number of generic interpolated varying float attributes
constexpr size_t MAX_VARYING_FLOATS = 16;

// Raw / Structured Input Vertex
struct VertexInput {
    Vec3 position;             // Object-space position (x, y, z)
    Vec3 normal{0, 1, 0};      // Surface normal
    Vec4 color{1, 1, 1, 1};    // Vertex RGBA color
    Vec2 uv{0, 0};             // Texture coordinates (u, v)
    Vec4 extra{0, 0, 0, 0};    // User/generic extra attribute

    VertexInput() = default;
    VertexInput(const Vec3& p) : position(p) {}
    VertexInput(const Vec3& p, const Vec3& n) : position(p), normal(n) {}
    VertexInput(const Vec3& p, const Vec4& c) : position(p), color(c) {}
    VertexInput(const Vec3& p, const Vec3& n, const Vec4& c) : position(p), normal(n), color(c) {}
    VertexInput(const Vec3& p, const Vec3& n, const Vec4& c, const Vec2& tex)
        : position(p), normal(n), color(c), uv(tex) {}
};

// Processed Vertex (Output of Vertex Processor)
struct ProcessedVertex {
    Vec4 clipPos;              // Homogeneous clip space: [x_c, y_c, z_c, w_c]
    Vec3 ndcPos;               // Normalized Device Coordinates [-1, 1]^3
    Vec3 screenPos;            // Screen coordinates: [x_pixel, y_pixel, z_depth]
    
    // Varyings passed down the graphics pipeline
    Vec3 worldPos;             // Interpolated world-space position
    Vec3 normal;               // Interpolated normal
    Vec4 color{1, 1, 1, 1};    // Interpolated color
    Vec2 uv{0, 0};             // Interpolated UV
    float customVaryings[MAX_VARYING_FLOATS]{};
    size_t customVaryingCount{0};

    // Linear interpolation of all vertex attributes (for clipping & rasterization)
    static ProcessedVertex lerp(const ProcessedVertex& a, const ProcessedVertex& b, float t) {
        ProcessedVertex out;
        out.clipPos = a.clipPos + (b.clipPos - a.clipPos) * t;
        out.worldPos = a.worldPos + (b.worldPos - a.worldPos) * t;
        out.normal = (a.normal + (b.normal - a.normal) * t).normalized();
        out.color = a.color + (b.color - a.color) * t;
        out.uv = a.uv + (b.uv - a.uv) * t;
        
        out.customVaryingCount = std::max(a.customVaryingCount, b.customVaryingCount);
        for (size_t i = 0; i < out.customVaryingCount; ++i) {
            out.customVaryings[i] = a.customVaryings[i] + (b.customVaryings[i] - a.customVaryings[i]) * t;
        }
        return out;
    }
};

// Vertex Buffer and Index Buffer Abstraction (Strided Raw Buffer Support)
enum class AttributeType {
    FLOAT,
    FLOAT2,
    FLOAT3,
    FLOAT4,
    UBYTE4_NORM
};

struct VertexAttributeDesc {
    uint32_t location;          // Shader register location
    AttributeType type;         // Component type & count
    uint32_t offset;            // Byte offset from start of vertex
};

struct VertexBufferLayout {
    uint32_t stride{0};
    std::vector<VertexAttributeDesc> attributes;

    void addAttribute(uint32_t loc, AttributeType type, uint32_t offset) {
        attributes.push_back({loc, type, offset});
    }
};

// Primitive topologies supported by the Vertex Processor
enum class PrimitiveTopology {
    TRIANGLES,
    TRIANGLE_STRIP,
    TRIANGLE_FAN,
    LINES,
    POINTS
};

} // namespace vpu

#endif // VERTEX_HPP
