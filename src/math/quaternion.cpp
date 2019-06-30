#include <cmath>

struct Vector3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    Vector3 cross(const Vector3 & other) const
    {
        return
        {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        };
    }
    friend Vector3 operator*(float lhs, const Vector3 & rhs)
    {
        return { lhs * rhs.x, lhs * rhs.y, lhs * rhs.z };
    }
    friend Vector3 operator*(const Vector3 & lhs, float rhs)
    {
        return { lhs.x * rhs, lhs.y * rhs, lhs.z * rhs };
    }
    friend Vector3 operator*(const Vector3 & lhs, const Vector3 & rhs)
    {
        return { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
    }
    friend Vector3 operator+(const Vector3 & lhs, const Vector3 & rhs)
    {
        return
        {
            lhs.x + rhs.x,
            lhs.y + rhs.y,
            lhs.z + rhs.z
        };
    }
    friend Vector3 operator-(const Vector3 & lhs, const Vector3 & rhs)
    {
        return
        {
            lhs.x - rhs.x,
            lhs.y - rhs.y,
            lhs.z - rhs.z
        };
    }
    float dot(const Vector3 & other) const
    {
        return x * other.x + y * other.y + z * other.z;
    }
    float length_squared() const
    {
        return dot(*this);
    }
    float length() const
    {
        return std::sqrt(length_squared());
    }
    Vector3 normalized()
    {
        float multiplier = 1.0f / length();
        return
        {
            x * multiplier,
            y * multiplier,
            z * multiplier
        };
    }
};

struct Quaternion
{
    float yz = 0.0f;
    float zx = 0.0f;
    float xy = 0.0f;
    float s = 1.0f;
    float normal_length_squared = 0.0f;

    Quaternion() = default;
    Quaternion(float yz, float zx, float xy, float s)
        : yz(yz), zx(zx), xy(xy), s(s), normal_length_squared(yz * yz + zx * zx + xy * xy)
    {
    }

    static Quaternion FromAxisAngle(const Vector3 & axis, float angle)
    {
        angle *= 0.5f;
        float sin_angle = std::sin(angle);
        float cos_angle = std::cos(angle);
        return
        {
            sin_angle * axis.x,
            sin_angle * axis.y,
            sin_angle * axis.z,
            cos_angle
        };
    }


    friend Quaternion operator*(const Quaternion & lhs, const Quaternion & rhs)
    {
        return
        {
              lhs.xy * rhs.zx + lhs.yz * rhs.s  - lhs.zx * rhs.xy + lhs.s * rhs.yz,
            - lhs.xy * rhs.yz + lhs.yz * rhs.xy + lhs.zx * rhs.s  + lhs.s * rhs.zx,
              lhs.xy * rhs.s  - lhs.yz * rhs.zx + lhs.zx * rhs.yz + lhs.s * rhs.xy,
            - lhs.xy * rhs.xy - lhs.yz * rhs.yz - lhs.zx * rhs.zx + lhs.s * rhs.s
        };
    }

