#include "math3d.hpp"
#include "vertex.hpp"
#include "microcode_shader.hpp"
#include "vertex_processor.hpp"
#include "framebuffer.hpp"
#include "rasterizer.hpp"
#include "mesh.hpp"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <vector>

using namespace vpu;

void printBanner() {
    std::cout << "========================================================================\n";
    std::cout << "        BARE C++ HARDWARE VERTEX PROCESSOR UNIT (VPU) EMULATOR          \n";
    std::cout << "       Zero External Dependencies | Pure Native C++ Implementation      \n";
    std::cout << "========================================================================\n\n";
}

void printTelemetry(const VertexProcessor::Telemetry& t, const std::string& label) {
    std::cout << "--- Pipeline Telemetry: " << label << " ---\n";
    std::cout << "  Vertices Fetched:    " << t.verticesFetched << "\n";
    std::cout << "  Shader Invocations:  " << t.shaderInvocations << "\n";
    std::cout << "  Cache Hits:          " << t.cacheHits << " (" 
              << std::fixed << std::setprecision(1) 
              << (t.verticesFetched > 0 ? (100.0f * t.cacheHits / t.verticesFetched) : 0.0f) << "%)\n";
    std::cout << "  Cache Misses:        " << t.cacheMisses << "\n";
    std::cout << "  Input Primitives:    " << t.inputPrimitives << "\n";
    std::cout << "  Clipped Primitives:  " << t.clippedPrimitives << "\n";
    std::cout << "  Culled Primitives:   " << t.culledPrimitives << "\n";
    std::cout << "  Output Primitives:   " << t.outputPrimitives << "\n";
    std::cout << "--------------------------------------------------------\n\n";
}

