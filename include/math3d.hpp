#ifndef MATH3D_HPP
#define MATH3D_HPP

#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace vpu {

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG2RAD = PI / 180.0f;
constexpr float RAD2DEG = 180.0f / PI;

template <typename T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
    return (v < lo) ? lo : ((hi < v) ? hi : v);
}

// Vec2: 2D Vector
struct Vec2 {
    float x{0.0f}, y{0.0f};

    Vec2() = default;
    constexpr Vec2(float x_, float y_) : x(x_), y(y_) {}

    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { float inv = 1.0f / s; return {x * inv, y * inv}; }
    Vec2 operator*(const Vec2& o) const { return {x * o.x, y * o.y}; }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s) { x *= s; y *= s; return *this; }
    Vec2& operator/=(float s) { float inv = 1.0f / s; x *= inv; y *= inv; return *this; }

    float dot(const Vec2& o) const { return x * o.x + y * o.y; }
    float lengthSq() const { return x * x + y * y; }
    float length() const { return std::sqrt(lengthSq()); }

    Vec2 normalized() const {
        float len = length();
        return (len > 1e-6f) ? (*this / len) : Vec2{0.0f, 0.0f};
    }
};

inline Vec2 operator*(float s, const Vec2& v) { return v * s; }

// Vec3: 3D Vector
struct Vec3 {
    float x{0.0f}, y{0.0f}, z{0.0f};

    Vec3() = default;
    constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { float inv = 1.0f / s; return {x * inv, y * inv, z * inv}; }
    Vec3 operator*(const Vec3& o) const { return {x * o.x, y * o.y, z * o.z}; }

    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    Vec3& operator/=(float s) { float inv = 1.0f / s; x *= inv; y *= inv; z *= inv; return *this; }

    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }

    Vec3 cross(const Vec3& o) const {
        return {
            y * o.z - z * o.y,
            z * o.x - x * o.z,
            x * o.y - y * o.x
        };
    }

    float lengthSq() const { return x * x + y * y + z * z; }
    float length() const { return std::sqrt(lengthSq()); }

    Vec3 normalized() const {
        float len = length();
        return (len > 1e-6f) ? (*this / len) : Vec3{0.0f, 0.0f, 0.0f};
    }
};

inline Vec3 operator*(float s, const Vec3& v) { return v * s; }

// Vec4: 4D Homogeneous Vector
struct Vec4 {
    float x{0.0f}, y{0.0f}, z{0.0f}, w{1.0f};

    Vec4() = default;
    constexpr Vec4(float x_, float y_, float z_, float w_ = 1.0f) : x(x_), y(y_), z(z_), w(w_) {}
    constexpr Vec4(const Vec3& v, float w_ = 1.0f) : x(v.x), y(v.y), z(v.z), w(w_) {}

    float& operator[](int i) { return (&x)[i]; }
    const float& operator[](int i) const { return (&x)[i]; }

    Vec3 xyz() const { return {x, y, z}; }

    Vec4 operator+(const Vec4& o) const { return {x + o.x, y + o.y, z + o.z, w + o.w}; }
    Vec4 operator-(const Vec4& o) const { return {x - o.x, y - o.y, z - o.z, w - o.w}; }
    Vec4 operator*(float s) const { return {x * s, y * s, z * s, w * s}; }
    Vec4 operator/(float s) const { float inv = 1.0f / s; return {x * inv, y * inv, z * inv, w * inv}; }
    Vec4 operator*(const Vec4& o) const { return {x * o.x, y * o.y, z * o.z, w * o.w}; }

    Vec4& operator+=(const Vec4& o) { x += o.x; y += o.y; z += o.z; w += o.w; return *this; }
    Vec4& operator-=(const Vec4& o) { x -= o.x; y -= o.y; z -= o.z; w -= o.w; return *this; }
    Vec4& operator*=(float s) { x *= s; y *= s; z *= s; w *= s; return *this; }
    Vec4& operator/=(float s) { float inv = 1.0f / s; x *= inv; y *= inv; z *= inv; w *= inv; return *this; }

