/*
 * Arithmetic helpers RV32 has no instruction for: 64-bit division and remainder, 64-bit
 * integer <-> float conversions, count leading/trailing zeros, integer powers.
 *
 * libgcc has them, but toolchains ship it built for absolute addressing (it reads `__clz_tab`
 * and constants through lui/addi pairs). A pack or box app is placed in memory at load time
 * and only 32-bit address words are relocated, so those copies cannot be used. These are
 * plain C, compiled PC-relative with the rest of the program; the linker takes them before
 * libgcc, and --gc-sections keeps only what the program calls.
 *
 * Do not write 64-bit `/`, `%` or int64 <-> float casts in this file: the compiler turns them
 * into calls to the functions defined here.
 */
#include <stdint.h>

#ifndef RESONA_ARITH
#define RESONA_ARITH(name) name /* the host test renames them */
#endif

typedef uint64_t u64;
typedef int64_t i64;
typedef uint32_t u32;

static u64 udivmod(u64 n, u64 d, u64 *rem)
{
    u64 q = 0;
    u64 r = 0;
    int i;

    if (d == 0) { /* undefined in C; give what the RISC-V divide instructions give */
        if (rem) {
            *rem = n;
        }
        return ~(u64)0;
    }
    if ((n >> 32) == 0 && (d >> 32) == 0) { /* the hardware divides 32 bits */
        if (rem) {
            *rem = (u32)n % (u32)d;
        }
        return (u32)n / (u32)d;
    }
    if (d >> 63) { /* the quotient is 0 or 1; the shift below would overflow */
        q = n >= d;
        if (rem) {
            *rem = q ? n - d : n;
        }
        return q;
    }
    for (i = 63; i >= 0 && !((n >> i) & 1u); --i) {
    }
    for (; i >= 0; --i) {
        r = (r << 1) | ((n >> i) & 1u);
        if (r >= d) {
            r -= d;
            q |= (u64)1 << i;
        }
    }
    if (rem) {
        *rem = r;
    }
    return q;
}

static u64 magnitude(i64 v)
{
    return v < 0 ? (u64)0 - (u64)v : (u64)v;
}

u64 RESONA_ARITH(__udivdi3)(u64 a, u64 b) { return udivmod(a, b, 0); }

u64 RESONA_ARITH(__umoddi3)(u64 a, u64 b)
{
    u64 r;
    udivmod(a, b, &r);
    return r;
}

i64 RESONA_ARITH(__divdi3)(i64 a, i64 b)
{
    u64 q = udivmod(magnitude(a), magnitude(b), 0);
    return ((a < 0) != (b < 0)) ? (i64)((u64)0 - q) : (i64)q;
}

i64 RESONA_ARITH(__moddi3)(i64 a, i64 b)
{
    u64 r;
    udivmod(magnitude(a), magnitude(b), &r);
    return a < 0 ? (i64)((u64)0 - r) : (i64)r;
}

/* int64 -> double: hi * 2^32 and lo are exact, so the sum is rounded once. */
double RESONA_ARITH(__floatundidf)(u64 a) { return (double)(u32)(a >> 32) * 4294967296.0 + (double)(u32)a; }
double RESONA_ARITH(__floatdidf)(i64 a) { return (double)(int32_t)(a >> 32) * 4294967296.0 + (double)(u32)a; }

/* int64 -> float: below 2^53 the double is exact and the float rounds once; above, fold the
 * low bits into a sticky bit so the single rounding is still correct. */
static float u64_to_float(u64 a)
{
    if (a >> 53) {
        a = (a >> 11) | ((a & 0x7FFu) != 0);
        return (float)(RESONA_ARITH(__floatundidf)(a) * 2048.0);
    }
    return (float)RESONA_ARITH(__floatundidf)(a);
}

float RESONA_ARITH(__floatundisf)(u64 a) { return u64_to_float(a); }
float RESONA_ARITH(__floatdisf)(i64 a) { return a < 0 ? -u64_to_float(magnitude(a)) : u64_to_float((u64)a); }

/* double -> int64, toward zero; out of range saturates, NaN gives 0. */
u64 RESONA_ARITH(__fixunsdfdi)(double x)
{
    u32 hi;
    double rest;

    if (!(x > 0.0)) {
        return 0;
    }
    if (x >= 18446744073709551616.0) {
        return ~(u64)0;
    }
    hi = (u32)(x * (1.0 / 4294967296.0)); /* the hardware conversion truncates */
    rest = x - (double)hi * 4294967296.0;
    return ((u64)hi << 32) | (u32)rest;
}

i64 RESONA_ARITH(__fixdfdi)(double x)
{
    if (x != x) {
        return 0;
    }
    if (x >= 9223372036854775808.0) {
        return INT64_MAX;
    }
    if (x <= -9223372036854775808.0) {
        return INT64_MIN;
    }
    return x < 0.0 ? (i64)((u64)0 - RESONA_ARITH(__fixunsdfdi)(-x)) : (i64)RESONA_ARITH(__fixunsdfdi)(x);
}

u64 RESONA_ARITH(__fixunssfdi)(float x) { return RESONA_ARITH(__fixunsdfdi)((double)x); }
i64 RESONA_ARITH(__fixsfdi)(float x) { return RESONA_ARITH(__fixdfdi)((double)x); }

int RESONA_ARITH(__clzsi2)(u32 x)
{
    int n = 0;

    if (!(x & 0xFFFF0000u)) { n += 16; x <<= 16; }
    if (!(x & 0xFF000000u)) { n += 8; x <<= 8; }
    if (!(x & 0xF0000000u)) { n += 4; x <<= 4; }
    if (!(x & 0xC0000000u)) { n += 2; x <<= 2; }
    if (!(x & 0x80000000u)) { n += 1; }
    return n;
}

int RESONA_ARITH(__clzdi2)(u64 x)
{
    return (x >> 32) ? RESONA_ARITH(__clzsi2)((u32)(x >> 32)) : 32 + RESONA_ARITH(__clzsi2)((u32)x);
}

int RESONA_ARITH(__ctzsi2)(u32 x)
{
    int n = 0;

    if (!(x & 0xFFFFu)) { n += 16; x >>= 16; }
    if (!(x & 0xFFu)) { n += 8; x >>= 8; }
    if (!(x & 0xFu)) { n += 4; x >>= 4; }
    if (!(x & 0x3u)) { n += 2; x >>= 2; }
    if (!(x & 0x1u)) { n += 1; }
    return n;
}

int RESONA_ARITH(__ctzdi2)(u64 x)
{
    return (u32)x ? RESONA_ARITH(__ctzsi2)((u32)x) : 32 + RESONA_ARITH(__ctzsi2)((u32)(x >> 32));
}

float RESONA_ARITH(__powisf2)(float a, int b)
{
    u32 n = b < 0 ? (u32)0 - (u32)b : (u32)b;
    float r = (n & 1u) ? a : 1.0f;

    while (n >>= 1) {
        a *= a;
        if (n & 1u) {
            r *= a;
        }
    }
    return b < 0 ? 1.0f / r : r;
}

double RESONA_ARITH(__powidf2)(double a, int b)
{
    u32 n = b < 0 ? (u32)0 - (u32)b : (u32)b;
    double r = (n & 1u) ? a : 1.0;

    while (n >>= 1) {
        a *= a;
        if (n & 1u) {
            r *= a;
        }
    }
    return b < 0 ? 1.0 / r : r;
}
