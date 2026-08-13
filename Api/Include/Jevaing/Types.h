#pragma once

#include <cmath>

namespace Jevaing
{
    inline constexpr float Pi = 3.14159265358979323846f;

    struct Vec2
    {
        float X = 0.0f;
        float Y = 0.0f;
    };

    inline Vec2 operator+(const Vec2& left, const Vec2& right)
    {
        return { left.X + right.X, left.Y + right.Y };
    }

    inline Vec2 operator-(const Vec2& left, const Vec2& right)
    {
        return { left.X - right.X, left.Y - right.Y };
    }

    struct Vec3
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;
    };

    struct Color
    {
        float R = 1.0f;
        float G = 1.0f;
        float B = 1.0f;
        float A = 1.0f;
    };

    inline Vec3 operator+(const Vec3& left, const Vec3& right)
    {
        return { left.X + right.X, left.Y + right.Y, left.Z + right.Z };
    }

    inline Vec3 operator-(const Vec3& left, const Vec3& right)
    {
        return { left.X - right.X, left.Y - right.Y, left.Z - right.Z };
    }

    inline Vec3 operator*(const Vec3& value, float scale)
    {
        return { value.X * scale, value.Y * scale, value.Z * scale };
    }

    inline float Dot(const Vec3& left, const Vec3& right)
    {
        return left.X * right.X + left.Y * right.Y + left.Z * right.Z;
    }

    inline Vec3 Cross(const Vec3& left, const Vec3& right)
    {
        return {
            left.Y * right.Z - left.Z * right.Y,
            left.Z * right.X - left.X * right.Z,
            left.X * right.Y - left.Y * right.X
        };
    }

    inline float Length(const Vec3& value)
    {
        return std::sqrt(Dot(value, value));
    }

    inline Vec3 Normalize(const Vec3& value)
    {
        const float length = Length(value);

        if (length <= 0.000001f)
        {
            return {};
        }

        return value * (1.0f / length);
    }

    struct Mat4
    {
        float M[4][4] = {};

        static Mat4 Identity()
        {
            Mat4 result = {};
            result.M[0][0] = 1.0f;
            result.M[1][1] = 1.0f;
            result.M[2][2] = 1.0f;
            result.M[3][3] = 1.0f;
            return result;
        }

        static Mat4 Scale(const Vec3& scale)
        {
            Mat4 result = Identity();
            result.M[0][0] = scale.X;
            result.M[1][1] = scale.Y;
            result.M[2][2] = scale.Z;
            return result;
        }

        static Mat4 Translation(const Vec3& translation)
        {
            Mat4 result = Identity();
            result.M[3][0] = translation.X;
            result.M[3][1] = translation.Y;
            result.M[3][2] = translation.Z;
            return result;
        }

        static Mat4 RotationX(float radians)
        {
            Mat4 result = Identity();
            const float sine = std::sin(radians);
            const float cosine = std::cos(radians);

            result.M[1][1] = cosine;
            result.M[1][2] = sine;
            result.M[2][1] = -sine;
            result.M[2][2] = cosine;
            return result;
        }

        static Mat4 RotationY(float radians)
        {
            Mat4 result = Identity();
            const float sine = std::sin(radians);
            const float cosine = std::cos(radians);

            result.M[0][0] = cosine;
            result.M[0][2] = -sine;
            result.M[2][0] = sine;
            result.M[2][2] = cosine;
            return result;
        }

        static Mat4 RotationZ(float radians)
        {
            Mat4 result = Identity();
            const float sine = std::sin(radians);
            const float cosine = std::cos(radians);

            result.M[0][0] = cosine;
            result.M[0][1] = sine;
            result.M[1][0] = -sine;
            result.M[1][1] = cosine;
            return result;
        }

        static Mat4 Perspective(
            float verticalFovRadians,
            float aspectRatio,
            float nearPlane,
            float farPlane
        )
        {
            Mat4 result = {};
            const float yScale = 1.0f / std::tan(verticalFovRadians * 0.5f);
            const float xScale = yScale / aspectRatio;
            const float depth = farPlane - nearPlane;

            result.M[0][0] = xScale;
            result.M[1][1] = yScale;
            result.M[2][2] = farPlane / depth;
            result.M[2][3] = 1.0f;
            result.M[3][2] = -(nearPlane * farPlane) / depth;
            return result;
        }

        static Mat4 LookAt(
            const Vec3& eye,
            const Vec3& target,
            const Vec3& up
        )
        {
            const Vec3 forward = Normalize(target - eye);
            const Vec3 right = Normalize(Cross(up, forward));
            const Vec3 cameraUp = Cross(forward, right);

            Mat4 result = Identity();
            result.M[0][0] = right.X;
            result.M[1][0] = right.Y;
            result.M[2][0] = right.Z;
            result.M[0][1] = cameraUp.X;
            result.M[1][1] = cameraUp.Y;
            result.M[2][1] = cameraUp.Z;
            result.M[0][2] = forward.X;
            result.M[1][2] = forward.Y;
            result.M[2][2] = forward.Z;
            result.M[3][0] = -Dot(right, eye);
            result.M[3][1] = -Dot(cameraUp, eye);
            result.M[3][2] = -Dot(forward, eye);
            return result;
        }
    };

    inline Mat4 operator*(const Mat4& left, const Mat4& right)
    {
        Mat4 result = {};

        for (int row = 0; row < 4; ++row)
        {
            for (int column = 0; column < 4; ++column)
            {
                result.M[row][column] =
                    left.M[row][0] * right.M[0][column] +
                    left.M[row][1] * right.M[1][column] +
                    left.M[row][2] * right.M[2][column] +
                    left.M[row][3] * right.M[3][column];
            }
        }

        return result;
    }

    struct Transform
    {
        Vec3 Position = {};
        Vec3 Rotation = {};
        Vec3 Scale = { 1.0f, 1.0f, 1.0f };

        Mat4 ToMatrix() const
        {
            return
                Mat4::Scale(Scale) *
                Mat4::RotationX(Rotation.X) *
                Mat4::RotationY(Rotation.Y) *
                Mat4::RotationZ(Rotation.Z) *
                Mat4::Translation(Position);
        }
    };

    struct PerspectiveCamera
    {
        Vec3 Position = { 0.0f, 0.0f, -5.0f };
        Vec3 Target = { 0.0f, 0.0f, 0.0f };
        Vec3 Up = { 0.0f, 1.0f, 0.0f };
        float VerticalFovRadians = Pi / 3.0f;
        float AspectRatio = 16.0f / 9.0f;
        float NearPlane = 0.1f;
        float FarPlane = 100.0f;

        Mat4 GetViewMatrix() const
        {
            return Mat4::LookAt(Position, Target, Up);
        }

        Mat4 GetProjectionMatrix() const
        {
            return Mat4::Perspective(
                VerticalFovRadians,
                AspectRatio,
                NearPlane,
                FarPlane
            );
        }
    };
}
