#pragma once

#include <stdbool.h>

#define RG_PI 3.14159265358979323846
#define RG_PI_2 1.57079632679489661923
#define RG_3PI_2 4.71238898038468985769
#define RG_2PI 6.28318530717958647692

// --=== Types ===--

typedef union rg_vec2
{
    struct
    {
        float x, y;
    };
    float v[2];
} rg_vec2;

typedef union rg_vec3
{
    struct
    {
        float x, y, z;
    };
    struct
    {
        float r, g, b;
    };
    float v[3];
} rg_vec3;

typedef union rg_vec4
{
    struct
    {
        float x, y, z, w;
    };
    struct
    {
        float r, g, b, a;
    };
    float v[4];
} rg_vec4;

typedef union rg_mat4
{
    struct
    {
        float m00, m01, m02, m03;
        float m10, m11, m12, m13;
        float m20, m21, m22, m23;
        float m30, m31, m32, m33;
    };
    float m[4][4];
} rg_mat4;

/**
 * Quaternion of the form
 * a + bi + cj + dk
 * or
 * w + xi + yj + zk
 */
typedef union rg_quat
{
    struct
    {
        float a, b, c, d;
    };
    struct
    {
        float r, i, j, k;
    };
    struct
    {
        float w, x, y, z;
    };
    float v[4];
} rg_quat;

// --=== Functions ===--

// General math
double              rg_cos(double radians);
double              rg_sin(double radians);
double              rg_sqrt(double x);
float               rg_cosf(float radians);
float               rg_sinf(float radians);
float               rg_sqrtf(float x);
static inline float rg_radians(float degrees)
{
    return degrees * (float) RG_PI / 180.0f;
}


// Vec 2

// --=== Vec2 ===--

// region Vec2

static inline rg_vec2 rg_vec2_add(const rg_vec2 a, const rg_vec2 b)
{
    return (rg_vec2) {.x = a.x + b.x, .y = a.y + b.y};
}

static inline rg_vec2 rg_vec2_sub(const rg_vec2 a, const rg_vec2 b)
{
    return (rg_vec2) {.x = a.x - b.x, .y = a.y - b.y};
}

static inline bool rg_vec2_equals(const rg_vec2 a, const rg_vec2 b)
{
    return a.x == b.x && a.y == b.y;
}

static inline float rg_vec2_norm(const rg_vec2 v)
{
    return rg_sqrtf(v.x * v.x + v.y * v.y);
}

static inline rg_vec2 rg_vec2_normalize(const rg_vec2 v)
{
    float norm = rg_vec2_norm(v);
    return (rg_vec2) {
        .x = v.x / norm,
        .y = v.y / norm,
    };
}

