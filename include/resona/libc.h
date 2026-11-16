/*
 * The few C library functions a box app gets (it links no C library): memory and string
 * helpers and a small snprintf (d i u x X c s p %, width, '-', '0', '+', 'l' ignored).
 * Implemented in sdk/device/runtime.c and app_runtime.c.
 */
#ifndef RESONA_LIBC_H
#define RESONA_LIBC_H

#include <stdarg.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
void *memset(void *dst, int c, size_t n);
int memcmp(const void *a, const void *b, size_t n);
size_t strlen(const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t n);
char *strncpy(char *dst, const char *src, size_t n);
char *strncat(char *dst, const char *src, size_t n);
int vsnprintf(char *out, size_t cap, const char *fmt, va_list ap);
int snprintf(char *out, size_t cap, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* RESONA_LIBC_H */
