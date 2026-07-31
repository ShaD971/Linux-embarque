#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "lvgl/lvgl.h"
#include "dashboard.h"

#if !defined(DASH_BACKEND_DRM) && !defined(DASH_BACKEND_SDL)
#error "Aucun backend selectionne. Compiler via le Makefile : make (drm) ou make BACKEND=sdl"
#endif

/* Au boot, /dev/dri/card* peut ne pas encore exister et le connecteur HDMI peut
 * n'etre detecte qu'apres l'allumage de l'ecran. Sans reprise, l'init echoue une
 * seule fois et l'ecran reste noir jusqu'au prochain reboot. */
#define DRM_RETRY_COUNT   10
#define DRM_RETRY_DELAY_S 1

/* Bornes de sommeil de la boucle principale. Le plafond garde le processus
 * reactif aux signaux meme quand aucun timer n'est pret. */
#define LOOP_MIN_SLEEP_MS 1
#define LOOP_MAX_SLEEP_MS 100

static volatile sig_atomic_t should_quit = 0;

static void on_signal(int sig)
{
    LV_UNUSED(sig);
    should_quit = 1;
}

static void install_signal_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

static uint32_t millis(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

/* ------------------------------------------------------------------ */
/* Backend DRM : la cible                                              */
/* ------------------------------------------------------------------ */

#if defined(DASH_BACKEND_DRM)

static lv_display_t *open_display(void)
{
    for (int attempt = 1; attempt <= DRM_RETRY_COUNT && !should_quit; attempt++) {
        lv_display_t *disp = lv_linux_drm_create();

        if (disp == NULL) {
            fprintf(stderr, "DRM [%d/%d] : lv_linux_drm_create a echoue\n",
                    attempt, DRM_RETRY_COUNT);
        }
        else {
            char *found = lv_linux_drm_find_device_path();
            const char *device = (found != NULL) ? found : "/dev/dri/card0";

            /* connector_id = -1 : le driver prend le premier connecteur
             * effectivement connecte. S'il n'y en a aucun (ecran eteint ou pas
             * encore branche), drm_setup echoue et on retentera. */
            lv_result_t res = lv_linux_drm_set_file(disp, device, -1);

            if (res == LV_RESULT_OK) {
                fprintf(stderr, "DRM : affichage initialise sur %s (%" LV_PRId32
                        "x%" LV_PRId32 ")\n", device,
                        lv_display_get_horizontal_resolution(disp),
                        lv_display_get_vertical_resolution(disp));
                lv_free(found);
                return disp;
            }

            fprintf(stderr, "DRM [%d/%d] : %s inutilisable "
                    "(peripherique absent ou aucun connecteur actif)\n",
                    attempt, DRM_RETRY_COUNT, device);
            lv_free(found);

            /* lv_linux_drm_create a deja alloue le display et ses donnees de
             * driver : sans ce delete, chaque tentative en fuirait un. */
            lv_display_delete(disp);
        }

        if (attempt < DRM_RETRY_COUNT) {
            sleep(DRM_RETRY_DELAY_S);
        }
    }

    return NULL;
}

#endif /* DASH_BACKEND_DRM */

/* ------------------------------------------------------------------ */
/* Backend SDL : poste de developpement uniquement                     */
/* ------------------------------------------------------------------ */

#if defined(DASH_BACKEND_SDL)

/* Ecrit un BMP 32 bits non compresse (lignes de bas en haut). L'ordre memoire
 * de LV_COLOR_FORMAT_ARGB8888 est deja B,G,R,A : identique au BMP. */
static bool write_bmp(const char *path, const lv_draw_buf_t *buf)
{
    const int32_t w = (int32_t)buf->header.w;
    const int32_t h = (int32_t)buf->header.h;
    const uint32_t stride = buf->header.stride;
    const uint32_t row_bytes = (uint32_t)w * 4u;
    const uint32_t pixels_size = row_bytes * (uint32_t)h;
    const uint32_t offset = 14u + 40u;

    FILE *f = fopen(path, "wb");
    if (f == NULL) {
        fprintf(stderr, "Capture : impossible d'ecrire %s\n", path);
        return false;
    }

    uint8_t hdr[54] = {0};
    hdr[0] = 'B';
    hdr[1] = 'M';
    const uint32_t file_size = offset + pixels_size;
    memcpy(&hdr[2], &file_size, 4);
    memcpy(&hdr[10], &offset, 4);

    const uint32_t dib_size = 40;
    memcpy(&hdr[14], &dib_size, 4);
    memcpy(&hdr[18], &w, 4);
    memcpy(&hdr[22], &h, 4);
    const uint16_t planes = 1, bpp = 32;
    memcpy(&hdr[26], &planes, 2);
    memcpy(&hdr[28], &bpp, 2);
    memcpy(&hdr[34], &pixels_size, 4);

    bool ok = fwrite(hdr, sizeof(hdr), 1, f) == 1;

    for (int32_t y = h - 1; ok && y >= 0; y--) {
        const uint8_t *row = buf->data + (size_t)y * stride;
        ok = fwrite(row, row_bytes, 1, f) == 1;
    }

    if (fclose(f) != 0) {
        ok = false;
    }

    if (ok) {
        fprintf(stderr, "Capture : %s (%" LV_PRId32 "x%" LV_PRId32 ")\n", path, w, h);
    }
    else {
        fprintf(stderr, "Capture : echec d'ecriture de %s\n", path);
    }
    return ok;
}

static bool take_screenshot(const char *path)
{
    lv_draw_buf_t *snap = lv_snapshot_take(lv_screen_active(), LV_COLOR_FORMAT_ARGB8888);
    if (snap == NULL) {
        fprintf(stderr, "Capture : lv_snapshot_take a echoue\n");
        return false;
    }

    bool ok = write_bmp(path, snap);
    lv_draw_buf_destroy(snap);
    return ok;
}

static void usage(const char *argv0)
{
    fprintf(stderr,
            "Usage : %s [--size LxH] [--screenshot FICHIER.bmp]\n"
            "  --size LxH          resolution de la fenetre (defaut 1920x1080)\n"
            "  --screenshot F      rend quelques images, ecrit F puis quitte\n",
            argv0);
}

#endif /* DASH_BACKEND_SDL */

/* ------------------------------------------------------------------ */

int main(int argc, char **argv)
{
#if defined(DASH_BACKEND_SDL)
    int32_t win_w = 1920, win_h = 1080;
    const char *shot_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
            int w, h;
            if (sscanf(argv[++i], "%dx%d", &w, &h) != 2 || w <= 0 || h <= 0) {
                fprintf(stderr, "Taille invalide : %s\n", argv[i]);
                return 2;
            }
            win_w = w;
            win_h = h;
        }
        else if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
            shot_path = argv[++i];
        }
        else {
            usage(argv[0]);
            return 2;
        }
    }
