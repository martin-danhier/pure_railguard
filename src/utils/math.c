#include "railguard/utils/math.h"

#include <math.h>

// --=== General math ===--

double rg_cos(double radians)
{
    // Avoid floating point errors
    if (radians == 0.0)
    {
        return 1.0;
    }
    else if (radians == RG_PI)
    {
        return -1.0;
    }
    else if (radians == RG_PI_2 || radians == RG_3PI_2)
    {
        return 0;
    }
    else
    {
        return cos(radians);
    }
}
double rg_sin(double radians)
{
    // Avoid floating point errors
    if (radians == 0.0 || radians == RG_PI)
    {
        return 0.0;
    }
    else if (radians == RG_PI_2)
    {
        return 1.0;
    }
    else if (radians == RG_3PI_2)
    {
        return -1.0;
    }
    else
    {
        return sin(radians);
    }
}
double rg_sqrt(double x)
{
    return sqrt(x);
}
float rg_cosf(float radians)
{
    return cosf(radians);
}
float rg_sinf(float radians)
{
    return sinf(radians);
}
float rg_sqrtf(float x)
{
    return sqrtf(x);
}

// --=== Mat4 ===--

// region Mat4

rg_mat4 rg_mat4_mul(const rg_mat4 a, const rg_mat4 b)
{
    rg_mat4 result;

    // Rows
    for (int i = 0; i < 4; i++)
    {
        // Columns
        for (int j = 0; j < 4; j++)
        {
            // Sum the products of the row and column
            result.m[i][j] = 0;
            for (int k = 0; k < 4; k++)
            {
                result.m[i][j] += a.m[i][k] * b.m[k][j];
            }
        }
    }
    return result;
}

rg_mat4 rg_mat4_ortho(const float left, const float right, const float bottom, const float top, const float near, const float far)
{
    rg_mat4 m = rg_mat4_identity();

    m.m00 = 2 / (right - left);
    m.m11 = 2 / (top - bottom);
    m.m22 = (-2) / (far - near);

    m.m03 = -((right + left) / (right - left));
    m.m13 = -((top + bottom) / (top - bottom));
    m.m23 = -((far + near) / (far - near));

    return m;
}

rg_mat4 rg_mat4_perspective(const float fov, const float aspect, const float near, const float far)
{
    rg_mat4 result = rg_mat4_identity();

    float f     = 1.0f / tanf(fov * 0.5f);
    float range = near - far;

    result.m00 = f / aspect;
    result.m11 = f;
    result.m22 = (near + far) / range;
    result.m23 = 2.0f * near * far / range;
    result.m32 = -1.0f;

    return result;
}

// endregion

// --=== Quat ===--

// region Quat




rg_quat rg_quat_mul(rg_quat a, rg_quat b)
{
    // Use Hamilton product formula
    // Simply take:
    // (a1 + b1 * i + c1 * j + d1 * k) *  (a2 + b2 * i + c2 * j + d2 * k)
    // Distribute that -> sum of 16 products
    // Then apply quaternions rules to simplify i * j = k, etc.
    return (rg_quat) {
        .a = (a.a * b.a) - (a.b * b.b) - (a.c * b.c) - (a.d * b.d),
        .b = (a.a * b.b) + (a.b * b.a) + (a.c * b.d) - (a.d * b.c),
        .c = (a.a * b.c) - (a.b * b.d) + (a.c * b.a) + (a.d * b.b),
        .d = (a.a * b.d) + (a.b * b.c) - (a.c * b.b) + (a.d * b.a),
    };
}

rg_quat rg_quat_cross(rg_quat a, rg_quat b)
{
    return (rg_quat) {
        .a = 0,
        .b = (a.c * b.d) - (a.d * b.c),
        .c = (a.d * b.b) - (a.b * b.d),
        .d = (a.b * b.c) - (a.c * b.b),
    };
}

rg_mat4 rg_quat_to_rotation_matrix(rg_quat quat)
{
    rg_mat4 result = rg_mat4_identity();

    // Normalize quaternion
    quat = rg_quat_normalize(quat);

    // Precalculate values
    const float ii = quat.i * quat.i;
    const float jj = quat.j * quat.j;
    const float kk = quat.k * quat.k;
    const float ij = quat.i * quat.j;
    const float ik = quat.i * quat.k;
    const float jk = quat.j * quat.k;
    const float kr = quat.k * quat.r;
    const float ir = quat.i * quat.r;
    const float jr = quat.j * quat.r;

    // Save in matrix
    result.m00 = 1 - 2 * (jj - kk);
    result.m01 = 2 * (ij + kr);
    result.m02 = 2 * (ik - jr);

    result.m10 = 2 * (ij - kr);
    result.m11 = 1 - 2 * (ii - kk);
    result.m12 = 2 * (jk + ir);

    result.m20 = 2 * (ik + jr);
    result.m21 = 2 * (jk - ir);
    result.m22 = 1 - 2 * (ii - jj);

    return result;
}

rg_vec3 rg_quat_rotate_point(rg_quat quat, rg_vec3 point)
{
    rg_quat point_quat = (rg_quat) {
        0,
        point.x,
        point.y,
        point.z,
    };

    // P' = q * P * q^-1
    point_quat = rg_quat_mul(quat, point_quat);
    point_quat = rg_quat_mul(point_quat, rg_quat_reciprocal(quat));

    return (rg_vec3) {
        point_quat.b,
        point_quat.c,
        point_quat.d,
    };
}

rg_quat rg_quat_from_axis_angle(rg_vec3 axis, double angle)
{
    const float s = (float) rg_sin(angle / 2);
    return (rg_quat) {
        .i = axis.x * s,
        .j = axis.y * s,
        .k = axis.z * s,
        .r = (float) rg_cos(angle / 2),
    };
}

rg_quat rg_quat_from_euler(float yaw, float pitch, float roll)
{
    const float cos_y = cosf(yaw * 0.5f);
    const float sin_y = sinf(yaw * 0.5f);
    const float cos_p = cosf(pitch - 0.5f);
    const float sin_p = sinf(pitch - 0.5f);
    const float cos_r = cosf(roll - 0.5f);
    const float sin_r = sinf(roll - 0.5f);

    return (rg_quat) {
        .r = cos_r * cos_p * cos_y + sin_r * sin_p * sin_y,
        .i = sin_r * cos_p * cos_y - cos_r * sin_p * sin_y,
        .j = cos_r * sin_p * cos_y + sin_r * cos_p * sin_y,
        .k = cos_r * cos_p * sin_y - sin_r * sin_p * cos_y,
    };
}

// endregion
