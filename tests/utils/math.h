#pragma once

#include "../framework/test_framework.h"
#include <railguard/utils/math.h>

TEST(Vector2)
{
    rg_vec2 a = (rg_vec2) {1, 2};
    rg_vec2 b = (rg_vec2) {3, 4};

    // Addition
    rg_vec2 c = rg_vec2_add(a, b);
    EXPECT_TRUE(c.x == 4 && c.y == 6);
    EXPECT_TRUE(rg_vec2_equals(c, (rg_vec2) {4, 6}));
    EXPECT_FALSE(rg_vec2_equals(c, (rg_vec2) {5, 6}));
    EXPECT_FALSE(rg_vec2_equals(c, (rg_vec2) {4, 7}));
    EXPECT_FALSE(rg_vec2_equals(c, (rg_vec2) {3, 5}));

    // Subtraction
    rg_vec2 d = rg_vec2_sub(a, b);
    EXPECT_TRUE(d.x == -2 && d.y == -2);
    EXPECT_TRUE(rg_vec2_equals(d, (rg_vec2) {-2, -2}));
    EXPECT_FALSE(rg_vec2_equals(d, (rg_vec2) {-3, -2}));
    EXPECT_FALSE(rg_vec2_equals(d, (rg_vec2) {-2, -3}));
    EXPECT_FALSE(rg_vec2_equals(d, (rg_vec2) {1, 3}));

    // Norm
    EXPECT_TRUE(rg_vec2_norm((rg_vec2) {1, 2}) == 2.23606797749979f);
    EXPECT_TRUE(rg_vec2_norm((rg_vec2) {0, 5}) == 5.0f);
    EXPECT_TRUE(rg_vec2_norm((rg_vec2) {-5, 0}) == 5.0f);
    EXPECT_TRUE(rg_vec2_norm((rg_vec2) {3, 4}) == 5.0f);

    // Normalize
    EXPECT_TRUE(rg_vec2_equals(rg_vec2_normalize((rg_vec2) {0, 5}), (rg_vec2) {0, 1}));
    EXPECT_TRUE(rg_vec2_equals(rg_vec2_normalize((rg_vec2) {-5, 0}), (rg_vec2) {-1, 0}));
    EXPECT_TRUE(rg_vec2_equals(rg_vec2_normalize((rg_vec2) {3, 4}), (rg_vec2) {0.6f, 0.8f}));

    // Dot product
    rg_vec2 f = (rg_vec2) {1, 2};
    rg_vec2 g = (rg_vec2) {3, 4};
    EXPECT_TRUE(rg_vec2_dot(f, g) == 11);
    EXPECT_TRUE(rg_vec2_dot(g, f) == 11);
}

TEST(Vector3)
{
    // Addition
    rg_vec3 a = (rg_vec3) {1, 2, 3};
    rg_vec3 b = (rg_vec3) {3, 4, 5};
    rg_vec3 c = rg_vec3_add(a, b);
    EXPECT_TRUE(c.x == 4 && c.y == 6 && c.z == 8);
    EXPECT_TRUE(rg_vec3_equals(c, (rg_vec3) {4, 6, 8}));
    EXPECT_FALSE(rg_vec3_equals(c, (rg_vec3) {5, 6, 8}));
    EXPECT_FALSE(rg_vec3_equals(c, (rg_vec3) {4, 7, 8}));
    EXPECT_FALSE(rg_vec3_equals(c, (rg_vec3) {3, 5, 8}));
    EXPECT_FALSE(rg_vec3_equals(c, (rg_vec3) {4, 6, 7}));

    // Subtraction
    rg_vec3 d = rg_vec3_sub(a, b);
    EXPECT_TRUE(d.x == -2 && d.y == -2 && d.z == -2);
    EXPECT_TRUE(rg_vec3_equals(d, (rg_vec3) {-2, -2, -2}));
    EXPECT_FALSE(rg_vec3_equals(d, (rg_vec3) {-3, -2, -2}));

    // Norm
    EXPECT_TRUE(rg_vec3_norm((rg_vec3) {1, 2, 3}) == 3.74165738677394f);
    EXPECT_TRUE(rg_vec3_norm((rg_vec3) {0, 5, 0}) == 5.0f);
    EXPECT_TRUE(rg_vec3_norm((rg_vec3) {-5, 0, 0}) == 5.0f);

    // Normalize
    EXPECT_TRUE(rg_vec3_equals(rg_vec3_normalize((rg_vec3) {0, 5, 0}), (rg_vec3) {0, 1, 0}));
    EXPECT_TRUE(rg_vec3_equals(rg_vec3_normalize((rg_vec3) {-5, 0, 0}), (rg_vec3) {-1, 0, 0}));
    EXPECT_TRUE(rg_vec3_equals(rg_vec3_normalize((rg_vec3) {0, 3, 4}), (rg_vec3) {0, 0.6f, 0.8f}));

    // Dot product
    rg_vec3 f = (rg_vec3) {1, 2, 3};
    rg_vec3 g = (rg_vec3) {3, 4, 5};
    EXPECT_TRUE(rg_vec3_dot(f, g) == 26);
}

