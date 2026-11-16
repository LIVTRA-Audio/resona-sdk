/* GPL-3.0 licensed. Explicit algorithm code placement. */
#ifndef RESONA_CODE_H
#define RESONA_CODE_H
#include <stdint.h>
#define RESONA_CODE_VERSION 1u
#define RESONA_CODE_FAST 1u /* dedicated fast code pool (default) */
#define RESONA_CODE_SRAM 2u /* shared SRAM heap, charged with state/buffers */
#define RESONA_CODE_PSRAM 3u /* shared PSRAM heap, charged with state/buffers */
typedef struct resona_code_profile {
    uint32_t version, struct_size, region;
} resona_code_profile;
#endif