    Vector3 multiply_simple(const Vector3 & rhs)
    {
        // x * xy = y
        // yz * xy = zx
        // x * zx = -z
        // yz * zx = yx = -xy
        // y * xy = -x
        // zx * xy = zy = -yz
        // y * yz = z
        // zx * yz = xy
        // z * yz = -y
        // xy * yz = -zx
        // z * zx = x
        // xy * zx = yz

        // x * xyz = yz
        // y * xyz = -xz = zx
        // z * xyz = xy
        Quaternion as_quaternion{ rhs.x, rhs.y, rhs.z, 0.0f };
        Quaternion multiplied = *this * as_quaternion * -*this;
        return { multiplied.yz, multiplied.zx, multiplied.xy };
    }
    Vector3 multiply_ryg(const Vector3 & rhs)
    {
        Vector3 normal{yz, zx, xy};
        Vector3 t = 2.0f * rhs.cross(normal);
        return rhs + s * t + t.cross(normal);
        //t = 2 * cross(q.xyz, v)
        //v' = v + q.w * t + cross(q.xyz, t)
    }
    Vector3 multiply_half(const Vector3 & rhs)
    {
        // xyxyz = -xxyyz = -z
        // yzxyz = yxy = -x
        // zxxyz = zyz = -y
        Vector3 normal_squared = { yz * yz, zx * zx, xy * xy };
        float normal_length_squared = normal_squared.x + normal_squared.y + normal_squared.z;
        if (normal_length_squared < 0.00000000000001f)
            return rhs;
        Vector3 projected = normal_squared * rhs * (1.0f / normal_length_squared);
        Vector3 on_plane = rhs - projected;
        // xyx = -y
        // yzx = xyz
        // zxx = z
        // xyy = x
        // yzy = -z
        // zxy = xyz
        // xyz = xyz
        // yzz = y
        // zxz = -x
        Vector3 multiplied =
        {
            xy * on_plane.y - zx * on_plane.z + s * on_plane.x,
            yz * on_plane.z - xy * on_plane.x + s * on_plane.y,
            zx * on_plane.x - yz * on_plane.y + s * on_plane.z
        };
        return multiplied + projected;
    }
    Vector3 multiply_half_precalculated(const Vector3 & rhs)
    {
        // xyxyz = -xxyyz = -z
        // yzxyz = yxy = -x
        // zxxyz = zyz = -y
        if (normal_length_squared < 0.00000000000001f)
            return rhs;
        Vector3 normal = { yz, zx, xy };
        float normal_dot = normal.dot(rhs);
        Vector3 projected = normal * (normal_dot / normal_length_squared);
        Vector3 on_plane = rhs - projected;
        // xyx = -y
        // yzx = xyz
        // zxx = z
        // xyy = x
        // yzy = -z
        // zxy = xyz
        // xyz = xyz
        // yzz = y
        // zxz = -x
        Vector3 multiplied =
        {
            xy * on_plane.y - zx * on_plane.z + s * on_plane.x,
            yz * on_plane.z - xy * on_plane.x + s * on_plane.y,
            zx * on_plane.x - yz * on_plane.y + s * on_plane.z
        };
        return multiplied + projected;
    }
    Quaternion operator-() const
    {
        return
        {
            -yz,
            -zx,
            -xy,
            s
        };
    }
};


#ifndef DISABLE_GTEST
#include "test/include_test.hpp"

TEST(quaternion, ninety_degrees)
{
    constexpr float pi = 3.1415926535897932384626433f;
    Quaternion ninety_degrees = Quaternion::FromAxisAngle({ 0.0f, 1.0f, 0.0f }, 0.5f * pi);
    Vector3 x {1.0f, 0.0f, 0.0f};
    Vector3 rotated_simple = ninety_degrees.multiply_simple(x);
    ASSERT_NEAR(0.0f, rotated_simple.x, 0.0000001f);
    ASSERT_NEAR(0.0f, rotated_simple.y, 0.0000001f);
    ASSERT_NEAR(1.0f, rotated_simple.z, 0.0000001f);
    Vector3 rotated_ryg = ninety_degrees.multiply_ryg(x);
    ASSERT_NEAR(0.0f, rotated_ryg.x, 0.0000001f);
    ASSERT_NEAR(0.0f, rotated_ryg.y, 0.0000001f);
    ASSERT_NEAR(1.0f, rotated_ryg.z, 0.0000001f);
    Vector3 rotated_half = ninety_degrees.multiply_half(x);
    Vector3 rotated_full = ninety_degrees.multiply_half(rotated_half);
    ASSERT_NEAR(0.0f, rotated_full.x, 0.0000001f);
    ASSERT_NEAR(0.0f, rotated_full.y, 0.0000001f);
    ASSERT_NEAR(1.0f, rotated_full.z, 0.0000001f);
    Vector3 rotated_half_precalculated = ninety_degrees.multiply_half_precalculated(x);
    Vector3 rotated_full_precalculated = ninety_degrees.multiply_half_precalculated(rotated_half_precalculated);
    ASSERT_NEAR(0.0f, rotated_full_precalculated.x, 0.0000001f);
    ASSERT_NEAR(0.0f, rotated_full_precalculated.y, 0.0000001f);
    ASSERT_NEAR(1.0f, rotated_full_precalculated.z, 0.0000001f);
}
TEST(quaternion, forty_five_degrees)
{
    constexpr float pi = 3.1415926535897932384626433f;
    Quaternion ninety_degrees = Quaternion::FromAxisAngle({ 0.0f, 0.0f, 1.0f }, 0.25f * pi);
    Vector3 x {1.0f, 0.0f, 0.0f};
    Vector3 rotated_simple = ninety_degrees.multiply_simple(x);
    float expected = 1.0f / std::sqrt(2.0f);
    ASSERT_NEAR(expected, rotated_simple.x, 0.0000001f);
    ASSERT_NEAR(-expected, rotated_simple.y, 0.0000001f);
    ASSERT_NEAR(0.0f, rotated_simple.z, 0.0000001f);
    Vector3 rotated_ryg = ninety_degrees.multiply_ryg(x);
    ASSERT_NEAR(expected, rotated_ryg.x, 0.0000001f);
    ASSERT_NEAR(-expected, rotated_ryg.y, 0.0000001f);
    ASSERT_NEAR(0.0f, rotated_ryg.z, 0.0000001f);
    Vector3 rotated_half = ninety_degrees.multiply_half(x);
    Vector3 rotated_full = ninety_degrees.multiply_half(rotated_half);
    ASSERT_NEAR(expected, rotated_full.x, 0.0000001f);
    ASSERT_NEAR(-expected, rotated_full.y, 0.0000001f);
    ASSERT_NEAR(0.0f, rotated_full.z, 0.0000001f);
    Vector3 rotated_half_precalculated = ninety_degrees.multiply_half_precalculated(x);
    Vector3 rotated_full_precalculated = ninety_degrees.multiply_half_precalculated(rotated_half_precalculated);
    ASSERT_NEAR(expected, rotated_full_precalculated.x, 0.0000001f);
    ASSERT_NEAR(-expected, rotated_full_precalculated.y, 0.0000001f);
    ASSERT_NEAR(0.0f, rotated_full_precalculated.z, 0.0000001f);
}

