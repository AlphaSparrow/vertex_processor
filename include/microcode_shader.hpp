#ifndef MICROCODE_SHADER_HPP
#define MICROCODE_SHADER_HPP

#include "math3d.hpp"
#include "vertex.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

namespace vpu {

// Virtual Vector Processor Register File (Hardware Emulation)
struct RegisterFile {
    Vec4 v[16];   // Vertex Attribute Input Registers (v0..v15)
    Vec4 c[64];   // Constant / Uniform Registers (c0..c63)
    Vec4 r[16];   // Temporary / Scratch Registers (r0..r15)
    
    // Output Registers
    Vec4 oPos;    // Homogeneous Clip Position
    Vec4 oCol0;   // Primary Color
    Vec4 oNormal; // Normal vector
    Vec4 oTex0;   // Texture coords
};

enum class Opcode {
    MOV,   // dst = src0
    ADD,   // dst = src0 + src1
    SUB,   // dst = src0 - src1
    MUL,   // dst = src0 * src1 (component-wise)
    MAD,   // dst = src0 * src1 + src2
    DP3,   // dst.x = src0.xyz . src1.xyz
    DP4,   // dst.x = src0 . src1 (4D dot product)
    MIN,   // dst = min(src0, src1)
    MAX,   // dst = max(src0, src1)
    SLT,   // dst = (src0 < src1) ? 1.0 : 0.0
    SGE,   // dst = (src0 >= src1) ? 1.0 : 0.0
    RCP,   // dst = 1.0 / src0.x
    RSQ,   // dst = 1.0 / sqrt(abs(src0.x))
    LIT,   // dst = compute lighting coefficients
    END
};

enum class RegType {
    INPUT_V,    // v[index]
    CONST_C,    // c[index]
    TEMP_R,     // r[index]
    OUT_POS,    // oPos
    OUT_COL,    // oCol0
    OUT_NORM,   // oNormal
    OUT_TEX     // oTex0
};

// Swizzle & Masking for vector operands
struct RegRef {
    RegType type{RegType::TEMP_R};
    uint8_t index{0};
    uint8_t swizzle[4]{0, 1, 2, 3}; // 0=x, 1=y, 2=z, 3=w
    bool negate{false};
    uint8_t writeMask{0x0F};         // bit 0=x, 1=y, 2=z, 3=w

    static RegRef V(uint8_t idx) { return {RegType::INPUT_V, idx, {0, 1, 2, 3}, false, 0x0F}; }
    static RegRef C(uint8_t idx) { return {RegType::CONST_C, idx, {0, 1, 2, 3}, false, 0x0F}; }
    static RegRef R(uint8_t idx) { return {RegType::TEMP_R, idx, {0, 1, 2, 3}, false, 0x0F}; }
    static RegRef OPOS() { return {RegType::OUT_POS, 0, {0, 1, 2, 3}, false, 0x0F}; }
    static RegRef OCOL() { return {RegType::OUT_COL, 0, {0, 1, 2, 3}, false, 0x0F}; }
    static RegRef ONORM() { return {RegType::OUT_NORM, 0, {0, 1, 2, 3}, false, 0x0F}; }
    static RegRef OTEX() { return {RegType::OUT_TEX, 0, {0, 1, 2, 3}, false, 0x0F}; }

    // Swizzle helpers
    RegRef xxx() const { return {type, index, {0, 0, 0, 0}, negate, writeMask}; }
    RegRef yyy() const { return {type, index, {1, 1, 1, 1}, negate, writeMask}; }
    RegRef zzz() const { return {type, index, {2, 2, 2, 2}, negate, writeMask}; }
    RegRef www() const { return {type, index, {3, 3, 3, 3}, negate, writeMask}; }
    RegRef xyz() const { return {type, index, {0, 1, 2, 2}, negate, writeMask}; }
    RegRef mask(bool x, bool y, bool z, bool w) const {
        uint8_t m = (x ? 1 : 0) | (y ? 2 : 0) | (z ? 4 : 0) | (w ? 8 : 0);
        RegRef r = *this;
        r.writeMask = m;
        return r;
    }
};

struct Instruction {
    Opcode op{Opcode::END};
    RegRef dst;
    RegRef src0;
    RegRef src1;
    RegRef src2;

    Instruction() = default;
    Instruction(Opcode o, const RegRef& d)
        : op(o), dst(d), src0(), src1(), src2() {}
    Instruction(Opcode o, const RegRef& d, const RegRef& s0)
        : op(o), dst(d), src0(s0), src1(), src2() {}
    Instruction(Opcode o, const RegRef& d, const RegRef& s0, const RegRef& s1)
        : op(o), dst(d), src0(s0), src1(s1), src2() {}
    Instruction(Opcode o, const RegRef& d, const RegRef& s0, const RegRef& s1, const RegRef& s2)
        : op(o), dst(d), src0(s0), src1(s1), src2(s2) {}
};

// Microcode Execution Engine
class MicrocodeVertexProgram {
private:
    std::vector<Instruction> instructions;