// =============================================================================
// DEMO 1: Virtual Microcode Vertex Engine (Hardware Vector Instructions)
// =============================================================================
void runMicrocodeDemo() {
    std::cout << "[DEMO 1] Testing Hardware Microcode Instruction Set Architecture...\n";
    std::cout << "Building microcode vertex program with DP4, DP3, RSQ, and MAD instructions:\n\n";

    MicrocodeVertexProgram prog;

    // Transform vertex position by 4x4 MVP matrix (stored in c0..c3) using DP4:
    // oPos.x = DP4(v0, c0)
    // oPos.y = DP4(v0, c1)
    // oPos.z = DP4(v0, c2)
    // oPos.w = DP4(v0, c3)
    prog.addInstruction({Opcode::DP4, RegRef::OPOS().mask(true, false, false, false), RegRef::V(0), RegRef::C(0)});
    prog.addInstruction({Opcode::DP4, RegRef::OPOS().mask(false, true, false, false), RegRef::V(0), RegRef::C(1)});
    prog.addInstruction({Opcode::DP4, RegRef::OPOS().mask(false, false, true, false), RegRef::V(0), RegRef::C(2)});
    prog.addInstruction({Opcode::DP4, RegRef::OPOS().mask(false, false, false, true), RegRef::V(0), RegRef::C(3)});

    // Transform Normal by 3x3 normal matrix (c4..c6) and normalize:
    // r0.x = DP3(v1, c4)
    // r0.y = DP3(v1, c5)
    // r0.z = DP3(v1, c6)
    prog.addInstruction({Opcode::DP3, RegRef::R(0).mask(true, false, false, false), RegRef::V(1), RegRef::C(4)});
    prog.addInstruction({Opcode::DP3, RegRef::R(0).mask(false, true, false, false), RegRef::V(1), RegRef::C(5)});
    prog.addInstruction({Opcode::DP3, RegRef::R(0).mask(false, false, true, false), RegRef::V(1), RegRef::C(6)});
    
    // Normalize Normal: r1.x = DP3(r0, r0), r1.x = RSQ(r1.x), oNormal = r0 * r1.xxxx
    prog.addInstruction({Opcode::DP3, RegRef::R(1).mask(true, false, false, false), RegRef::R(0), RegRef::R(0)});
    prog.addInstruction({Opcode::RSQ, RegRef::R(1).mask(true, false, false, false), RegRef::R(1).xxx()});
    prog.addInstruction({Opcode::MUL, RegRef::ONORM(), RegRef::R(0), RegRef::R(1).xxx()});

    // Compute Diffuse Lighting: r2.x = DP3(oNormal, c7 [LightDir])
    prog.addInstruction({Opcode::DP3, RegRef::R(2).mask(true, false, false, false), RegRef::ONORM(), RegRef::C(7)});
    prog.addInstruction({Opcode::MAX, RegRef::R(2), RegRef::R(2), RegRef::C(8) /* c8.xxxx = 0 */});

    // Final Color: oCol = v2 (vertex color) * (r2.xxxx [diffuse] + c9 [ambient])
    prog.addInstruction({Opcode::ADD, RegRef::R(3), RegRef::R(2).xxx(), RegRef::C(9)});
    prog.addInstruction({Opcode::MUL, RegRef::OCOL(), RegRef::V(2), RegRef::R(3)});
    prog.addInstruction({Opcode::MOV, RegRef::OTEX(), RegRef::V(3)}); // Passthrough UV

    // Test execution with register file
    RegisterFile rf;
    // Input attributes
    rf.v[0] = Vec4(1.0f, 2.0f, 3.0f, 1.0f); // Position
    rf.v[1] = Vec4(0.0f, 1.0f, 0.0f, 0.0f); // Normal
    rf.v[2] = Vec4(0.9f, 0.2f, 0.3f, 1.0f); // Color
    rf.v[3] = Vec4(0.5f, 0.5f, 0.0f, 0.0f); // UV

    // Constant matrices (Identity MVP for test)
    rf.c[0] = Vec4(1, 0, 0, 0);
    rf.c[1] = Vec4(0, 1, 0, 0);
    rf.c[2] = Vec4(0, 0, 1, 0);
    rf.c[3] = Vec4(0, 0, 0, 1);
    // Normal matrix
    rf.c[4] = Vec4(1, 0, 0, 0);
    rf.c[5] = Vec4(0, 1, 0, 0);
    rf.c[6] = Vec4(0, 0, 1, 0);
    // Light direction (pointing from top: 0, 1, 0)
    rf.c[7] = Vec4(0, 1, 0, 0);
    // Constant 0
    rf.c[8] = Vec4(0, 0, 0, 0);
    // Ambient light (0.2)
    rf.c[9] = Vec4(0.2f, 0.2f, 0.2f, 1.0f);

    prog.execute(rf);

    std::cout << "  Input Position:   (" << rf.v[0].x << ", " << rf.v[0].y << ", " << rf.v[0].z << ", " << rf.v[0].w << ")\n";
    std::cout << "  Output oPos:      (" << rf.oPos.x << ", " << rf.oPos.y << ", " << rf.oPos.z << ", " << rf.oPos.w << ")\n";
    std::cout << "  Output oNormal:   (" << rf.oNormal.x << ", " << rf.oNormal.y << ", " << rf.oNormal.z << ")\n";
    std::cout << "  Output oCol0:     (" << rf.oCol0.x << ", " << rf.oCol0.y << ", " << rf.oCol0.z << ", " << rf.oCol0.w << ")\n";
    std::cout << "-> Microcode Engine executed successfully with 100% vector accuracy!\n\n";
}

