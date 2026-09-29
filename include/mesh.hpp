#ifndef MESH_HPP
#define MESH_HPP

#include "vertex.hpp"
#include <vector>
#include <cmath>

namespace vpu {

struct Mesh {
    std::vector<VertexInput> vertices;
    std::vector<uint32_t> indices;
};

// Procedural 3D Mesh Generators
class MeshGenerator {
public:
    // Generate a 3D Colored Cube with explicit normals and vertex colors
    static Mesh createColorCube(float size = 1.0f) {
        Mesh mesh;
        float h = size * 0.5f;

        // 6 faces * 4 vertices = 24 vertices for sharp normals
        // Front (+Z) - Red
        mesh.vertices.push_back({{-h, -h,  h}, {0, 0, 1}, {1.0f, 0.2f, 0.2f, 1.0f}, {0, 0}});
        mesh.vertices.push_back({{ h, -h,  h}, {0, 0, 1}, {1.0f, 0.2f, 0.2f, 1.0f}, {1, 0}});
        mesh.vertices.push_back({{ h,  h,  h}, {0, 0, 1}, {1.0f, 0.2f, 0.2f, 1.0f}, {1, 1}});
        mesh.vertices.push_back({{-h,  h,  h}, {0, 0, 1}, {1.0f, 0.2f, 0.2f, 1.0f}, {0, 1}});

        // Back (-Z) - Cyan
        mesh.vertices.push_back({{ h, -h, -h}, {0, 0, -1}, {0.2f, 1.0f, 1.0f, 1.0f}, {0, 0}});
        mesh.vertices.push_back({{-h, -h, -h}, {0, 0, -1}, {0.2f, 1.0f, 1.0f, 1.0f}, {1, 0}});
        mesh.vertices.push_back({{-h,  h, -h}, {0, 0, -1}, {0.2f, 1.0f, 1.0f, 1.0f}, {1, 1}});
        mesh.vertices.push_back({{ h,  h, -h}, {0, 0, -1}, {0.2f, 1.0f, 1.0f, 1.0f}, {0, 1}});

        // Top (+Y) - Green
        mesh.vertices.push_back({{-h,  h,  h}, {0, 1, 0}, {0.2f, 1.0f, 0.2f, 1.0f}, {0, 0}});
        mesh.vertices.push_back({{ h,  h,  h}, {0, 1, 0}, {0.2f, 1.0f, 0.2f, 1.0f}, {1, 0}});
        mesh.vertices.push_back({{ h,  h, -h}, {0, 1, 0}, {0.2f, 1.0f, 0.2f, 1.0f}, {1, 1}});
        mesh.vertices.push_back({{-h,  h, -h}, {0, 1, 0}, {0.2f, 1.0f, 0.2f, 1.0f}, {0, 1}});

        // Bottom (-Y) - Magenta
        mesh.vertices.push_back({{-h, -h, -h}, {0, -1, 0}, {1.0f, 0.2f, 1.0f, 1.0f}, {0, 0}});
        mesh.vertices.push_back({{ h, -h, -h}, {0, -1, 0}, {1.0f, 0.2f, 1.0f, 1.0f}, {1, 0}});
        mesh.vertices.push_back({{ h, -h,  h}, {0, -1, 0}, {1.0f, 0.2f, 1.0f, 1.0f}, {1, 1}});
        mesh.vertices.push_back({{-h, -h,  h}, {0, -1, 0}, {1.0f, 0.2f, 1.0f, 1.0f}, {0, 1}});

        // Right (+X) - Blue
        mesh.vertices.push_back({{ h, -h,  h}, {1, 0, 0}, {0.2f, 0.4f, 1.0f, 1.0f}, {0, 0}});
        mesh.vertices.push_back({{ h, -h, -h}, {1, 0, 0}, {0.2f, 0.4f, 1.0f, 1.0f}, {1, 0}});
        mesh.vertices.push_back({{ h,  h, -h}, {1, 0, 0}, {0.2f, 0.4f, 1.0f, 1.0f}, {1, 1}});
        mesh.vertices.push_back({{ h,  h,  h}, {1, 0, 0}, {0.2f, 0.4f, 1.0f, 1.0f}, {0, 1}});

        // Left (-X) - Yellow
        mesh.vertices.push_back({{-h, -h, -h}, {-1, 0, 0}, {1.0f, 1.0f, 0.2f, 1.0f}, {0, 0}});
        mesh.vertices.push_back({{-h, -h,  h}, {-1, 0, 0}, {1.0f, 1.0f, 0.2f, 1.0f}, {1, 0}});
        mesh.vertices.push_back({{-h,  h,  h}, {-1, 0, 0}, {1.0f, 1.0f, 0.2f, 1.0f}, {1, 1}});
        mesh.vertices.push_back({{-h,  h, -h}, {-1, 0, 0}, {1.0f, 1.0f, 0.2f, 1.0f}, {0, 1}});

        for (uint32_t face = 0; face < 6; ++face) {
            uint32_t base = face * 4;
            mesh.indices.push_back(base + 0);
            mesh.indices.push_back(base + 1);
            mesh.indices.push_back(base + 2);
            mesh.indices.push_back(base + 0);
            mesh.indices.push_back(base + 2);
            mesh.indices.push_back(base + 3);
        }

        return mesh;
    }