    static Vec4 readReg(const RegisterFile& rf, const RegRef& ref) {
        Vec4 base;
        switch (ref.type) {
            case RegType::INPUT_V:  base = rf.v[ref.index & 15]; break;
            case RegType::CONST_C:  base = rf.c[ref.index & 63]; break;
            case RegType::TEMP_R:   base = rf.r[ref.index & 15]; break;
            case RegType::OUT_POS:  base = rf.oPos; break;
            case RegType::OUT_COL:  base = rf.oCol0; break;
            case RegType::OUT_NORM: base = rf.oNormal; break;
            case RegType::OUT_TEX:  base = rf.oTex0; break;
        }
        Vec4 swizzled{
            base[ref.swizzle[0]],
            base[ref.swizzle[1]],
            base[ref.swizzle[2]],
            base[ref.swizzle[3]]
        };
        if (ref.negate) {
            swizzled = -1.0f * swizzled;
        }
        return swizzled;
    }

    static void writeReg(RegisterFile& rf, const RegRef& ref, const Vec4& val) {
        Vec4* target = nullptr;
        switch (ref.type) {
            case RegType::TEMP_R:   target = &rf.r[ref.index & 15]; break;
            case RegType::OUT_POS:  target = &rf.oPos; break;
            case RegType::OUT_COL:  target = &rf.oCol0; break;
            case RegType::OUT_NORM: target = &rf.oNormal; break;
            case RegType::OUT_TEX:  target = &rf.oTex0; break;
            default: return; // Writes to input or constants are disallowed in hardware
        }
        if (ref.writeMask & 1) (*target)[0] = val[0];
        if (ref.writeMask & 2) (*target)[1] = val[1];
        if (ref.writeMask & 4) (*target)[2] = val[2];
        if (ref.writeMask & 8) (*target)[3] = val[3];
    }

public:
    void addInstruction(const Instruction& inst) {
        instructions.push_back(inst);
    }

    void clear() { instructions.clear(); }

    void execute(RegisterFile& rf) const {
        for (const auto& inst : instructions) {
            Vec4 s0 = readReg(rf, inst.src0);
            Vec4 s1 = readReg(rf, inst.src1);
            Vec4 s2 = readReg(rf, inst.src2);
            Vec4 res;

            switch (inst.op) {
                case Opcode::MOV: res = s0; break;
                case Opcode::ADD: res = s0 + s1; break;
                case Opcode::SUB: res = s0 - s1; break;
                case Opcode::MUL: res = s0 * s1; break;
                case Opcode::MAD: res = (s0 * s1) + s2; break;
                case Opcode::DP3: {
                    float d = s0.x * s1.x + s0.y * s1.y + s0.z * s1.z;
                    res = Vec4(d, d, d, d);
                    break;
                }
                case Opcode::DP4: {
                    float d = s0.dot(s1);
                    res = Vec4(d, d, d, d);
                    break;
                }
                case Opcode::MIN: {
                    res = Vec4(std::min(s0.x, s1.x), std::min(s0.y, s1.y),
                               std::min(s0.z, s1.z), std::min(s0.w, s1.w));
                    break;
                }
                case Opcode::MAX: {
                    res = Vec4(std::max(s0.x, s1.x), std::max(s0.y, s1.y),
                               std::max(s0.z, s1.z), std::max(s0.w, s1.w));
                    break;
                }
                case Opcode::SLT: {
                    res = Vec4(s0.x < s1.x ? 1.0f : 0.0f,
                               s0.y < s1.y ? 1.0f : 0.0f,
                               s0.z < s1.z ? 1.0f : 0.0f,
                               s0.w < s1.w ? 1.0f : 0.0f);
                    break;
                }
                case Opcode::SGE: {
                    res = Vec4(s0.x >= s1.x ? 1.0f : 0.0f,
                               s0.y >= s1.y ? 1.0f : 0.0f,
                               s0.z >= s1.z ? 1.0f : 0.0f,
                               s0.w >= s1.w ? 1.0f : 0.0f);
                    break;
                }
                case Opcode::RCP: {
                    float inv = (std::abs(s0.x) > 1e-8f) ? (1.0f / s0.x) : 0.0f;
                    res = Vec4(inv, inv, inv, inv);
                    break;
                }
                case Opcode::RSQ: {
                    float r = (s0.x > 1e-8f) ? (1.0f / std::sqrt(s0.x)) : 0.0f;
                    res = Vec4(r, r, r, r);
                    break;
                }
                case Opcode::LIT: {
                    // Classic GPU LIT instruction:
                    // s0.x = N dot L, s0.y = N dot H, s0.w = specular power
                    // res.x = 1.0 (ambient)
                    // res.y = max(s0.x, 0.0) (diffuse)
                    // res.z = (s0.x > 0 && s0.y > 0) ? (s0.y ^ s0.w) : 0.0 (specular)
                    // res.w = 1.0
                    float n_dot_l = std::max(s0.x, 0.0f);
                    float n_dot_h = std::max(s0.y, 0.0f);
                    float spec = 0.0f;
                    if (s0.x > 0.0f && s0.y > 0.0f) {
                        spec = std::pow(n_dot_h, clamp(s0.w, -128.0f, 128.0f));
                    }
                    res = Vec4(1.0f, n_dot_l, spec, 1.0f);
                    break;
                }
                case Opcode::END:
                    return;
            }

            writeReg(rf, inst.dst, res);
        }
    }
};

} // namespace vpu

#endif // MICROCODE_SHADER_HPP