    float dot(const Vec4& o) const { return x * o.x + y * o.y + z * o.z + w * o.w; }
};

inline Vec4 operator*(float s, const Vec4& v) { return v * s; }

// Mat4: 4x4 Row-Major Transformation Matrix
struct Mat4 {
    float m[4][4]{};

    Mat4() { setZero(); }

    void setZero() {
        for (int r = 0; r < 4; ++r)
            for (int c = 0; c < 4; ++c)
                m[r][c] = 0.0f;
    }

    static Mat4 identity() {
        Mat4 res;
        res.m[0][0] = 1.0f; res.m[1][1] = 1.0f;
        res.m[2][2] = 1.0f; res.m[3][3] = 1.0f;
        return res;
    }

    float* operator[](int row) { return m[row]; }
    const float* operator[](int row) const { return m[row]; }

    Mat4 operator*(const Mat4& o) const {
        Mat4 res;
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                res.m[r][c] = m[r][0] * o.m[0][c] +
                              m[r][1] * o.m[1][c] +
                              m[r][2] * o.m[2][c] +
                              m[r][3] * o.m[3][c];
            }
        }
        return res;
    }

    Vec4 operator*(const Vec4& v) const {
        return {
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w,
            m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w
        };
    }

    // Transform a 3D position vector (assumes w = 1)
    Vec3 transformPoint(const Vec3& p) const {
        Vec4 res = (*this) * Vec4(p, 1.0f);
        if (std::abs(res.w) > 1e-7f) {
            float invW = 1.0f / res.w;
            return {res.x * invW, res.y * invW, res.z * invW};
        }
        return res.xyz();
    }

    // Transform a 3D direction vector (assumes w = 0)
    Vec3 transformVector(const Vec3& v) const {
        return {
            m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
            m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
            m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z
        };
    }

    Mat4 transposed() const {
        Mat4 res;
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                res.m[r][c] = m[c][r];
            }
        }
        return res;
    }

    // Exact analytical 4x4 matrix inversion using adjugate / cofactor expansion
    Mat4 inverse() const {
        Mat4 inv;
        const float* a = &m[0][0];

        inv.m[0][0] = a[5]  * a[10] * a[15] - 
                      a[5]  * a[11] * a[14] - 
                      a[9]  * a[6]  * a[15] + 
                      a[9]  * a[7]  * a[14] +
                      a[13] * a[6]  * a[11] - 
                      a[13] * a[7]  * a[10];

        inv.m[1][0] = -a[4]  * a[10] * a[15] + 
                       a[4]  * a[11] * a[14] + 
                       a[8]  * a[6]  * a[15] - 
                       a[8]  * a[7]  * a[14] - 
                       a[12] * a[6]  * a[11] + 
                       a[12] * a[7]  * a[10];

        inv.m[2][0] = a[4]  * a[9] * a[15] - 
                      a[4]  * a[11] * a[13] - 
                      a[8]  * a[5] * a[15] + 
                      a[8]  * a[7] * a[13] + 
                      a[12] * a[5] * a[11] - 
                      a[12] * a[7] * a[9];

        inv.m[3][0] = -a[4]  * a[9] * a[14] + 
                       a[4]  * a[10] * a[13] +
                       a[8]  * a[5] * a[14] - 
                       a[8]  * a[6] * a[13] - 
                       a[12] * a[5] * a[10] + 
                       a[12] * a[6] * a[9];

        inv.m[0][1] = -a[1]  * a[10] * a[15] + 
                       a[1]  * a[11] * a[14] + 
                       a[9]  * a[2] * a[15] - 
                       a[9]  * a[3] * a[14] - 
                       a[13] * a[2] * a[11] + 
                       a[13] * a[3] * a[10];

        inv.m[1][1] = a[0]  * a[10] * a[15] - 
                      a[0]  * a[11] * a[14] - 
                      a[8]  * a[2] * a[15] + 
                      a[8]  * a[3] * a[14] + 
                      a[12] * a[2] * a[11] - 
                      a[12] * a[3] * a[10];

        inv.m[2][1] = -a[0]  * a[9] * a[15] + 
                       a[0]  * a[11] * a[13] + 
                       a[8]  * a[1] * a[15] - 
                       a[8]  * a[3] * a[13] - 
                       a[12] * a[1] * a[11] + 
                       a[12] * a[3] * a[9];

        inv.m[3][1] = a[0]  * a[9] * a[14] - 
                      a[0]  * a[10] * a[13] - 
                      a[8]  * a[1] * a[14] + 
                      a[8]  * a[2] * a[13] + 
                      a[12] * a[1] * a[10] - 
                      a[12] * a[2] * a[9];

        inv.m[0][2] = a[1]  * a[6] * a[15] - 
                      a[1]  * a[7] * a[14] - 
                      a[5]  * a[2] * a[15] + 
                      a[5]  * a[3] * a[14] + 
                      a[13] * a[2] * a[7] - 
                      a[13] * a[3] * a[6];

        inv.m[1][2] = -a[0]  * a[6] * a[15] + 
                       a[0]  * a[7] * a[14] + 
                       a[4]  * a[2] * a[15] - 
                       a[4]  * a[3] * a[14] - 
                       a[12] * a[2] * a[7] + 
                       a[12] * a[3] * a[6];

        inv.m[2][2] = a[0]  * a[5] * a[15] - 
                      a[0]  * a[7] * a[13] - 
                      a[4]  * a[1] * a[15] + 
                      a[4]  * a[3] * a[13] + 
                      a[12] * a[1] * a[7] - 
                      a[12] * a[3] * a[5];

        inv.m[3][2] = -a[0]  * a[5] * a[14] + 
                       a[0]  * a[6] * a[13] + 
                       a[4]  * a[1] * a[14] - 
                       a[4]  * a[2] * a[13] - 
                       a[12] * a[1] * a[6] + 
                       a[12] * a[2] * a[5];

        inv.m[0][3] = -a[1] * a[6] * a[11] + 
                       a[1] * a[7] * a[10] + 
                       a[5] * a[2] * a[11] - 
                       a[5] * a[3] * a[10] - 
                       a[9] * a[2] * a[7] + 
                       a[9] * a[3] * a[6];

        inv.m[1][3] = a[0] * a[6] * a[11] - 
                      a[0] * a[7] * a[10] - 
                      a[4] * a[2] * a[11] + 
                      a[4] * a[3] * a[10] + 
                      a[8] * a[2] * a[7] - 
                      a[8] * a[3] * a[6];

        inv.m[2][3] = -a[0] * a[5] * a[11] + 
                       a[0] * a[7] * a[9] + 
                       a[4] * a[1] * a[11] - 
                       a[4] * a[3] * a[9] - 
                       a[8] * a[1] * a[7] + 
                       a[8] * a[3] * a[5];

        inv.m[3][3] = a[0] * a[5] * a[10] - 
                      a[0] * a[6] * a[9] - 
                      a[4] * a[1] * a[10] + 
                      a[4] * a[2] * a[9] + 
                      a[8] * a[1] * a[6] - 
                      a[8] * a[2] * a[5];

        float det = a[0] * inv.m[0][0] + a[1] * inv.m[1][0] + a[2] * inv.m[2][0] + a[3] * inv.m[3][0];
        if (std::abs(det) < 1e-8f) {
            return Mat4::identity(); // Degenerate fallback
        }

        float invDet = 1.0f / det;
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                inv.m[r][c] *= invDet;
            }
        }
        return inv;
    }

    // Transformations
    static Mat4 translation(const Vec3& t) {
        Mat4 res = identity();
        res.m[0][3] = t.x;
        res.m[1][3] = t.y;
        res.m[2][3] = t.z;
        return res;
    }

    static Mat4 scale(const Vec3& s) {
        Mat4 res;
        res.m[0][0] = s.x;
        res.m[1][1] = s.y;
        res.m[2][2] = s.z;
        res.m[3][3] = 1.0f;
        return res;
    }

    static Mat4 rotationX(float radians) {
        Mat4 res = identity();
        float c = std::cos(radians);
        float s = std::sin(radians);
        res.m[1][1] = c;  res.m[1][2] = -s;
        res.m[2][1] = s;  res.m[2][2] = c;
        return res;
    }

    static Mat4 rotationY(float radians) {
        Mat4 res = identity();
        float c = std::cos(radians);
        float s = std::sin(radians);
        res.m[0][0] = c;  res.m[0][2] = s;
        res.m[2][0] = -s; res.m[2][2] = c;
        return res;
    }

    static Mat4 rotationZ(float radians) {
        Mat4 res = identity();
        float c = std::cos(radians);
        float s = std::sin(radians);
        res.m[0][0] = c;  res.m[0][1] = -s;
        res.m[1][0] = s;  res.m[1][1] = c;
        return res;
    }

    static Mat4 rotationAxis(const Vec3& axis, float radians) {
        Mat4 res = identity();
        Vec3 a = axis.normalized();
        float c = std::cos(radians);
        float s = std::sin(radians);
        float t = 1.0f - c;

        res.m[0][0] = t * a.x * a.x + c;
        res.m[0][1] = t * a.x * a.y - s * a.z;
        res.m[0][2] = t * a.x * a.z + s * a.y;

        res.m[1][0] = t * a.x * a.y + s * a.z;
        res.m[1][1] = t * a.y * a.y + c;
        res.m[1][2] = t * a.y * a.z - s * a.x;

        res.m[2][0] = t * a.x * a.z - s * a.y;
        res.m[2][1] = t * a.y * a.z + s * a.x;
        res.m[2][2] = t * a.z * a.z + c;

        return res;
    }

    // Camera / View matrix
    static Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
        Vec3 f = (target - eye).normalized();   // Forward (+Z in RH, look direction)
        Vec3 r = f.cross(up).normalized();       // Right
        Vec3 u = r.cross(f);                     // Up orthogonal

        Mat4 res = identity();
        res.m[0][0] = r.x;  res.m[0][1] = r.y;  res.m[0][2] = r.z;  res.m[0][3] = -r.dot(eye);
        res.m[1][0] = u.x;  res.m[1][1] = u.y;  res.m[1][2] = u.z;  res.m[1][3] = -u.dot(eye);
        res.m[2][0] = -f.x; res.m[2][1] = -f.y; res.m[2][2] = -f.z; res.m[2][3] = f.dot(eye); // Camera looks down -Z
        return res;
    }

    // Perspective Projection matrix (maps frustum to standard NDC [-1, 1]^3)
    static Mat4 perspective(float fovRadians, float aspect, float zNear, float zFar) {
        Mat4 res;
        float tanHalfFov = std::tan(fovRadians * 0.5f);
        res.m[0][0] = 1.0f / (aspect * tanHalfFov);
        res.m[1][1] = 1.0f / tanHalfFov;
        res.m[2][2] = -(zFar + zNear) / (zFar - zNear);
        res.m[2][3] = -(2.0f * zFar * zNear) / (zFar - zNear);
        res.m[3][2] = -1.0f;
        res.m[3][3] = 0.0f;
        return res;
    }

    // Orthographic Projection matrix
    static Mat4 orthographic(float left, float right, float bottom, float top, float zNear, float zFar) {
        Mat4 res = identity();
        res.m[0][0] = 2.0f / (right - left);
        res.m[1][1] = 2.0f / (top - bottom);
        res.m[2][2] = -2.0f / (zFar - zNear);
        res.m[0][3] = -(right + left) / (right - left);
        res.m[1][3] = -(top + bottom) / (top - bottom);
        res.m[2][3] = -(zFar + zNear) / (zFar - zNear);
        return res;
    }
};

} // namespace vpu

#endif // MATH3D_HPP
