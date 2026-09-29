#ifndef VERTEX_PROCESSOR_HPP
#define VERTEX_PROCESSOR_HPP

#include "math3d.hpp"
#include "vertex.hpp"
#include "microcode_shader.hpp"
#include <vector>
#include <functional>
#include <array>
#include <iostream>
#include <cstdint>

namespace vpu {

// Vertex Shader Interface (Callable Kernel)
struct Uniforms {
    Mat4 model;
    Mat4 view;
    Mat4 projection;
    Mat4 mvp;
    Mat4 normalMatrix; // (model^-1)^T
    Vec3 cameraPos;
    Vec3 lightDir;
    Vec4 lightColor{1.0f, 1.0f, 1.0f, 1.0f};
    Vec4 ambientColor{0.15f, 0.15f, 0.18f, 1.0f};
    float custom[16]{};
};

using VertexShaderFunc = std::function<ProcessedVertex(const VertexInput&, const Uniforms&)>;

// Post-Transform Vertex Cache (FIFO Hardware Emulation)
template <size_t CacheSize = 32>
class PostTransformCache {
private:
    struct CacheEntry {
        uint32_t index{0xFFFFFFFF};
        ProcessedVertex vertex;
        bool valid{false};
    };

    std::array<CacheEntry, CacheSize> entries;
    size_t fifoHead{0};

public:
    uint64_t hits{0};
    uint64_t misses{0};

    void reset() {
        for (auto& e : entries) e.valid = false;
        fifoHead = 0;
        hits = 0;
        misses = 0;
    }

    bool lookup(uint32_t index, ProcessedVertex& outVertex) {
        for (const auto& e : entries) {
            if (e.valid && e.index == index) {
                hits++;
                outVertex = e.vertex;
                return true;
            }
        }
        misses++;
        return false;
    }

    void insert(uint32_t index, const ProcessedVertex& v) {
        entries[fifoHead] = {index, v, true};
        fifoHead = (fifoHead + 1) % CacheSize;
    }

    float hitRatio() const {
        uint64_t total = hits + misses;
        return total > 0 ? (float)hits / (float)total : 0.0f;
    }
};

// Viewport Configuration
struct Viewport {
    float x{0.0f};
    float y{0.0f};
    float width{800.0f};
    float height{600.0f};
    float minDepth{0.0f};
    float maxDepth{1.0f};

    Viewport() = default;
    Viewport(float w, float h) : width(w), height(h) {}
    Viewport(float x_, float y_, float w, float h, float near_ = 0.0f, float far_ = 1.0f)
        : x(x_), y(y_), width(w), height(h), minDepth(near_), maxDepth(far_) {}
};

// Assembled Primitive
struct TrianglePrimitive {
    ProcessedVertex v[3];
    bool culled{false};
};

// Hardware Vertex Processor Unit (VPU)
class VertexProcessor {
public:
    enum class CullMode {
        NONE,
        FRONT,
        BACK
    };

    enum class WindingOrder {
        COUNTER_CLOCKWISE,
        CLOCKWISE
    };

    struct Telemetry {
        uint64_t verticesFetched{0};
        uint64_t shaderInvocations{0};
        uint64_t cacheHits{0};
        uint64_t cacheMisses{0};
        uint64_t inputPrimitives{0};
        uint64_t clippedPrimitives{0};
        uint64_t culledPrimitives{0};
        uint64_t outputPrimitives{0};

        void reset() {
            *this = Telemetry();
        }
    };

private:
    Uniforms uniforms;
    Viewport viewport;
    CullMode cullMode{CullMode::BACK};
    WindingOrder winding{WindingOrder::COUNTER_CLOCKWISE};
    PostTransformCache<32> vertexCache;
    VertexShaderFunc shaderKernel;
    MicrocodeVertexProgram microcodeProgram;
    bool useMicrocodeShader{false};
    Telemetry telemetry;

    // Frustum Clipping Planes (in Homogeneous Clip Space)
    // A point (x, y, z, w) is inside if:
    //   Left:   w + x >= 0
    //   Right:  w - x >= 0
    //   Bottom: w + y >= 0
    //   Top:    w - y >= 0
    //   Near:   w + z >= 0
    //   Far:    w - z >= 0
    enum class ClipPlane {
        NEAR_PLANE,
        FAR_PLANE,
        LEFT_PLANE,
        RIGHT_PLANE,
        BOTTOM_PLANE,
        TOP_PLANE
    };

