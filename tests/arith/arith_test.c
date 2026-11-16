/* Host test for sdk/device/arith.c: every helper against the host's own 64-bit arithmetic,
 * on edge values and a million pseudo-random ones. Built and run by lab-core's tests:
 *   cc -O1 sdk/tests/arith/arith_test.c -o arith_test && ./arith_test */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define RESONA_ARITH(name) test##name
#include "../../device/arith.c"

static uint64_t s = 0x9E3779B97F4A7C15ull;
static uint64_t next(void)
{
    s ^= s << 13; s ^= s >> 7; s ^= s << 17;
    switch (s & 7u) { /* mix magnitudes: small, 32-bit, huge */
    case 0: return s & 0xFFu;
    case 1: return s & 0xFFFFFFFFu;
    case 2: return s >> (s & 63u);
    default: return s;
    }
}

static int fails;
#define CHECK(cond, ...) do { if (!(cond)) { if (fails++ < 20) { printf("FAIL " __VA_ARGS__); printf("\n"); } } } while (0)

static int same_float(float a, float b) { return memcmp(&a, &b, sizeof a) == 0; }
static int same_double(double a, double b) { return memcmp(&a, &b, sizeof a) == 0; }

static void one(uint64_t a, uint64_t b)
{
    int64_t sa = (int64_t)a, sb = (int64_t)b;
    if (b) {
        CHECK(test__udivdi3(a, b) == a / b, "udiv %llx %llx", (unsigned long long)a, (unsigned long long)b);
        CHECK(test__umoddi3(a, b) == a % b, "umod %llx %llx", (unsigned long long)a, (unsigned long long)b);
        if (!(sa == INT64_MIN && sb == -1)) {
            CHECK(test__divdi3(sa, sb) == sa / sb, "div %lld %lld", (long long)sa, (long long)sb);
            CHECK(test__moddi3(sa, sb) == sa % sb, "mod %lld %lld", (long long)sa, (long long)sb);
        }
    }
    CHECK(same_double(test__floatundidf(a), (double)a), "floatundidf %llx", (unsigned long long)a);
    CHECK(same_double(test__floatdidf(sa), (double)sa), "floatdidf %lld", (long long)sa);
    CHECK(same_float(test__floatundisf(a), (float)a), "floatundisf %llx", (unsigned long long)a);
    CHECK(same_float(test__floatdisf(sa), (float)sa), "floatdisf %lld", (long long)sa);
    if (a) {
        CHECK(test__clzdi2(a) == __builtin_clzll(a), "clzdi2 %llx", (unsigned long long)a);
        CHECK(test__ctzdi2(a) == __builtin_ctzll(a), "ctzdi2 %llx", (unsigned long long)a);
    }
    /* conversions back, from a double in range */
    double d = (double)sa / 3.0;
    CHECK(test__fixdfdi(d) == (int64_t)d, "fixdfdi %.17g", d);
    double u = (double)a * 0.75;
    if (u < 18446744073709551616.0) {
        CHECK(test__fixunsdfdi(u) == (uint64_t)u, "fixunsdfdi %.17g", u);
    }
    float f = (float)d;
    if (fabsf(f) < 9.2e18f) {
        CHECK(test__fixsfdi(f) == (int64_t)f, "fixsfdi %.9g", f);
    }
}

int main(void)
{
    static const uint64_t edge[] = {0, 1, 2, 3, 7, 0x7F, 0xFFFFFFFFull, 0x100000000ull, 0x7FFFFFFFFFFFFFFFull,
                                    0x8000000000000000ull, 0xFFFFFFFFFFFFFFFFull, 0x20000000000001ull,
                                    0x1FFFFFFFFFFFFFull, 0xFFFFFFFFFFFFF800ull, 0x8000000000000001ull};
    for (unsigned i = 0; i < sizeof edge / sizeof edge[0]; ++i)
        for (unsigned j = 0; j < sizeof edge / sizeof edge[0]; ++j)
            one(edge[i], edge[j]);
    for (int i = 0; i < 1000000; ++i)
        one(next(), next());
    CHECK(test__powisf2(1.5f, 7) == 1.5f * 1.5f * 1.5f * 1.5f * 1.5f * 1.5f * 1.5f, "powisf2");
    CHECK(test__powidf2(2.0, -3) == 0.125, "powidf2");
    CHECK(test__fixdfdi(1e30) == INT64_MAX && test__fixdfdi(-1e30) == INT64_MIN && test__fixdfdi(NAN) == 0, "saturation");
    CHECK(test__clzsi2(1) == 31 && test__ctzsi2(0x80000000u) == 31, "clz/ctz 32");
    printf("%s (%d failures)\n", fails ? "FAILED" : "all match", fails);
    return fails != 0;
}
