#ifndef MATH_H
#define MATH_H

#include <inttypes.h>

#include <math.h>

#define PI 3.14159265358979323846f
#define SQRT_2 1.41421356237309504880f
#define SQRT_3 1.73205080756887729352f
#define DEG2RAD_MULTIPLIER (PI / 180.0f)
#define RAD2DEG_MULTIPLIER (180.0f / PI)
#define FLOAT_INFINITY 1e30f
#define FLOAT_EPSILON 1.192092896e-07f

static inline float SquareRoot(const float x) {
    return (float)sqrt((float)x);
}

static inline float Sin(const float x) {
    return (float)sin((float)x);
}
static inline float Cos(const float x) {
    return (float)cos((float)x);
}
static inline float Tan(const float x) {
    return (float)tan((float)x);
}

static inline float ArcSin(const float x) {
    return (float)asin((float)x);
}
static inline float ArcCos(const float x) {
    return (float)acos((float)x);
}
static inline float ArcTan(const float x) {
    return (float)atan((float)x);
}

typedef struct Vec2f {
    float data [2];
} Vec2f;
typedef struct Vec3f {
    float data [3];
} Vec3f;
typedef struct Vec4f {
    float data [4];
} Vec4f;

typedef struct Mat2f {
    float data [4];
} Mat2f;
typedef struct Mat3f {
    float data [9];
} Mat3f;
typedef struct Mat4f {
    float data [16];
} Mat4f;

static inline Mat3f Mat3f_Identity() {
    return (Mat3f){{
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    }};
}
static inline Mat4f Mat4f_Identity() {
    return (Mat4f){{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f 
    }}; 
}

static inline void Vec2f_Scale(Vec2f v [static 1], const float scalar) {
    v->data[0] *= scalar;
    v->data[1] *= scalar;
}
static inline void Vec3f_Scale(Vec3f v [static 1], const float scalar) {
    v->data[0] *= scalar;
    v->data[1] *= scalar;
    v->data[2] *= scalar;
}
static inline void Vec4f_Scale(Vec4f v [static 1], const float scalar) {
    v->data[0] *= scalar;
    v->data[1] *= scalar;
    v->data[2] *= scalar;
    v->data[3] *= scalar;
}

static inline void Vec2f_Normalize(Vec2f v [static 1]) {
    const float l = SquareRoot((v->data[0] * v->data[0]) + (v->data[1] * v->data[1]));
    v->data[0] /= l;
    v->data[1] /= l;
}
static inline void Vec3f_Normalize(Vec3f v [static 1]) {
    const float l = SquareRoot((v->data[0] * v->data[0]) + (v->data[1] * v->data[1]) + (v->data[2] * v->data[2]));
    v->data[0] /= l;
    v->data[1] /= l;
    v->data[2] /= l;
}
static inline void Vec4f_Normalize(Vec4f v [static 1]) {
    const float l = SquareRoot((v->data[0] * v->data[0]) + (v->data[1] * v->data[1]) + (v->data[2] * v->data[2]) + (v->data[3] * v->data[3]));
    v->data[0] /= l;
    v->data[1] /= l;
    v->data[2] /= l;
    v->data[3] /= l;
}

static inline float Vec2_Dot(const Vec2f v1 [static 1], const Vec2f v2 [static 1]) {
    return (v1->data[0] * v2->data[0]) + (v1->data[1] * v2->data[1]);
}
static inline float Vec3_Dot(const Vec3f v1 [static 1], const Vec3f v2 [static 1]) {
    return (v1->data[0] * v2->data[0]) + (v1->data[1] * v2->data[1]) + (v1->data[2] * v2->data[2]);
}
static inline float Vec4_Dot(const Vec4f v1 [static 1], const Vec4f v2 [static 1]) {
    return (v1->data[0] * v2->data[0]) + (v1->data[1] * v2->data[1]) + (v1->data[2] * v2->data[2]) + (v1->data[3] * v2->data[3]);
}

static inline Vec3f Vec3_Crossed(const Vec3f v1 [static 1], const Vec3f v2 [static 1]) {
    return (Vec3f){{
        (v1->data[1] * v2->data[2]) - (v1->data[2] * v2->data[1]),
        (v1->data[2] * v2->data[0]) - (v1->data[0] * v2->data[2]),
        (v1->data[0] * v2->data[1]) - (v1->data[1] * v2->data[0])
    }};
} 

static inline Mat4f Mat3f_Multiplied(const Mat4f m1 [static 1], const Mat4f m2 [static 1]) {
    Mat4f m3 = Mat4f_Identity();

    const float* p1 = m1->data;
    const float* p2 = m2->data;
    float* p3 = m3.data;

    for (uint32_t i = 0; i < 4; i++) {
        for (uint32_t j = 0; j < 4; j++) {
            *p3 = (
                (p1[0] * p2[0 + j]) +
                (p1[1] * p2[4 + j]) +
                (p1[2] * p2[8 + j]) +
                (p1[3] * p2[12 + j])
            );
            p1 += 4;
        }
    }

    return m3;
}

static inline Mat4f Mat4f_Translation(const Vec3f position [static 1]) {
    Mat4f m = Mat4f_Identity();

    m.data[12] = position->data[0];
    m.data[13] = position->data[1];
    m.data[14] = position->data[2];

    return m;
}

#endif