    static bool isInsideClipPlane(const ProcessedVertex& v, ClipPlane plane) {
        switch (plane) {
            case ClipPlane::LEFT_PLANE:   return (v.clipPos.w + v.clipPos.x) >= 0.0f;
            case ClipPlane::RIGHT_PLANE:  return (v.clipPos.w - v.clipPos.x) >= 0.0f;
            case ClipPlane::BOTTOM_PLANE: return (v.clipPos.w + v.clipPos.y) >= 0.0f;
            case ClipPlane::TOP_PLANE:    return (v.clipPos.w - v.clipPos.y) >= 0.0f;
            case ClipPlane::NEAR_PLANE:   return (v.clipPos.w + v.clipPos.z) >= 0.0f;
            case ClipPlane::FAR_PLANE:    return (v.clipPos.w - v.clipPos.z) >= 0.0f;
        }
        return true;
    }

    static float planeDistance(const ProcessedVertex& v, ClipPlane plane) {
        switch (plane) {
            case ClipPlane::LEFT_PLANE:   return v.clipPos.w + v.clipPos.x;
            case ClipPlane::RIGHT_PLANE:  return v.clipPos.w - v.clipPos.x;
            case ClipPlane::BOTTOM_PLANE: return v.clipPos.w + v.clipPos.y;
            case ClipPlane::TOP_PLANE:    return v.clipPos.w - v.clipPos.y;
            case ClipPlane::NEAR_PLANE:   return v.clipPos.w + v.clipPos.z;
            case ClipPlane::FAR_PLANE:    return v.clipPos.w - v.clipPos.z;
        }
        return 0.0f;
    }

    // Sutherland-Hodgman Polygon Clipping against a single homogeneous plane
    std::vector<ProcessedVertex> clipPolygonAgainstPlane(
        const std::vector<ProcessedVertex>& polygon,
        ClipPlane plane) const
    {
        std::vector<ProcessedVertex> output;
        if (polygon.empty()) return output;

        for (size_t i = 0; i < polygon.size(); ++i) {
            const ProcessedVertex& current = polygon[i];
            const ProcessedVertex& next = polygon[(i + 1) % polygon.size()];

            bool currentInside = isInsideClipPlane(current, plane);
            bool nextInside = isInsideClipPlane(next, plane);

            if (currentInside && nextInside) {
                // Both inside -> output next
                output.push_back(next);
            } else if (currentInside && !nextInside) {
                // Exiting -> calculate intersection and output
                float d1 = planeDistance(current, plane);
                float d2 = planeDistance(next, plane);
                float t = d1 / (d1 - d2);
                output.push_back(ProcessedVertex::lerp(current, next, t));
            } else if (!currentInside && nextInside) {
                // Entering -> calculate intersection and output intersection + next
                float d1 = planeDistance(current, plane);
                float d2 = planeDistance(next, plane);
                float t = d1 / (d1 - d2);
                output.push_back(ProcessedVertex::lerp(current, next, t));
                output.push_back(next);
            }
            // Both outside -> output nothing
        }

        return output;
    }

    // Full 6-Plane Sutherland-Hodgman Homogeneous Frustum Clipping
    std::vector<ProcessedVertex> clipTriangleFrustum(const ProcessedVertex& v0,
                                                     const ProcessedVertex& v1,
                                                     const ProcessedVertex& v2) const
    {
        std::vector<ProcessedVertex> poly = {v0, v1, v2};

        // Clip against all 6 viewing frustum planes
        static const ClipPlane planes[] = {
            ClipPlane::NEAR_PLANE,
            ClipPlane::FAR_PLANE,
            ClipPlane::LEFT_PLANE,
            ClipPlane::RIGHT_PLANE,
            ClipPlane::BOTTOM_PLANE,
            ClipPlane::TOP_PLANE
        };

        for (ClipPlane plane : planes) {
            poly = clipPolygonAgainstPlane(poly, plane);
            if (poly.empty()) break;
        }

        return poly;
    }

    // Perspective Division & Viewport Mapping
    void transformToScreenSpace(ProcessedVertex& v) const {
        // 1. Perspective Division: Clip Space -> Normalized Device Coordinates (NDC)
        float invW = 1.0f / v.clipPos.w;
        v.ndcPos.x = v.clipPos.x * invW;
        v.ndcPos.y = v.clipPos.y * invW;
        v.ndcPos.z = v.clipPos.z * invW;

        // 2. Viewport Transformation: NDC [-1, 1] -> Screen Pixels [0, W] x [0, H]
        v.screenPos.x = viewport.x + (v.ndcPos.x + 1.0f) * 0.5f * viewport.width;
        // Y inverted for standard top-left screen origin
        v.screenPos.y = viewport.y + (1.0f - v.ndcPos.y) * 0.5f * viewport.height;
        // Depth mapped to [minDepth, maxDepth] (typically [0, 1])
        v.screenPos.z = viewport.minDepth + (v.ndcPos.z + 1.0f) * 0.5f * (viewport.maxDepth - viewport.minDepth);
    }