static inline float rg_vec2_dot(const rg_vec2 a, const rg_vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

// endregion

// --=== Vec3 ===--

// region Vec3

static inline rg_vec3 rg_vec3_add(const rg_vec3 a, const rg_vec3 b)
{
    return (rg_vec3) {.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z};
}

static inline rg_vec3 rg_vec3_sub(const rg_vec3 a, const rg_vec3 b)
{
    return (rg_vec3) {.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z};
}

static inline rg_vec3 rg_vec3_mul(const rg_vec3 a, const rg_vec3 b)
{
    return (rg_vec3) {.x = a.x * b.x, .y = a.y * b.y, .z = a.z * b.z};
}

static inline float rg_vec3_dot(const rg_vec3 a, const rg_vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float rg_vec3_norm(const rg_vec3 v)
{
    return rg_sqrtf(rg_vec3_dot(v, v));
}

static inline rg_vec3 rg_vec3_normalize(const rg_vec3 v)
{
    float norm = rg_vec3_norm(v);
    return (rg_vec3) {
        .x = v.x / norm,
        .y = v.y / norm,
        .z = v.z / norm,
    };
}

static inline bool rg_vec3_equals(const rg_vec3 a, const rg_vec3 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

static inline rg_vec3 rg_vec3_zero(void)
{
    return (rg_vec3) {.x = 0.0f, .y = 0.0f, .z = 0.0f};
}

static inline rg_vec3 rg_vec3_one(void)
{
    return (rg_vec3) {.x = 1.0f, .y = 1.0f, .z = 1.0f};
}

// endregion

// --=== Vec4 ===--

// region Vec4

static inline rg_vec4 rg_vec4_add(const rg_vec4 a, const rg_vec4 b)
{
    return (rg_vec4) {.x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z, .w = a.w + b.w};
}

static inline rg_vec4 rg_vec4_sub(const rg_vec4 a, const rg_vec4 b)
{
    return (rg_vec4) {.x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z, .w = a.w - b.w};
}

static inline bool rg_vec4_equals(const rg_vec4 a, const rg_vec4 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
}

static inline float rg_vec4_dot(const rg_vec4 a, const rg_vec4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

static inline float rg_vec4_norm(const rg_vec4 v)
{
    return rg_sqrtf(rg_vec4_dot(v, v));
}

static inline rg_vec4 rg_vec4_normalize(const rg_vec4 v)
{
    float norm = rg_vec4_norm(v);
    return (rg_vec4) {
        .x = v.x / norm,
        .y = v.y / norm,
        .z = v.z / norm,
        .w = v.w / norm,
    };
}

// endregion

// --=== Mat4 ===--

// region Mat4

static inline rg_mat4 rg_mat4_identity()
{
    return (rg_mat4) {
        .m =
            {
                {1, 0, 0, 0},
                {0, 1, 0, 0},
                {0, 0, 1, 0},
                {0, 0, 0, 1},
            },
    };
}
rg_mat4               rg_mat4_mul(rg_mat4 a, rg_mat4 b);
static inline rg_mat4 rg_mat4_translate(const rg_mat4 m, const rg_vec3 v)
{
    rg_mat4 result = m;

    result.m03 += v.x;
    result.m13 += v.y;
    result.m23 += v.z;

    return result;
}

static inline rg_mat4 rg_mat4_scale(const rg_mat4 m, const rg_vec3 v) {
    rg_mat4 result = m;

    result.m00 *= v.x;
    result.m01 *= v.x;
    result.m02 *= v.x;
    result.m03 *= v.x;
    result.m10 *= v.y;
    result.m11 *= v.y;
    result.m12 *= v.y;
    result.m13 *= v.y;
    result.m20 *= v.z;
    result.m21 *= v.z;
    result.m22 *= v.z;
    result.m23 *= v.z;

    return result;
}
rg_mat4 rg_mat4_ortho(float left, float right, float bottom, float top, float near, float far);
rg_mat4 rg_mat4_perspective(float fov, float aspect, float near, float far);

// endregion

// --=== Quat ===--

static inline rg_quat rg_quat_identity(void) {
    return (rg_quat) {
        .r = 1,
        .i = 0,
        .j = 0,
        .k = 0
    };
}
static inline rg_quat rg_quat_sum(rg_quat a, rg_quat b)
{
    // Nothing special here
    return (rg_quat) {
        .a = a.a + b.a,
        .b = a.b + b.b,
        .c = a.c + b.c,
        .d = a.d + b.d,
    };
}
static inline rg_quat rg_quat_sub(rg_quat a, rg_quat b)
{
    // Nothing special here
    return (rg_quat) {
        .a = a.a - b.a,
        .b = a.b - b.b,
        .c = a.c - b.c,
        .d = a.d - b.d,
    };
}

rg_quat               rg_quat_mul(rg_quat a, rg_quat b);

static inline rg_quat rg_quat_conjugate(rg_quat quat)
{
    return (rg_quat) {
        .a = quat.a,
        .b = -quat.b,
        .c = -quat.c,
        .d = -quat.d,
    };
}
static inline float rg_quat_norm(rg_quat quat)
{
    return rg_sqrtf((quat.a * quat.a) + (quat.b * quat.b) + (quat.c * quat.c) + (quat.d * quat.d));
}
static inline float rg_quat_distance(rg_quat a, rg_quat b)
{
    return rg_quat_norm(rg_quat_sub(a, b));
}
static inline rg_quat rg_quat_div_scalar(rg_quat quat, float scalar)
{
    return (rg_quat) {
        .a = quat.a / scalar,
        .b = quat.b / scalar,
        .c = quat.c / scalar,
        .d = quat.d / scalar,
    };
}
static inline rg_quat rg_quat_normalize(rg_quat quat)
{
    return rg_quat_div_scalar(quat, rg_quat_norm(quat));
}

static inline rg_quat rg_quat_reciprocal(rg_quat quat)
{
    float norm = rg_quat_norm(quat);
    return rg_quat_div_scalar(rg_quat_conjugate(quat), norm * norm);
}
static inline rg_quat rg_quat_div_left(rg_quat a, rg_quat b)
{
    // a^-1 * b
    return rg_quat_mul(rg_quat_reciprocal(a), b);
}
static inline rg_quat rg_quat_div_right(rg_quat a, rg_quat b)
{
    // a * b^-1
    return rg_quat_mul(a, rg_quat_reciprocal(b));
}
static inline float rg_quat_dot(rg_quat a, rg_quat b)
{
    return (a.b * b.b) + (a.c * b.b) + (a.d * b.d);
}
rg_quat               rg_quat_cross(rg_quat a, rg_quat b);
rg_mat4               rg_quat_to_rotation_matrix(rg_quat quat);
rg_vec3               rg_quat_rotate_point(rg_quat quat, rg_vec3 point);

/**
 * Creates a quaternion from an axis and an angle.
 * @param axis normalized axis vector
 * @param angle angle in radians
 * @return a normalized quaternion
 */
rg_quat rg_quat_from_axis_angle(rg_vec3 axis, double angle);
rg_quat rg_quat_from_euler(float yaw, float pitch, float roll);