#else
    LV_UNUSED(argc);
    LV_UNUSED(argv);
#endif

    install_signal_handlers();
    lv_init();

#if defined(DASH_BACKEND_DRM)
    lv_display_t *disp = open_display();
#else
    lv_display_t *disp = lv_sdl_window_create(win_w, win_h);
    if (disp != NULL) {
        lv_sdl_window_set_title(disp, "lvgl_dashboard (dev)");
    }
#endif

    if (disp == NULL) {
        fprintf(stderr, "Aucun affichage disponible, abandon\n");
        lv_deinit();
        return 1;
    }

    create_dashboard();

    uint32_t last = millis();
#if defined(DASH_BACKEND_SDL)
    uint32_t frames = 0;
#endif

    while (!should_quit) {
        uint32_t now = millis();
        lv_tick_inc(now - last);
        last = now;

        /* Le delai renvoye indique quand le prochain timer sera pret : dormir
         * jusque-la evite de reveiller le CPU pour rien, ce qui fausserait la
         * mesure de charge que ce dashboard affiche justement. */
        uint32_t sleep_ms = lv_timer_handler();
        if (sleep_ms == LV_NO_TIMER_READY || sleep_ms > LOOP_MAX_SLEEP_MS) {
            sleep_ms = LOOP_MAX_SLEEP_MS;
        }
        else if (sleep_ms < LOOP_MIN_SLEEP_MS) {
            sleep_ms = LOOP_MIN_SLEEP_MS;
        }

#if defined(DASH_BACKEND_SDL)
        /* Quelques images suffisent pour que le layout soit stabilise. */
        if (shot_path != NULL && ++frames >= 3) {
            bool ok = take_screenshot(shot_path);
            lv_display_delete(disp);
            lv_deinit();
            return ok ? 0 : 1;
        }
#endif

        usleep(sleep_ms * 1000u);
    }

    fprintf(stderr, "Signal recu : arret propre\n");
    lv_display_delete(disp);
    lv_deinit();
    return 0;
}