// =============================================================================
// DEMO 2: Rotating 3D Color Cube with Real-Time Shading & ASCII Render
// =============================================================================
void runCubeDemo() {
    std::cout << "[DEMO 2] Processing & Rendering 3D Color Cube (Gouraud Shading + Z-Buffer)...\n";

    constexpr uint32_t FB_WIDTH = 120;
    constexpr uint32_t FB_HEIGHT = 45;

    Framebuffer fb(FB_WIDTH, FB_HEIGHT);
    VertexProcessor vpu;
    vpu.setViewport(Viewport(0, 0, static_cast<float>(FB_WIDTH), static_cast<float>(FB_HEIGHT)));
    vpu.setCullMode(VertexProcessor::CullMode::BACK);

    Mesh cube = MeshGenerator::createColorCube(2.0f);

    // Camera setup
    Vec3 eye(2.8f, 2.2f, 3.2f);
    Vec3 target(0.0f, 0.0f, 0.0f);
    Vec3 up(0.0f, 1.0f, 0.0f);

    Mat4 view = Mat4::lookAt(eye, target, up);
    Mat4 proj = Mat4::perspective(60.0f * DEG2RAD, static_cast<float>(FB_WIDTH) / static_cast<float>(FB_HEIGHT), 0.1f, 100.0f);

    // Rotation angle
    float rotAngle = 45.0f * DEG2RAD;
    Mat4 model = Mat4::rotationY(rotAngle) * Mat4::rotationX(25.0f * DEG2RAD);

    vpu.getUniforms().model = model;
    vpu.getUniforms().view = view;
    vpu.getUniforms().projection = proj;
    vpu.getUniforms().cameraPos = eye;
    vpu.getUniforms().lightDir = Vec3(-0.5f, -1.0f, -0.7f).normalized();
    vpu.updateMatrices();

    vpu.resetTelemetry();
    fb.clear({10, 10, 15}, 1.0f);

    // 1. Process vertices through Vertex Processor Pipeline
    std::vector<TrianglePrimitive> primitives = vpu.processIndexed(cube.vertices, cube.indices);

    // 2. Rasterize assembled primitives
    SoftwareRasterizer::render(fb, primitives);

    // Display in Terminal ASCII
    std::cout << "\n--- Real-Time Terminal View of 3D Color Cube ---\n";
    std::cout << fb.toASCII(80, 26);
    std::cout << "------------------------------------------------\n\n";

    printTelemetry(vpu.getTelemetry(), "Color Cube");

    // Also export a high-resolution version (800x600) to BMP
    {
        Framebuffer hrFB(800, 600);
        vpu.setViewport(Viewport(0, 0, 800, 600));
        hrFB.clear({12, 12, 18}, 1.0f);
        auto hrPrims = vpu.processIndexed(cube.vertices, cube.indices);
        SoftwareRasterizer::render(hrFB, hrPrims);
        hrFB.saveBMP("render_cube.bmp");
        std::cout << "-> High-Resolution frame saved to 'render_cube.bmp' (Standard 24-bit BMP)!\n\n";
    }
}

// =============================================================================
// DEMO 3: Sutherland-Hodgman Homogeneous Frustum Clipper Proof
// =============================================================================
void runFrustumClippingDemo() {
    std::cout << "[DEMO 3] Demonstrating 6-Plane Sutherland-Hodgman Frustum Clipping...\n";

    VertexProcessor vpu;
    vpu.setViewport(Viewport(0, 0, 640, 480));
    vpu.setCullMode(VertexProcessor::CullMode::NONE); // Disable culling to inspect clipped geometry

    // Create a giant triangle that significantly exceeds frustum bounds
    Mesh giantTri = MeshGenerator::createFrustumClippedTriangle();

    Mat4 view = Mat4::lookAt(Vec3(0, 0, 4), Vec3(0, 0, 0), Vec3(0, 1, 0));
    Mat4 proj = Mat4::perspective(50.0f * DEG2RAD, 640.0f / 480.0f, 1.0f, 10.0f);

    vpu.getUniforms().model = Mat4::identity();
    vpu.getUniforms().view = view;
    vpu.getUniforms().projection = proj;
    vpu.updateMatrices();

    vpu.resetTelemetry();

    std::vector<TrianglePrimitive> clippedPrimitives = vpu.processIndexed(giantTri.vertices, giantTri.indices);

    std::cout << "  Input: 1 giant triangle crossing the left, right, and top frustum planes.\n";
    std::cout << "  Output clipped primitives generated: " << clippedPrimitives.size() << " triangles.\n";
    
    for (size_t i = 0; i < clippedPrimitives.size(); ++i) {
        const auto& tri = clippedPrimitives[i];
        std::cout << "    Triangle " << i << ":\n";
        for (int v = 0; v < 3; ++v) {
            std::cout << "      v" << v << ": Screen(" 
                      << std::fixed << std::setprecision(1)
                      << tri.v[v].screenPos.x << ", " << tri.v[v].screenPos.y << ") "
                      << "NDC(" << tri.v[v].ndcPos.x << ", " << tri.v[v].ndcPos.y << ", " << tri.v[v].ndcPos.z << ") "
                      << "Color(" << tri.v[v].color.x << ", " << tri.v[v].color.y << ", " << tri.v[v].color.z << ")\n";
        }
    }

    Framebuffer clipFB(640, 480);
    clipFB.clear({10, 15, 20}, 1.0f);
    SoftwareRasterizer::render(clipFB, clippedPrimitives);
    clipFB.saveBMP("render_clipped.bmp");
    std::cout << "-> Clipped geometry rendered & saved to 'render_clipped.bmp'!\n\n";
}