    // Generate a 3D UV Sphere
    static Mesh createSphere(float radius = 1.0f, int rings = 16, int sectors = 32) {
        Mesh mesh;
        for (int r = 0; r <= rings; ++r) {
            float phi = PI * static_cast<float>(r) / static_cast<float>(rings); // 0 to PI
            for (int s = 0; s <= sectors; ++s) {
                float theta = 2.0f * PI * static_cast<float>(s) / static_cast<float>(sectors); // 0 to 2PI

                float x = std::sin(phi) * std::cos(theta);
                float y = std::cos(phi);
                float z = std::sin(phi) * std::sin(theta);

                Vec3 pos = Vec3(x, y, z) * radius;
                Vec3 norm = Vec3(x, y, z).normalized();
                Vec4 col = {0.85f, 0.65f, 0.25f, 1.0f}; // Warm gold
                Vec2 uv = {static_cast<float>(s) / sectors, static_cast<float>(r) / rings};

                mesh.vertices.push_back({pos, norm, col, uv});
            }
        }

        for (int r = 0; r < rings; ++r) {
            for (int s = 0; s < sectors; ++s) {
                uint32_t cur = r * (sectors + 1) + s;
                uint32_t next = cur + sectors + 1;

                mesh.indices.push_back(cur);
                mesh.indices.push_back(next);
                mesh.indices.push_back(cur + 1);

                mesh.indices.push_back(cur + 1);
                mesh.indices.push_back(next);
                mesh.indices.push_back(next + 1);
            }
        }
        return mesh;
    }

    // Generate a 3D Torus
    static Mesh createTorus(float rMajor = 1.0f, float rMinor = 0.35f, int numMajor = 24, int numMinor = 16) {
        Mesh mesh;
        for (int i = 0; i <= numMajor; ++i) {
            float u = 2.0f * PI * static_cast<float>(i) / numMajor;
            Vec3 center = {std::cos(u) * rMajor, 0.0f, std::sin(u) * rMajor};

            for (int j = 0; j <= numMinor; ++j) {
                float v = 2.0f * PI * static_cast<float>(j) / numMinor;
                Vec3 pos = {
                    (rMajor + rMinor * std::cos(v)) * std::cos(u),
                    rMinor * std::sin(v),
                    (rMajor + rMinor * std::cos(v)) * std::sin(u)
                };

                Vec3 norm = (pos - center).normalized();
                Vec4 col = {0.3f, 0.7f, 1.0f, 1.0f}; // Cyan/Blue
                Vec2 uv = {static_cast<float>(i) / numMajor, static_cast<float>(j) / numMinor};

                mesh.vertices.push_back({pos, norm, col, uv});
            }
        }

        for (int i = 0; i < numMajor; ++i) {
            for (int j = 0; j < numMinor; ++j) {
                uint32_t cur = i * (numMinor + 1) + j;
                uint32_t next = (i + 1) * (numMinor + 1) + j;

                mesh.indices.push_back(cur);
                mesh.indices.push_back(cur + 1);
                mesh.indices.push_back(next);

                mesh.indices.push_back(next);
                mesh.indices.push_back(cur + 1);
                mesh.indices.push_back(next + 1);
            }
        }
        return mesh;
    }

    // Massive Triangle that crosses the camera frustum boundary to demonstrate frustum clipping
    static Mesh createFrustumClippedTriangle() {
        Mesh mesh;
        mesh.vertices.push_back({{-8.0f, -4.0f, -2.0f}, {0, 0, 1}, {1.0f, 0.0f, 0.0f, 1.0f}, {0, 0}});
        mesh.vertices.push_back({{ 8.0f, -4.0f, -2.0f}, {0, 0, 1}, {0.0f, 1.0f, 0.0f, 1.0f}, {1, 0}});
        mesh.vertices.push_back({{ 0.0f,  9.0f, -2.0f}, {0, 0, 1}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.5f, 1}});

        mesh.indices.push_back(0);
        mesh.indices.push_back(1);
        mesh.indices.push_back(2);
        return mesh;
    }
};

} // namespace vpu

#endif // MESH_HPP