TEST(Vector4)
{
    // Addition
    rg_vec4 a = (rg_vec4) {1, 2, 3, 4};
    rg_vec4 b = (rg_vec4) {5, 6, 7, 8};
    rg_vec4 c = rg_vec4_add(a, b);
    EXPECT_TRUE(c.x == 6 && c.y == 8 && c.z == 10 && c.w == 12);
    EXPECT_TRUE(rg_vec4_equals(c, (rg_vec4) {6, 8, 10, 12}));
    EXPECT_FALSE(rg_vec4_equals(c, (rg_vec4) {7, 8, 10, 12}));
    EXPECT_FALSE(rg_vec4_equals(c, (rg_vec4) {6, 9, 10, 12}));

    // Subtraction
    rg_vec4 d = rg_vec4_sub(a, b);
    EXPECT_TRUE(d.x == -4 && d.y == -4 && d.z == -4 && d.w == -4);
    EXPECT_TRUE(rg_vec4_equals(d, (rg_vec4) {-4, -4, -4, -4}));

    // Norm
    EXPECT_TRUE(rg_vec4_norm((rg_vec4) {1, 2, 3, 4}) == 5.477225575051661f);
    EXPECT_TRUE(rg_vec4_norm((rg_vec4) {0, 5, 0, 0}) == 5.0f);
    EXPECT_TRUE(rg_vec4_norm((rg_vec4) {-5, 0, 0, 0}) == 5.0f);
    EXPECT_TRUE(rg_vec4_norm((rg_vec4) {3, 4, 0, 0}) == 5.0f);

    // Normalize
    EXPECT_TRUE(rg_vec4_equals(rg_vec4_normalize((rg_vec4) {0, 5, 0, 0}), (rg_vec4) {0, 1, 0, 0}));
    EXPECT_TRUE(rg_vec4_equals(rg_vec4_normalize((rg_vec4) {-5, 0, 0, 0}), (rg_vec4) {-1, 0, 0, 0}));
    EXPECT_TRUE(rg_vec4_equals(rg_vec4_normalize((rg_vec4) {3, 4, 0, 0}), (rg_vec4) {0.6f, 0.8f, 0, 0}));

    // Dot product
    EXPECT_TRUE(rg_vec4_dot((rg_vec4) {1, 2, 3, 4}, (rg_vec4) {5, 6, 7, 8}) == 70);
    EXPECT_TRUE(rg_vec4_dot((rg_vec4) {5, 6, 7, 8}, (rg_vec4) {1, 2, 3, 4}) == 70);
}

TEST(Matrix4)
{
    // Generate a few matrices
    rg_mat4 id     = rg_mat4_identity();
    rg_vec3 v      = (rg_vec3) {7, 8, 9};
    rg_mat4 scaled = rg_mat4_scale(id, v);
    rg_mat4 translated = rg_mat4_translate(id, v);
    rg_mat4 translated_then_scaled = rg_mat4_mul(scaled, translated);
    rg_mat4 translated_then_scaled2 = rg_mat4_scale(translated, v);
    rg_mat4 scaled_then_translated  = rg_mat4_mul(translated, scaled);
    rg_mat4 scaled_then_translated2 = rg_mat4_translate(scaled, v);

    // Test the contents
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (i == j)
            {
                EXPECT_TRUE(id.m[i][j] == 1);
                EXPECT_TRUE(translated.m[i][j] == 1);
                if (i < 3)
                {
                    EXPECT_TRUE(scaled.m[i][j] == v.v[i]);
                    EXPECT_TRUE(translated_then_scaled.m[i][j] == v.v[i]);
                    EXPECT_TRUE(translated_then_scaled2.m[i][j] == v.v[i]);
                    EXPECT_TRUE(scaled_then_translated.m[i][j] == v.v[i]);
                    EXPECT_TRUE(scaled_then_translated2.m[i][j] == v.v[i]);
                }
                else
                {
                    EXPECT_TRUE(scaled.m[i][j] == 1);
                    EXPECT_TRUE(translated_then_scaled.m[i][j] == 1);
                    EXPECT_TRUE(translated_then_scaled2.m[i][j] == 1);
                    EXPECT_TRUE(scaled_then_translated.m[i][j] == 1);
                    EXPECT_TRUE(scaled_then_translated2.m[i][j] == 1);
                }
            }
            else
            {
                EXPECT_TRUE(id.m[i][j] == 0);
                EXPECT_TRUE(scaled.m[i][j] == 0);

                if (j == 3) {
                    EXPECT_TRUE(translated.m[i][j] == v.v[i]);
                    EXPECT_TRUE(translated_then_scaled.m[i][j] == v.v[i] * v.v[i]);
                    EXPECT_TRUE(translated_then_scaled2.m[i][j] == v.v[i] * v.v[i]);
                    EXPECT_TRUE(scaled_then_translated.m[i][j] == v.v[i]);
                    EXPECT_TRUE(scaled_then_translated2.m[i][j] == v.v[i]);
                } else {
                    EXPECT_TRUE(translated.m[i][j] == 0);
                    EXPECT_TRUE(translated_then_scaled.m[i][j] == 0);
                    EXPECT_TRUE(translated_then_scaled2.m[i][j] == 0);
                    EXPECT_TRUE(scaled_then_translated.m[i][j] == 0);
                    EXPECT_TRUE(scaled_then_translated2.m[i][j] == 0);
                }
            }
        }
    }

    // Test the multiplication
    rg_mat4 m1 = {
        .m = {
            {1, 2, 3, 4},
            {5, 6, 7, 8},
            {9, 10, 11, 12},
            {13, 14, 15, 16}
        }
    };
    rg_mat4 m2 = {
        .m = {
            {17, 18, 19, 20},
            {21, 22, 23, 24},
            {25, 26, 27, 28},
            {29, 30, 31, 32}
        }
    };
    rg_mat4 m3 = rg_mat4_mul(m1, m2);

    // Check that the multiplication worked
    EXPECT_TRUE(m3.m00 == 250);
    EXPECT_TRUE(m3.m01 == 260);
    EXPECT_TRUE(m3.m02 == 270);
    EXPECT_TRUE(m3.m03 == 280);
    EXPECT_TRUE(m3.m10 == 618);
    EXPECT_TRUE(m3.m11 == 644);
    EXPECT_TRUE(m3.m12 == 670);
    EXPECT_TRUE(m3.m13 == 696);
    EXPECT_TRUE(m3.m20 == 986);
    EXPECT_TRUE(m3.m21 == 1028);
    EXPECT_TRUE(m3.m22 == 1070);
    EXPECT_TRUE(m3.m23 == 1112);
    EXPECT_TRUE(m3.m30 == 1354);
    EXPECT_TRUE(m3.m31 == 1412);
    EXPECT_TRUE(m3.m32 == 1470);
    EXPECT_TRUE(m3.m33 == 1528);

    // Projections TODO
    // do that when we have a graphical feedback



}