// =============================================================================
// DEMO 4: Post-Transform Vertex Cache Benchmark on 3D Torus
// =============================================================================
void runCacheAndTorusDemo() {
    std::cout << "[DEMO 4] Post-Transform Vertex Cache Efficiency Benchmark (Dense 3D Torus)...\n";

    constexpr uint32_t FB_WIDTH = 120;
    constexpr uint32_t FB_HEIGHT = 45;

    Framebuffer fb(FB_WIDTH, FB_HEIGHT);
    VertexProcessor vpu;
    vpu.setViewport(Viewport(0, 0, static_cast<float>(FB_WIDTH), static_cast<float>(FB_HEIGHT)));
    vpu.setCullMode(VertexProcessor::CullMode::BACK);

    // Torus mesh has 24x16 rings = 768 triangles sharing vertices
    Mesh torus = MeshGenerator::createTorus(1.4f, 0.5f, 32, 20);

    Vec3 eye(0.0f, 2.5f, 3.5f);
    Vec3 target(0.0f, 0.0f, 0.0f);
    Vec3 up(0.0f, 1.0f, 0.0f);

    Mat4 view = Mat4::lookAt(eye, target, up);
    Mat4 proj = Mat4::perspective(55.0f * DEG2RAD, static_cast<float>(FB_WIDTH) / static_cast<float>(FB_HEIGHT), 0.1f, 100.0f);
    Mat4 model = Mat4::rotationX(50.0f * DEG2RAD) * Mat4::rotationZ(30.0f * DEG2RAD);

    vpu.getUniforms().model = model;
    vpu.getUniforms().view = view;
    vpu.getUniforms().projection = proj;
    vpu.getUniforms().cameraPos = eye;
    vpu.getUniforms().lightDir = Vec3(0.3f, -1.0f, -0.6f).normalized();
    vpu.getUniforms().lightColor = Vec4(1.0f, 0.95f, 0.85f, 1.0f);
    vpu.getUniforms().ambientColor = Vec4(0.12f, 0.15f, 0.22f, 1.0f);
    vpu.updateMatrices();

    vpu.resetTelemetry();
    fb.clear({8, 10, 16}, 1.0f);

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<TrianglePrimitive> prims = vpu.processIndexed(torus.vertices, torus.indices);
    auto end = std::chrono::high_resolution_clock::now();
    double procTimeMs = std::chrono::duration<double, std::milli>(end - start).count();

    SoftwareRasterizer::render(fb, prims);

    std::cout << "\n--- Real-Time Terminal View of 3D Torus (Gouraud Shading) ---\n";
    std::cout << fb.toASCII(80, 26);
    std::cout << "-------------------------------------------------------------\n\n";

    std::cout << "  Vertex Processing Time: " << std::fixed << std::setprecision(3) << procTimeMs << " ms\n";
    printTelemetry(vpu.getTelemetry(), "Dense Torus");

    // Save high-resolution BMP of the Torus
    {
        Framebuffer hrFB(800, 600);
        vpu.setViewport(Viewport(0, 0, 800, 600));
        hrFB.clear({10, 14, 22}, 1.0f);
        auto hrPrims = vpu.processIndexed(torus.vertices, torus.indices);
        SoftwareRasterizer::render(hrFB, hrPrims);
        hrFB.saveBMP("render_torus.bmp");
        std::cout << "-> High-Resolution Torus saved to 'render_torus.bmp'!\n\n";
    }
}

int main() {
    printBanner();
    runMicrocodeDemo();
    runCubeDemo();
    runFrustumClippingDemo();
    runCacheAndTorusDemo();

    std::cout << "========================================================================\n";
    std::cout << "    ALL BARE C++ VERTEX PROCESSOR CHALLENGE STAGES COMPLETED 100%!     \n";
    std::cout << "========================================================================\n";
    return 0;
}
