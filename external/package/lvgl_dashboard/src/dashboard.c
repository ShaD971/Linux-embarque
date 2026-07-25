#include "dashboard.h"
#include "sysinfo.h"
#include "lvgl.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define HISTORY_POINTS    60
#define REFRESH_PERIOD_MS 1000

/* Palette : fond tres sombre, cartes legerement plus claires, accents vifs.
 * Pensee pour un ecran HDMI regarde de loin, sans interaction. */
#define COL_BG          lv_color_hex(0x0d1117)
#define COL_CARD        lv_color_hex(0x161b22)
#define COL_CARD_EDGE   lv_color_hex(0x232b36)
#define COL_TRACK       lv_color_hex(0x2a323d)
#define COL_TEXT        lv_color_hex(0xe6edf3)
#define COL_MUTED       lv_color_hex(0x8b949e)
#define COL_CPU         lv_color_hex(0x4a9eff)
#define COL_RAM         lv_color_hex(0x2dd4a7)
#define COL_OK          lv_color_hex(0x3fb950)
#define COL_WARN        lv_color_hex(0xd29922)
#define COL_CRIT        lv_color_hex(0xf85149)

/* Seuils de temperature du SoC (BCM2711) : throttling logiciel a 80 C. */
#define TEMP_WARN_C 62.0f
#define TEMP_CRIT_C 75.0f

typedef struct {
    lv_obj_t *arc;
    lv_obj_t *value;
    lv_obj_t *caption;
} gauge_t;

static struct {
    lv_obj_t *host_label;
    lv_obj_t *uptime_label;
    lv_obj_t *status_dot;
    lv_obj_t *clock_label;
    lv_obj_t *date_label;

    gauge_t cpu;
    gauge_t ram;
    gauge_t temp;

    lv_obj_t *net_state;
    lv_obj_t *net_rx;
    lv_obj_t *net_tx;

    lv_obj_t *chart;
    lv_chart_series_t *cpu_series;
    lv_chart_series_t *ram_series;
} ui;

/* ------------------------------------------------------------------ */
/* Helpers de construction                                             */
/* ------------------------------------------------------------------ */

/* Conteneur transparent : sert uniquement a grouper/aligner. */
static lv_obj_t *make_box(lv_obj_t *parent, lv_flex_flow_t flow, int32_t gap)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_set_style_pad_row(box, gap, 0);
    lv_obj_set_style_pad_column(box, gap, 0);
    lv_obj_set_flex_flow(box, flow);
    lv_obj_set_scrollable(box, false);
    return box;
}