#include <benchmark/benchmark.h>
void benchmark_simple_quaternion_multiplication(benchmark::State & state)
{
    Quaternion rotor = Quaternion::FromAxisAngle(Vector3{ 10.0f, 1.0f, -100.0f }.normalized(), 0.1f);
    Vector3 to_rotate{1.0f, 100.0f, -2.0f};
    while (state.KeepRunning())
    {
        to_rotate = rotor.multiply_simple(to_rotate);
        benchmark::DoNotOptimize(to_rotate);
    }
}
void benchmark_ryg_quaternion_multiplication(benchmark::State & state)
{
    Quaternion rotor = Quaternion::FromAxisAngle(Vector3{ 10.0f, 1.0f, -100.0f }.normalized(), 0.1f);
    Vector3 to_rotate{1.0f, 100.0f, -2.0f};
    while (state.KeepRunning())
    {
        to_rotate = rotor.multiply_ryg(to_rotate);
        benchmark::DoNotOptimize(to_rotate);
    }
}
void benchmark_half_quaternion_multiplication(benchmark::State & state)
{
    Quaternion rotor = Quaternion::FromAxisAngle(Vector3{ 10.0f, 1.0f, -100.0f }.normalized(), 0.1f);
    Vector3 to_rotate{1.0f, 100.0f, -2.0f};
    while (state.KeepRunning())
    {
        to_rotate = rotor.multiply_half(to_rotate);
        benchmark::DoNotOptimize(to_rotate);
    }
}
void benchmark_half_precalculated_quaternion_multiplication(benchmark::State & state)
{
    Quaternion rotor = Quaternion::FromAxisAngle(Vector3{ 10.0f, 1.0f, -100.0f }.normalized(), 0.1f);
    Vector3 to_rotate{1.0f, 100.0f, -2.0f};
    while (state.KeepRunning())
    {
        to_rotate = rotor.multiply_half_precalculated(to_rotate);
        benchmark::DoNotOptimize(to_rotate);
    }
}
BENCHMARK(benchmark_simple_quaternion_multiplication);
BENCHMARK(benchmark_ryg_quaternion_multiplication);
BENCHMARK(benchmark_half_quaternion_multiplication);
BENCHMARK(benchmark_half_precalculated_quaternion_multiplication);

#endif


