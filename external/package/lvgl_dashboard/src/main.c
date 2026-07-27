#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "lvgl/lvgl.h"
#include "dashboard.h"

static uint32_t millis(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

int main(void)
{
    lv_init();

    lv_display_t *disp = lv_linux_drm_create();
    if (disp == NULL) {
        fprintf(stderr, "Impossible d'initialiser le peripherique DRM\n");
        return 1;
    }

    char *drm_path = lv_linux_drm_find_device_path();
    const char *device = drm_path != NULL ? drm_path : "/dev/dri/card0";

    if (lv_linux_drm_set_file(disp, device, -1) != LV_RESULT_OK) {
        fprintf(stderr, "Impossible d'ouvrir %s\n", device);
        lv_free(drm_path);
        return 1;
    }
    lv_free(drm_path);

    create_dashboard();

    uint32_t last = millis();
    while (1) {
        uint32_t now = millis();
        lv_tick_inc(now - last);
        last = now;

        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}