static lv_obj_t *make_card(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_style_bg_color(card, COL_CARD, 0);
    lv_obj_set_style_radius(card, 18, 0);
    lv_obj_set_style_border_color(card, COL_CARD_EDGE, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_pad_all(card, 18, 0);
    lv_obj_set_style_pad_row(card, 6, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollable(card, false);
    return card;
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text,
                            const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);
    return label;
}

/* Carte KPI : titre, jauge circulaire avec la valeur au centre, legende. */
static void make_gauge_card(lv_obj_t *parent, const char *title,
                            lv_color_t accent, gauge_t *out)
{
    lv_obj_t *card = make_card(parent);
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    make_label(card, title, &lv_font_montserrat_16, COL_MUTED);

    lv_obj_t *arc = lv_arc_create(card);
    lv_obj_set_size(arc, 132, 132);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_set_clickable(arc, false);
    lv_obj_set_style_arc_width(arc, 12, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, COL_TRACK, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, accent, LV_PART_INDICATOR);

    lv_obj_t *value = make_label(arc, "--", &lv_font_montserrat_28, COL_TEXT);
    lv_obj_center(value);

    out->arc = arc;
    out->value = value;
    out->caption = make_label(card, "-", &lv_font_montserrat_14, COL_MUTED);
}

/* Ligne "symbole  valeur", utilisee dans la carte reseau. */
static lv_obj_t *make_stat_row(lv_obj_t *parent, const char *symbol,
                               lv_color_t symbol_color)
{
    lv_obj_t *row = make_box(parent, LV_FLEX_FLOW_ROW, 10);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    make_label(row, symbol, &lv_font_montserrat_16, symbol_color);
    return make_label(row, "-", &lv_font_montserrat_20, COL_TEXT);
}

/* ------------------------------------------------------------------ */
/* Sections                                                            */
/* ------------------------------------------------------------------ */

static void build_header(lv_obj_t *parent)
{
    lv_obj_t *bar = make_box(parent, LV_FLEX_FLOW_ROW, 0);
    lv_obj_set_width(bar, LV_PCT(100));
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Bloc gauche : pastille d'etat, hostname, uptime */
    lv_obj_t *left = make_box(bar, LV_FLEX_FLOW_ROW, 12);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *dot = lv_obj_create(left);
    lv_obj_set_size(dot, 14, 14);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, COL_OK, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_scrollable(dot, false);
    ui.status_dot = dot;

    lv_obj_t *names = make_box(left, LV_FLEX_FLOW_COLUMN, 2);
    ui.host_label = make_label(names, sysinfo_hostname(),
                               &lv_font_montserrat_20, COL_TEXT);
    ui.uptime_label = make_label(names, "demarrage...",
                                 &lv_font_montserrat_14, COL_MUTED);

    /* Bloc droit : horloge et date, alignes a droite */
    lv_obj_t *right = make_box(bar, LV_FLEX_FLOW_COLUMN, 2);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);

    ui.clock_label = make_label(right, "--:--:--",
                                &lv_font_montserrat_40, COL_TEXT);
    ui.date_label = make_label(right, "-", &lv_font_montserrat_16, COL_MUTED);
}

static void build_network_card(lv_obj_t *parent)
{
    lv_obj_t *card = make_card(parent);
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_style_pad_row(card, 10, 0);

    make_label(card, "RESEAU", &lv_font_montserrat_16, COL_MUTED);
    ui.net_state = make_label(card, "recherche...",
                              &lv_font_montserrat_20, COL_TEXT);

    ui.net_rx = make_stat_row(card, LV_SYMBOL_DOWNLOAD, COL_CPU);
    ui.net_tx = make_stat_row(card, LV_SYMBOL_UPLOAD, COL_RAM);
}

/* Trait de couleur + libelle, pour la legende du graphique. */
static void add_legend_entry(lv_obj_t *parent, const char *text, lv_color_t color)
{
    lv_obj_t *entry = make_box(parent, LV_FLEX_FLOW_ROW, 8);
    lv_obj_set_flex_align(entry, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *swatch = lv_obj_create(entry);
    lv_obj_set_size(swatch, 16, 4);
    lv_obj_set_style_radius(swatch, 2, 0);
    lv_obj_set_style_bg_color(swatch, color, 0);
    lv_obj_set_style_border_width(swatch, 0, 0);
    lv_obj_set_scrollable(swatch, false);

    make_label(entry, text, &lv_font_montserrat_14, COL_MUTED);
}

static void build_chart_card(lv_obj_t *parent)
{
    lv_obj_t *card = make_card(parent);
    lv_obj_set_width(card, LV_PCT(100));
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_style_pad_row(card, 12, 0);

    lv_obj_t *head = make_box(card, LV_FLEX_FLOW_ROW, 20);
    lv_obj_set_width(head, LV_PCT(100));
    lv_obj_set_flex_align(head, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    make_label(head, "CHARGE SYSTEME  -  60 DERNIERES SECONDES",
               &lv_font_montserrat_16, COL_MUTED);

    lv_obj_t *legend = make_box(head, LV_FLEX_FLOW_ROW, 18);
    add_legend_entry(legend, "CPU", COL_CPU);
    add_legend_entry(legend, "RAM", COL_RAM);

    lv_obj_t *chart = lv_chart_create(card);
    lv_obj_set_width(chart, LV_PCT(100));
    lv_obj_set_flex_grow(chart, 1);
    lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_obj_set_style_pad_all(chart, 0, 0);
    lv_obj_set_style_line_color(chart, COL_TRACK, LV_PART_MAIN);
    lv_obj_set_style_line_width(chart, 1, LV_PART_MAIN);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, HISTORY_POINTS);
    lv_chart_set_div_line_count(chart, 5, 0);
    lv_chart_set_axis_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
    /* Aucun point dessine : seule la ligne compte, plus lisible de loin. */
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);
    lv_obj_set_style_line_width(chart, 3, LV_PART_ITEMS);
    lv_obj_set_scrollable(chart, false);

    ui.chart = chart;
    ui.cpu_series = lv_chart_add_series(chart, COL_CPU, LV_CHART_AXIS_PRIMARY_Y);
    ui.ram_series = lv_chart_add_series(chart, COL_RAM, LV_CHART_AXIS_PRIMARY_Y);
}

