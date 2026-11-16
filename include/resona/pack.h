/*
 * Resona pack file (.rpk), format 1, draft. This header defines the container layout.
 * Constants shared by the Lab tools and device hosts. All integers are
 * little-endian.
 */
#ifndef RESONA_PACK_H
#define RESONA_PACK_H

#include <stdint.h>

#define RESONA_PACK_MAGIC 0x4B415052u   /* bytes 'R' 'P' 'A' 'K' */
#define RESONA_PACK_FORMAT 1u
#define RESONA_PACK_HEADER_BYTES 32u
#define RESONA_PACK_MAX_BYTES 0x810000u /* bounded container sanity; live memory decides admission */

#define RESONA_PACK_FLAG_SIGNED 0x1u    /* reserved */
#define RESONA_PACK_FLAG_ENCRYPTED 0x2u /* reserved */

#define RESONA_PACK_TAG(a, b, c, d) \
    ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))
#define RESONA_PACK_TAG_MREQ RESONA_PACK_TAG('M', 'R', 'E', 'Q')
#define RESONA_PACK_TAG_META RESONA_PACK_TAG('M', 'E', 'T', 'A')
#define RESONA_PACK_TAG_CODE RESONA_PACK_TAG('C', 'O', 'D', 'E')
#define RESONA_PACK_TAG_SIGN RESONA_PACK_TAG('S', 'I', 'G', 'N')
#define RESONA_PACK_TAG_LICN RESONA_PACK_TAG('L', 'I', 'C', 'N')

/* CODE targets */
#define RESONA_PACK_TARGET_BOX RESONA_PACK_TAG('R', 'B', 'X', '1') /* Resona Box target 1. */

/* META kind (the byte after flags) */
#define RESONA_PACK_KIND_ALGORITHM 0u
#define RESONA_PACK_KIND_APP 1u

#define RESONA_PACK_TAG_APP1 RESONA_PACK_TAG('A', 'P', 'P', '1')

/* META flags */
#define RESONA_PACK_META_SCREEN 0x1u
#define RESONA_PACK_META_EVENTS 0x2u
#define RESONA_PACK_META_STATE_PSRAM 0x4u
#define RESONA_PACK_META_CODE_SRAM 0x8u
#define RESONA_PACK_META_CODE_PSRAM 0x10u

#define RESONA_PACK_STRING_MAX 60u
#define RESONA_PACK_PARAM_NAME 16u
#define RESONA_PACK_PARAM_UNIT 8u
#define RESONA_PACK_PARAM_BYTES 44u /* name, unit, min, max, def, ext_flags, step */

#endif /* RESONA_PACK_H */
