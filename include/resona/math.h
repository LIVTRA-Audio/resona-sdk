/*
 * Freestanding math for Resona modules.
 *
 * The device build links no C library, so modules cannot call powf, sinf and
 * friends. These inline replacements are used by the host build too, so a module
 * computes the same coefficients on the desktop and on the device. Use them in
 * init() and set_param(); keep them out of process().
 *
 * Measured against the C library (see tests/sdk_math.rs): exp2/db_to_lin better
 * than 1e-6 relative; expf better than 1e-5 relative over [-80, 80]; log2 better
 * than 1e-6 absolute; sin/cos better than 1e-5 absolute over [-100, 100] (float
 * range reduction limits large arguments); tanh better than 1e-6 absolute.
 */
#ifndef RESONA_MATH_H
#define RESONA_MATH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RESONA_PI 3.14159265358979f

static inline uint32_t resona_f2u_(float x)
{
    union { float f; uint32_t u; } c;
    c.f = x;
    return c.u;
}

static inline float resona_u2f_(uint32_t u)
{
    union { float f; uint32_t u; } c;
    c.u = u;
    return c.f;
}

static inline float resona_fabsf(float x)
{
    return resona_u2f_(resona_f2u_(x) & 0x7FFFFFFFu);
}

static inline float resona_sqrtf(float x)
{
    return x > 0.0f ? __builtin_sqrtf(x) : 0.0f;
}

/* 2^x for x in about [-126, 127]; out-of-range inputs saturate. */
static inline float resona_exp2f(float x)
{
    int i;
    float f, p;

    if (x < -126.0f) {
        return 0.0f;
    }
    if (x > 126.0f) {
        x = 126.0f;
    }
    i = (int)(x + (x >= 0.0f ? 0.5f : -0.5f)); /* nearest integer */
    f = x - (float)i;                           /* [-0.5, 0.5] */
    /* Taylor series of 2^f = e^(f ln2), degree 6. */
    p = 1.0f + f * (0.693147181f + f * (0.240226507f + f * (0.0555041087f +
        f * (0.00961812911f + f * (0.00133335581f + f * 0.000154035304f)))));
    return p * resona_u2f_((uint32_t)(i + 127) << 23);
}

/* log2(x) for x > 0; returns -126 for x <= 0. */
static inline float resona_log2f(float x)
{
    uint32_t u;
    int e;
    float m, t, t2;

    if (!(x > 0.0f)) {
        return -126.0f;
    }
    u = resona_f2u_(x);
    e = (int)((u >> 23) & 0xFFu) - 127;
    m = resona_u2f_((u & 0x007FFFFFu) | 0x3F800000u); /* [1, 2) */
    if (m > 1.41421356f) {
        m *= 0.5f;
        e += 1;
    }
    t = (m - 1.0f) / (m + 1.0f);
    t2 = t * t;
    /* log2(m) = 2/ln2 * atanh(t) */
    return (float)e + 2.88539008f * t * (1.0f + t2 * (0.333333333f +
           t2 * (0.2f + t2 * (0.142857143f + t2 * 0.111111111f))));
}

static inline float resona_expf(float x) { return resona_exp2f(x * 1.44269504f); }
static inline float resona_logf(float x) { return resona_log2f(x) * 0.693147181f; }

/* a^b for a > 0; returns 0 for a <= 0. */
static inline float resona_powf(float a, float b)
{
    return a > 0.0f ? resona_exp2f(b * resona_log2f(a)) : 0.0f;
}

/* 10^(db / 20): decibels to a linear amplitude. */
static inline float resona_db_to_lin(float db)
{
    return resona_exp2f(db * 0.166096405f);
}

/* Linear amplitude to decibels; floored at -200 dB. */
static inline float resona_lin_to_db(float lin)
{
    return lin > 1.0e-10f ? 6.02059991f * resona_log2f(lin) : -200.0f;
}

static inline float resona_sinf(float x)
{
    float k, x2;
    int n;

    /* reduce to [-pi, pi] */
    k = x * 0.159154943f;
    n = (int)(k + (k >= 0.0f ? 0.5f : -0.5f));
    x -= (float)n * 6.28318531f;
    /* fold to [-pi/2, pi/2]: sin(pi - x) = sin(x) */
    if (x > 1.57079633f) {
        x = RESONA_PI - x;
    } else if (x < -1.57079633f) {
        x = -RESONA_PI - x;
    }
    x2 = x * x;
    return x * (1.0f + x2 * (-0.166666667f + x2 * (0.00833333333f +
           x2 * (-0.000198412698f + x2 * (2.75573192e-06f +
           x2 * -2.50521084e-08f)))));
}

static inline float resona_cosf(float x)
{
    return resona_sinf(x + 1.57079633f);
}

static inline float resona_tanhf(float x)
{
    float e;

    if (x > 9.0f) {
        return 1.0f;
    }
    if (x < -9.0f) {
        return -1.0f;
    }
    e = resona_exp2f(x * 2.88539008f); /* e^(2x) */
    return (e - 1.0f) / (e + 1.0f);
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RESONA_MATH_H */