/* ------------------------------------------------------------------ */
/* Mise a jour                                                         */
/* ------------------------------------------------------------------ */

static lv_color_t temp_color(float temp_c)
{
    if (temp_c < 0) {
        return COL_MUTED;
    }
    if (temp_c < TEMP_WARN_C) {
        return COL_OK;
    }
    if (temp_c < TEMP_CRIT_C) {
        return COL_WARN;
    }
    return COL_CRIT;
}

static const char *temp_caption(float temp_c)
{
    if (temp_c < 0) {
        return "capteur absent";
    }
    if (temp_c < TEMP_WARN_C) {
        return "nominal";
    }
    if (temp_c < TEMP_CRIT_C) {
        return "eleve";
    }
    return "throttling proche";
}

static void format_uptime(uint64_t seconds, char *buf, size_t size)
{
    unsigned long days = (unsigned long)(seconds / 86400u);
    unsigned long hours = (unsigned long)((seconds % 86400u) / 3600u);
    unsigned long minutes = (unsigned long)((seconds % 3600u) / 60u);

    if (days > 0) {
        snprintf(buf, size, "actif depuis %lu j %lu h", days, hours);
    }
    else if (hours > 0) {
        snprintf(buf, size, "actif depuis %lu h %lu min", hours, minutes);
    }
    else {
        snprintf(buf, size, "actif depuis %lu min", minutes);
    }
}

static void format_rate(double kbps, char *buf, size_t size)
{
    if (kbps >= 1024.0) {
        snprintf(buf, size, "%.1f Mo/s", kbps / 1024.0);
    }
    else {
        snprintf(buf, size, "%.0f Ko/s", kbps);
    }
}

/* Renseigne une jauge graduee 0-100. `suffix` s'affiche apres la valeur. */
static void set_gauge(gauge_t *gauge, float value, const char *suffix,
                      const char *caption)
{
    if (value < 0) {
        lv_arc_set_value(gauge->arc, 0);
        lv_label_set_text(gauge->value, "N/A");
        lv_label_set_text(gauge->caption, "indisponible");
        return;
    }

    int32_t rounded = (int32_t)(value + 0.5f);
    if (rounded > 100) {
        rounded = 100;
    }

    lv_arc_set_value(gauge->arc, rounded);
    lv_label_set_text_fmt(gauge->value, "%d%s", (int)rounded, suffix);
    if (caption != NULL) {
        lv_label_set_text(gauge->caption, caption);
    }
}

static void update_header(const sysinfo_t *info)
{
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);

    char time_buf[16];
    char date_buf[64];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &local);
    strftime(date_buf, sizeof(date_buf), "%A %d %B %Y", &local);

    lv_label_set_text(ui.clock_label, time_buf);
    lv_label_set_text(ui.date_label, date_buf);

    if (info->uptime_s > 0) {
        char uptime_buf[48];
        format_uptime(info->uptime_s, uptime_buf, sizeof(uptime_buf));
        lv_label_set_text(ui.uptime_label, uptime_buf);
    }

    /* La pastille resume l'etat global : la temperature prime sur le CPU. */
    lv_color_t health = COL_OK;
    if (info->soc_temp_c >= TEMP_CRIT_C || info->cpu_usage_pct >= 95.0f) {
        health = COL_CRIT;
    }
    else if (info->soc_temp_c >= TEMP_WARN_C || info->cpu_usage_pct >= 80.0f) {
        health = COL_WARN;
    }
    lv_obj_set_style_bg_color(ui.status_dot, health, 0);
}

