/*
 * Resona module image (device build), draft.
 *
 * A device module is one flat image linked at address 0, carried in the CODE
 * section of a pack (resona/pack.h). Its first bytes are a
 * resona_image_header; the rest is code and read-only data. The host copies it
 * in its declared executable memory region and adds the load address to each word listed
 * in the pack's relocation table. Modules have no mutable globals: everything
 * that persists is in the state block the host allocates.
 */
#ifndef RESONA_IMAGE_H
#define RESONA_IMAGE_H

#include <stdint.h>

#define RESONA_IMAGE_MAGIC 0x444F4D52u      /* bytes 'R' 'M' 'O' 'D' */
#define RESONA_IMAGE_MAX_BYTES 0x400000u    /* physical declaration sanity, not a reservation */
#define RESONA_IMAGE_ALIGN 16u

typedef struct resona_image_header {
    uint32_t magic;    /* RESONA_IMAGE_MAGIC */
    uint32_t abi;      /* RESONA_MODULE_ABI the module was built against */
    uint32_t entry;    /* offset of resona_module_get from the image start */
    uint32_t reserved; /* zero */
} resona_image_header;

#endif /* RESONA_IMAGE_H */
