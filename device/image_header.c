/* Places the image header at the start of a device module. */
#include <resona/image.h>
#include <resona/module.h>

extern const resona_module *resona_module_get(void);

__attribute__((section(".resona_header"), used))
const resona_image_header resona_header = {
    RESONA_IMAGE_MAGIC,
    RESONA_MODULE_ABI,
    (uint32_t)(uintptr_t)&resona_module_get,
    0u,
};
