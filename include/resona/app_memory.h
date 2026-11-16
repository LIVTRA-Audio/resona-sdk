/* GPL-3.0. App-owned memory banks. */
#ifndef RESONA_APP_MEMORY_H
#define RESONA_APP_MEMORY_H
#include <stdint.h>

#define RESONA_APP_MEMORY_LARGE 0u
#define RESONA_APP_MEMORY_FAST 1u
#define RESONA_APP_MEMORY_REALTIME 2u
#define RESONA_APP_MEMORY_BANKS 3u

typedef struct resona_app_memory_info {
    uint32_t version, size;
    uint32_t bank;
    uint32_t budget_bytes;    /* this App's allocation limit */
    uint32_t used_bytes;      /* live allocations, excluding alignment gaps */
    uint32_t reserved_bytes;  /* physical arena, including isolation padding */
    uint32_t available_bytes; /* unowned bytes, after platform reserves */
    uint32_t largest_free_bytes; /* contiguous unowned block, before alignment */
} resona_app_memory_info;
#endif