    // Shader Dispatcher
    ProcessedVertex executeVertexShader(const VertexInput& in) {
        telemetry.shaderInvocations++;

        if (useMicrocodeShader) {
            // Emulate execution via hardware microcode register engine
            RegisterFile rf;
            // Load input attributes into v registers
            rf.v[0] = Vec4(in.position, 1.0f);
            rf.v[1] = Vec4(in.normal, 0.0f);
            rf.v[2] = in.color;
            rf.v[3] = Vec4(in.uv.x, in.uv.y, 0.0f, 0.0f);

            // Execute microcode instructions
            microcodeProgram.execute(rf);

            ProcessedVertex out;
            out.clipPos = rf.oPos;
            out.worldPos = (uniforms.model * Vec4(in.position, 1.0f)).xyz();
            out.normal = rf.oNormal.xyz();
            out.color = rf.oCol0;
            out.uv = Vec2(rf.oTex0.x, rf.oTex0.y);
            return out;
        } else if (shaderKernel) {
            return shaderKernel(in, uniforms);
        } else {
            // Default Fixed-Function Vertex Processing Pipeline
            ProcessedVertex out;
            Vec4 worldP = uniforms.model * Vec4(in.position, 1.0f);
            out.worldPos = worldP.xyz();
            out.clipPos = uniforms.mvp * Vec4(in.position, 1.0f);
            out.normal = uniforms.normalMatrix.transformVector(in.normal).normalized();
            out.color = in.color;
            out.uv = in.uv;
            return out;
        }
    }

public:
    VertexProcessor() {
        setDefaultShader();
    }

    void setViewport(const Viewport& vp) { viewport = vp; }
    const Viewport& getViewport() const { return viewport; }

    void setCullMode(CullMode mode) { cullMode = mode; }
    void setWindingOrder(WindingOrder order) { winding = order; }

    Uniforms& getUniforms() { return uniforms; }
    const Uniforms& getUniforms() const { return uniforms; }

    void updateMatrices() {
        uniforms.mvp = uniforms.projection * uniforms.view * uniforms.model;
        uniforms.normalMatrix = uniforms.model.inverse().transposed();
    }

    void setShader(VertexShaderFunc func) {
        shaderKernel = func;
        useMicrocodeShader = false;
    }

    void setMicrocodeProgram(const MicrocodeVertexProgram& prog) {
        microcodeProgram = prog;
        useMicrocodeShader = true;
    }

    void setDefaultShader() {
        useMicrocodeShader = false;
        shaderKernel = [](const VertexInput& in, const Uniforms& u) -> ProcessedVertex {
            ProcessedVertex out;
            Vec4 world4 = u.model * Vec4(in.position, 1.0f);
            out.worldPos = world4.xyz();
            out.clipPos = u.mvp * Vec4(in.position, 1.0f);
            out.normal = u.normalMatrix.transformVector(in.normal).normalized();
            
            // Standard Gouraud Per-Vertex Lighting
            Vec3 lDir = -u.lightDir.normalized();
            float diff = std::max(out.normal.dot(lDir), 0.0f);
            
            // Phong specular calculation at vertex
            Vec3 viewDir = (u.cameraPos - out.worldPos).normalized();
            Vec3 reflectDir = (2.0f * out.normal.dot(lDir) * out.normal - lDir).normalized();
            float spec = std::pow(std::max(viewDir.dot(reflectDir), 0.0f), 16.0f);

            Vec4 diffuseLight = u.lightColor * diff;
            Vec4 specularLight = u.lightColor * spec * 0.5f;
            Vec4 litColor = in.color * (u.ambientColor + diffuseLight) + specularLight;
            litColor.w = in.color.w;

            out.color = litColor;
            out.uv = in.uv;
            return out;
        };
    }

    void resetTelemetry() {
        telemetry.reset();
        vertexCache.reset();
    }

    const Telemetry& getTelemetry() const {
        return telemetry;
    }