TEST(Quaternion)
{
    rg_quat q = rg_quat_identity();
    // q1 = i
    rg_quat q1 = rg_quat_from_axis_angle((rg_vec3) {1, 0, 0}, RG_PI);
    rg_quat q2 = rg_quat_from_axis_angle((rg_vec3) {0, 1, 0}, rg_radians(90));

    // Ensure that the quaternions were created correctly
    EXPECT_TRUE(q.x == 0);
    EXPECT_TRUE(q.y == 0);
    EXPECT_TRUE(q.z == 0);
    EXPECT_TRUE(q.w == 1);

    EXPECT_TRUE(q1.x == 1);
    EXPECT_TRUE(q1.y == 0);
    EXPECT_TRUE(q1.z == 0);
    EXPECT_TRUE(q1.w == 0);

    EXPECT_TRUE(q2.x == 0);
    // SQRT(2)/2
    EXPECT_TRUE(q2.y == 0.707106769f);
    EXPECT_TRUE(q2.z == 0);
    EXPECT_TRUE(q2.w == 0.707106769f);

    // Test the multiplication
    rg_quat q3 = rg_quat_mul(q1, q2);
    EXPECT_TRUE(q3.x == 0.707106769f);
    EXPECT_TRUE(q3.y == 0);
    EXPECT_TRUE(q3.z == 0.707106769f);
    EXPECT_TRUE(q3.w == 0);

    // Test the multiplication with identity
    rg_quat q4 = rg_quat_mul(q3, rg_quat_identity());
    EXPECT_TRUE(q4.x == 0.707106769f);
    EXPECT_TRUE(q4.y == 0);
    EXPECT_TRUE(q4.z == 0.707106769f);
    EXPECT_TRUE(q4.w == 0);

    // Test the multiplication with itself
    rg_quat q5 = rg_quat_mul(q2, q2);
    EXPECT_TRUE(q5.x == 0);
    // Allow a small margin of error because of floating point imprecision
    EXPECT_TRUE(q5.y >= 0.999999 && q5.y <= 1.000001);
    EXPECT_TRUE(q5.z == 0);
    EXPECT_TRUE(q5.w == 0);

    // Test sum
    rg_quat q6 = rg_quat_sum((rg_quat) {1,2,3,4}, (rg_quat) {1,2,3,4});
    EXPECT_TRUE(q6.w == 2);
    EXPECT_TRUE(q6.x == 4);
    EXPECT_TRUE(q6.y == 6);
    EXPECT_TRUE(q6.z == 8);

    // Test sub
    rg_quat q7 = rg_quat_sub((rg_quat) {1,2,3,4}, (rg_quat) {4,3,2,1});
    EXPECT_TRUE(q7.w == -3);
    EXPECT_TRUE(q7.x == -1);
    EXPECT_TRUE(q7.y == 1);
    EXPECT_TRUE(q7.z == 3);

    // Test conjugate
    rg_quat conj = rg_quat_conjugate((rg_quat) { 1 , 2, 3, 4});
    EXPECT_TRUE(conj.w == 1);
    EXPECT_TRUE(conj.x == -2);
    EXPECT_TRUE(conj.y == -3);
    EXPECT_TRUE(conj.z == -4);

    // More tests TODO

}