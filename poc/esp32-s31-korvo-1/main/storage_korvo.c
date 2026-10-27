#include "storage_korvo.h"

#include "bsp/esp-bsp.h"

esp_err_t storage_korvo_mount(void)
{
    return bsp_sdcard_mount();
}

const char *storage_korvo_mount_point(void)
{
    return BSP_SD_MOUNT_POINT;
}
