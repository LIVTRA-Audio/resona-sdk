/*
 * What a compiler may need in a box app besides runtime.c's memory routines: C string
 * helpers and the C++ hooks. Apps link no C library; text formatting is platform->format.
 */
#include <stddef.h>
#include <stdint.h>

#define NOOPT __attribute__((optimize("no-tree-loop-distribute-patterns")))

NOOPT size_t strlen(const char *s)
{
    const char *p = s;
    while (*p) {
        ++p;
    }
    return (size_t)(p - s);
}

NOOPT int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) {
        ++a;
        ++b;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

NOOPT int strncmp(const char *a, const char *b, size_t n)
{
    for (; n; --n, ++a, ++b) {
        if (*a != *b || !*a) {
            return (int)(unsigned char)*a - (int)(unsigned char)*b;
        }
    }
    return 0;
}

NOOPT char *strncpy(char *d, const char *s, size_t n)
{
    size_t i = 0;
    for (; i < n && s[i]; ++i) {
        d[i] = s[i];
    }
    for (; i < n; ++i) {
        d[i] = '\0';
    }
    return d;
}

NOOPT char *strncat(char *d, const char *s, size_t n)
{
    char *p = d + strlen(d);
    while (n-- && *s) {
        *p++ = *s++;
    }
    *p = '\0';
    return d;
}

/* A pure virtual call is a bug; stop where a debugger can see it. */
void __cxa_pure_virtual(void)
{
    for (;;) {
    }
}

void *__dso_handle = 0;

/* ---- snprintf: d i u x X c s p %, width, '-', '0', '+'; 'l' and 'h' accepted ---------- */

#include <stdarg.h>

typedef struct {
    char *out;
    size_t cap;
    size_t n;
} fmt_sink;

static void put_c(fmt_sink *s, char c)
{
    if (s->n + 1 < s->cap) {
        s->out[s->n] = c;
    }
    s->n++;
}

static void put_s(fmt_sink *s, const char *str, int width, int left)
{
    int len = (int)strlen(str);
    for (int i = len; !left && i < width; ++i) put_c(s, ' ');
    while (*str) put_c(s, *str++);
    for (int i = len; left && i < width; ++i) put_c(s, ' ');
}

static void put_num(fmt_sink *s, unsigned long v, int neg, int base, int upper, int width, int zero,
                    int left, int plus)
{
    char buf[24];
    int n = 0;
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char sign = neg ? '-' : (plus ? '+' : 0);

    do {
        buf[n++] = digits[v % (unsigned)base];
        v /= (unsigned)base;
    } while (v);
    int len = n + (sign ? 1 : 0);
    if (!left && !zero) for (int i = len; i < width; ++i) put_c(s, ' ');
    if (sign) put_c(s, sign);
    if (!left && zero) for (int i = len; i < width; ++i) put_c(s, '0');
    while (n) put_c(s, buf[--n]);
    if (left) for (int i = len; i < width; ++i) put_c(s, ' ');
}

int vsnprintf(char *out, size_t cap, const char *fmt, va_list ap)
{
    fmt_sink s = {out, cap, 0};

    for (; *fmt; ++fmt) {
        if (*fmt != '%') {
            put_c(&s, *fmt);
            continue;
        }
        int left = 0, zero = 0, plus = 0, width = 0;
        for (;; ++fmt) {
            if (fmt[1] == '-') left = 1;
            else if (fmt[1] == '0') zero = 1;
            else if (fmt[1] == '+') plus = 1;
            else break;
        }
        while (fmt[1] >= '0' && fmt[1] <= '9') width = width * 10 + (*++fmt - '0');
        while (fmt[1] == 'l' || fmt[1] == 'h' || fmt[1] == 'z') ++fmt;
        switch (*++fmt) {
        case 'd':
        case 'i': {
            long v = va_arg(ap, int);
            put_num(&s, (unsigned long)(v < 0 ? -v : v), v < 0, 10, 0, width, zero, left, plus);
            break;
        }
        case 'u': put_num(&s, va_arg(ap, unsigned), 0, 10, 0, width, zero, left, 0); break;
        case 'x': put_num(&s, va_arg(ap, unsigned), 0, 16, 0, width, zero, left, 0); break;
        case 'X': put_num(&s, va_arg(ap, unsigned), 0, 16, 1, width, zero, left, 0); break;
        case 'p': put_num(&s, (unsigned long)(uintptr_t)va_arg(ap, void *), 0, 16, 0, width, zero, left, 0); break;
        case 'c': {
            char c = (char)va_arg(ap, int);
            for (int i = 1; !left && i < width; ++i) put_c(&s, ' ');
            put_c(&s, c);
            for (int i = 1; left && i < width; ++i) put_c(&s, ' ');
            break;
        }
        case 's': {
            const char *str = va_arg(ap, const char *);
            put_s(&s, str ? str : "(null)", width, left);
            break;
        }
        case '%': put_c(&s, '%'); break;
        case '\0': --fmt; break;
        default: put_c(&s, '%'); put_c(&s, *fmt); break;
        }
    }
    if (cap) out[s.n < cap ? s.n : cap - 1] = '\0';
    return (int)s.n;
}

int snprintf(char *out, size_t cap, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(out, cap, fmt, ap);
    va_end(ap);
    return n;
}
