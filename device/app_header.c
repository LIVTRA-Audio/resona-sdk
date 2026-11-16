/* Places the image header at the start of a box app. */
#include <resona/image.h>
#include <resona/platform.h>

__attribute__((section(".resona_header"), used))
const resona_image_header resona_header = {
    RESONA_IMAGE_MAGIC,
    RESONA_PLATFORM_VERSION,
    (uint32_t)(uintptr_t)&resona_app_get,
    0u,
};