    // Vertex Processing Entry: Process Indexed Primitives (glDrawElements model)
    std::vector<TrianglePrimitive> processIndexed(
        const std::vector<VertexInput>& vertices,
        const std::vector<uint32_t>& indices,
        PrimitiveTopology topology = PrimitiveTopology::TRIANGLES)
    {
        std::vector<TrianglePrimitive> outputPrimitives;
        if (indices.empty() || vertices.empty()) return outputPrimitives;

        auto fetchAndShade = [&](uint32_t idx) -> ProcessedVertex {
            telemetry.verticesFetched++;
            ProcessedVertex pv;
            if (vertexCache.lookup(idx, pv)) {
                return pv;
            }
            pv = executeVertexShader(vertices[idx]);
            vertexCache.insert(idx, pv);
            return pv;
        };

        // Assemble input triangles based on topology
        std::vector<std::array<ProcessedVertex, 3>> rawTriangles;

        if (topology == PrimitiveTopology::TRIANGLES) {
            for (size_t i = 0; i + 2 < indices.size(); i += 3) {
                telemetry.inputPrimitives++;
                rawTriangles.push_back({
                    fetchAndShade(indices[i]),
                    fetchAndShade(indices[i + 1]),
                    fetchAndShade(indices[i + 2])
                });
            }
        } else if (topology == PrimitiveTopology::TRIANGLE_STRIP) {
            for (size_t i = 0; i + 2 < indices.size(); ++i) {
                telemetry.inputPrimitives++;
                if (i % 2 == 0) {
                    rawTriangles.push_back({
                        fetchAndShade(indices[i]),
                        fetchAndShade(indices[i + 1]),
                        fetchAndShade(indices[i + 2])
                    });
                } else {
                    // Alternate winding order to keep orientation consistent
                    rawTriangles.push_back({
                        fetchAndShade(indices[i]),
                        fetchAndShade(indices[i + 2]),
                        fetchAndShade(indices[i + 1])
                    });
                }
            }
        } else if (topology == PrimitiveTopology::TRIANGLE_FAN) {
            for (size_t i = 1; i + 1 < indices.size(); ++i) {
                telemetry.inputPrimitives++;
                rawTriangles.push_back({
                    fetchAndShade(indices[0]),
                    fetchAndShade(indices[i]),
                    fetchAndShade(indices[i + 1])
                });
            }
        }

        // Clip each triangle against the 6 frustum planes
        for (const auto& tri : rawTriangles) {
            std::vector<ProcessedVertex> clippedPoly = clipTriangleFrustum(tri[0], tri[1], tri[2]);

            if (clippedPoly.size() < 3) {
                telemetry.clippedPrimitives++;
                continue; // Triangle was completely outside frustum
            }

            // Fan triangulation of the clipped polygon
            for (size_t i = 1; i + 1 < clippedPoly.size(); ++i) {
                TrianglePrimitive prim;
                prim.v[0] = clippedPoly[0];
                prim.v[1] = clippedPoly[i];
                prim.v[2] = clippedPoly[i + 1];

                // Transform all 3 vertices to Screen Space
                transformToScreenSpace(prim.v[0]);
                transformToScreenSpace(prim.v[1]);
                transformToScreenSpace(prim.v[2]);

                // Screen-Space Backface Culling
                if (cullMode != CullMode::NONE) {
                    // 2D Cross Product (Signed Area * 2) in screen space
                    float edge1_x = prim.v[1].screenPos.x - prim.v[0].screenPos.x;
                    float edge1_y = prim.v[1].screenPos.y - prim.v[0].screenPos.y;
                    float edge2_x = prim.v[2].screenPos.x - prim.v[0].screenPos.x;
                    float edge2_y = prim.v[2].screenPos.y - prim.v[0].screenPos.y;
                    float signedArea = edge1_x * edge2_y - edge1_y * edge2_x;

                    bool isCCW = signedArea < 0.0f; // Remember Y is down in screen space
                    if (winding == WindingOrder::CLOCKWISE) isCCW = !isCCW;

                    if ((cullMode == CullMode::BACK && !isCCW) ||
                        (cullMode == CullMode::FRONT && isCCW)) {
                        telemetry.culledPrimitives++;
                        continue;
                    }
                }

                telemetry.outputPrimitives++;
                outputPrimitives.push_back(prim);
            }
        }

        // Update telemetry cache stats
        telemetry.cacheHits = vertexCache.hits;
        telemetry.cacheMisses = vertexCache.misses;

        return outputPrimitives;
    }

    // Direct Draw (glDrawArrays model)
    std::vector<TrianglePrimitive> processArrays(
        const std::vector<VertexInput>& vertices,
        PrimitiveTopology topology = PrimitiveTopology::TRIANGLES)
    {
        std::vector<uint32_t> indices(vertices.size());
        for (uint32_t i = 0; i < vertices.size(); ++i) indices[i] = i;
        return processIndexed(vertices, indices, topology);
    }
};

} // namespace vpu

#endif // VERTEX_PROCESSOR_HPP