static void update_network(const sysinfo_t *info)
{
    if (!info->net_available) {
        lv_label_set_text(ui.net_state, "aucune interface");
        lv_obj_set_style_text_color(ui.net_state, COL_MUTED, 0);
        lv_label_set_text(ui.net_rx, "-");
        lv_label_set_text(ui.net_tx, "-");
        return;
    }

    if (info->net_up) {
        lv_label_set_text_fmt(ui.net_state, "%s en ligne", info->net_ifname);
        lv_obj_set_style_text_color(ui.net_state, COL_OK, 0);

        char buf[24];
        format_rate(info->net_rx_kbps, buf, sizeof(buf));
        lv_label_set_text(ui.net_rx, buf);
        format_rate(info->net_tx_kbps, buf, sizeof(buf));
        lv_label_set_text(ui.net_tx, buf);
    }
    else {
        lv_label_set_text_fmt(ui.net_state, "%s hors ligne", info->net_ifname);
        lv_obj_set_style_text_color(ui.net_state, COL_CRIT, 0);
        lv_label_set_text(ui.net_rx, "-");
        lv_label_set_text(ui.net_tx, "-");
    }
}

static void refresh_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    sysinfo_t info;
    sysinfo_read(&info);

    update_header(&info);

    char caption[48];

    if (info.load1 >= 0) {
        snprintf(caption, sizeof(caption), "charge %.2f  -  %d coeurs",
                 (double)info.load1, sysinfo_cpu_count());
    }
    else {
        snprintf(caption, sizeof(caption), "%d coeurs", sysinfo_cpu_count());
    }
    set_gauge(&ui.cpu, info.cpu_usage_pct, "%", caption);

    if (info.ram_total_mb > 0) {
        snprintf(caption, sizeof(caption), "%.1f / %.1f Go",
                 (double)info.ram_used_mb / 1024.0,
                 (double)info.ram_total_mb / 1024.0);
    }
    else {
        snprintf(caption, sizeof(caption), "-");
    }
    set_gauge(&ui.ram, info.ram_usage_pct, "%", caption);

    /* Jauge graduee en degres Celsius, pas en pourcentage. */
    set_gauge(&ui.temp, info.soc_temp_c, " C", temp_caption(info.soc_temp_c));
    lv_obj_set_style_arc_color(ui.temp.arc, temp_color(info.soc_temp_c),
                               LV_PART_INDICATOR);

    update_network(&info);

    lv_chart_set_next_value(ui.chart, ui.cpu_series,
                            info.cpu_usage_pct >= 0
                                ? (int32_t)(info.cpu_usage_pct + 0.5f)
                                : LV_CHART_POINT_NONE);
    lv_chart_set_next_value(ui.chart, ui.ram_series,
                            info.ram_usage_pct >= 0
                                ? (int32_t)(info.ram_usage_pct + 0.5f)
                                : LV_CHART_POINT_NONE);
}

/* ------------------------------------------------------------------ */
/* Entree publique                                                     */
/* ------------------------------------------------------------------ */

void create_dashboard(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, COL_BG, 0);
    lv_obj_set_style_pad_all(scr, 28, 0);
    lv_obj_set_style_pad_row(scr, 20, 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollable(scr, false);

    build_header(scr);

    /* Rangee de KPI : chaque carte prend une part egale de la largeur. */
    lv_obj_t *kpi_row = make_box(scr, LV_FLEX_FLOW_ROW, 20);
    lv_obj_set_width(kpi_row, LV_PCT(100));

    make_gauge_card(kpi_row, "PROCESSEUR", COL_CPU, &ui.cpu);
    make_gauge_card(kpi_row, "MEMOIRE", COL_RAM, &ui.ram);
    make_gauge_card(kpi_row, "TEMPERATURE SOC", COL_OK, &ui.temp);
    build_network_card(kpi_row);

    build_chart_card(scr);

    refresh_cb(NULL);
    lv_timer_create(refresh_cb, REFRESH_PERIOD_MS, NULL);
}
