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

/* Plage de la jauge de temperature, en degres Celsius. Explicite : l'arc n'est
 * pas gradue en pourcentage, il se trouvait seulement que 0..100 convenait aux
 * deux. Le BCM2711 throttle vers 80-85 C, 100 C est donc un plein d'echelle
 * lisible. */
#define TEMP_RANGE_MIN_C 0
#define TEMP_RANGE_MAX_C 100

typedef struct {
    lv_obj_t *arc;
    lv_obj_t *value;
    lv_obj_t *caption;
    int32_t   range_min;
    int32_t   range_max;
    const char *unit;
} gauge_t;

/* Dimensions derivees de la resolution reelle du display. Les valeurs etaient
 * auparavant figees et supposaient implicitement du 1920x1080 : en 800x480 la
 * rangee de 4 cartes debordait, et les conteneurs n'etant pas scrollables, le
 * surplus etait simplement coupe. */
typedef struct {
    int32_t pad;
    int32_t gap;
    int32_t card_pad;
    int32_t card_radius;
    int32_t arc_size;
    int32_t arc_width;
    int32_t dot_size;
    int32_t chart_line;
    int32_t swatch_w;
    int32_t swatch_h;
    bool    compact;
    const lv_font_t *f_small;
    const lv_font_t *f_title;
    const lv_font_t *f_value;
    const lv_font_t *f_host;
    const lv_font_t *f_clock;
} metrics_t;

static metrics_t M;

/* Trois paliers : ~800x480, ~1280x720, ~1920x1080. Le choix se fait sur la
 * largeur, c'est elle qui contraint la rangee de 4 cartes. */
static void pick_metrics(int32_t hor_res)
{
    if (hor_res < 1024) {
        M = (metrics_t){
            .pad = 12, .gap = 10, .card_pad = 10, .card_radius = 12,
            .arc_size = 76, .arc_width = 8, .dot_size = 10,
            .chart_line = 2, .swatch_w = 12, .swatch_h = 3,
            .compact = true,
            .f_small = &lv_font_montserrat_14,
            .f_title = &lv_font_montserrat_14,
            .f_value = &lv_font_montserrat_20,
            .f_host  = &lv_font_montserrat_16,
            .f_clock = &lv_font_montserrat_28,
        };
    }
    else if (hor_res < 1600) {
        M = (metrics_t){
            .pad = 20, .gap = 14, .card_pad = 14, .card_radius = 16,
            .arc_size = 108, .arc_width = 10, .dot_size = 12,
            .chart_line = 2, .swatch_w = 14, .swatch_h = 4,
            .compact = false,
            .f_small = &lv_font_montserrat_14,
            .f_title = &lv_font_montserrat_16,
            .f_value = &lv_font_montserrat_28,
            .f_host  = &lv_font_montserrat_20,
            .f_clock = &lv_font_montserrat_40,
        };
    }
    else {
        M = (metrics_t){
            .pad = 28, .gap = 20, .card_pad = 18, .card_radius = 18,
            .arc_size = 132, .arc_width = 12, .dot_size = 14,
            .chart_line = 3, .swatch_w = 16, .swatch_h = 4,
            .compact = false,
            .f_small = &lv_font_montserrat_14,
            .f_title = &lv_font_montserrat_16,
            .f_value = &lv_font_montserrat_28,
            .f_host  = &lv_font_montserrat_20,
            .f_clock = &lv_font_montserrat_40,
        };
    }
}

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

/* Hauteur necessaire a une carte KPI : sans hauteur explicite, les cartes
 * heritaient de la taille par defaut du theme (~130 px), qui ne suit pas la
 * resolution. En 800x480 l'arc de 76 px y tenait par chance, mais en 1920x1080
 * l'arc de 132 px plus le titre et la legende debordaient, et comme les
 * conteneurs ne sont pas scrollables, titre et legende etaient simplement
 * coupes. */
static int32_t kpi_row_height(void)
{
    const int32_t inner_gap = M.compact ? 4 : 6;

    return 2 * M.card_pad                /* padding haut + bas */
           + 2                           /* bordures */
           + M.f_title->line_height      /* titre */
           + inner_gap
           + M.arc_size
           + inner_gap
           + M.f_small->line_height;     /* legende */
}

static lv_obj_t *make_card(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_style_bg_color(card, COL_CARD, 0);
    lv_obj_set_style_radius(card, M.card_radius, 0);
    lv_obj_set_style_border_color(card, COL_CARD_EDGE, 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_pad_all(card, M.card_pad, 0);
    lv_obj_set_style_pad_row(card, M.compact ? 4 : 6, 0);
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

/* Carte KPI : titre, jauge circulaire avec la valeur au centre, legende.
 * `range_min`/`range_max` sont l'echelle de l'arc et `unit` le suffixe affiche :
 * les trois jauges n'ont pas la meme unite. */
static void make_gauge_card(lv_obj_t *parent, const char *title,
                            lv_color_t accent, int32_t range_min,
                            int32_t range_max, const char *unit, gauge_t *out)
{
    lv_obj_t *card = make_card(parent);
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_height(card, LV_PCT(100));
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    make_label(card, title, M.f_title, COL_MUTED);

    lv_obj_t *arc = lv_arc_create(card);
    lv_obj_set_size(arc, M.arc_size, M.arc_size);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_range(arc, range_min, range_max);
    lv_arc_set_value(arc, range_min);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_set_clickable(arc, false);
    lv_obj_set_style_arc_width(arc, M.arc_width, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, M.arc_width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, COL_TRACK, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, accent, LV_PART_INDICATOR);

    lv_obj_t *value = make_label(arc, "--", M.f_value, COL_TEXT);
    lv_obj_center(value);

    out->arc = arc;
    out->value = value;
    out->caption = make_label(card, "-", M.f_small, COL_MUTED);
    out->range_min = range_min;
    out->range_max = range_max;
    out->unit = unit;
}

/* Ligne "symbole  valeur", utilisee dans la carte reseau. */
static lv_obj_t *make_stat_row(lv_obj_t *parent, const char *symbol,
                               lv_color_t symbol_color)
{
    lv_obj_t *row = make_box(parent, LV_FLEX_FLOW_ROW, M.compact ? 6 : 10);
    lv_obj_set_width(row, LV_PCT(100));
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    make_label(row, symbol, M.f_title, symbol_color);
    return make_label(row, "-", M.f_host, COL_TEXT);
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
    lv_obj_t *left = make_box(bar, LV_FLEX_FLOW_ROW, M.compact ? 8 : 12);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *dot = lv_obj_create(left);
    lv_obj_set_size(dot, M.dot_size, M.dot_size);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, COL_OK, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_scrollable(dot, false);
    ui.status_dot = dot;

    lv_obj_t *names = make_box(left, LV_FLEX_FLOW_COLUMN, 2);
    ui.host_label = make_label(names, sysinfo_hostname(), M.f_host, COL_TEXT);
    ui.uptime_label = make_label(names, "demarrage...", M.f_small, COL_MUTED);

    /* Bloc droit : horloge et date, alignes a droite */
    lv_obj_t *right = make_box(bar, LV_FLEX_FLOW_COLUMN, 2);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);

    ui.clock_label = make_label(right, "--:--:--", M.f_clock, COL_TEXT);
    ui.date_label = make_label(right, "-", M.f_title, COL_MUTED);
}

static void build_network_card(lv_obj_t *parent)
{
    lv_obj_t *card = make_card(parent);
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_height(card, LV_PCT(100));
    lv_obj_set_style_pad_row(card, M.compact ? 6 : 10, 0);

    make_label(card, "RESEAU", M.f_title, COL_MUTED);
    ui.net_state = make_label(card, "recherche...", M.f_host, COL_TEXT);

    ui.net_rx = make_stat_row(card, LV_SYMBOL_DOWNLOAD, COL_CPU);
    ui.net_tx = make_stat_row(card, LV_SYMBOL_UPLOAD, COL_RAM);
}

/* Trait de couleur + libelle, pour la legende du graphique. */
static void add_legend_entry(lv_obj_t *parent, const char *text, lv_color_t color)
{
    lv_obj_t *entry = make_box(parent, LV_FLEX_FLOW_ROW, M.compact ? 6 : 8);
    lv_obj_set_flex_align(entry, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *swatch = lv_obj_create(entry);
    lv_obj_set_size(swatch, M.swatch_w, M.swatch_h);
    lv_obj_set_style_radius(swatch, 2, 0);
    lv_obj_set_style_bg_color(swatch, color, 0);
    lv_obj_set_style_border_width(swatch, 0, 0);
    lv_obj_set_scrollable(swatch, false);

    make_label(entry, text, M.f_small, COL_MUTED);
}

static void build_chart_card(lv_obj_t *parent)
{
    lv_obj_t *card = make_card(parent);
    lv_obj_set_width(card, LV_PCT(100));
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_style_pad_row(card, M.compact ? 8 : 12, 0);

    lv_obj_t *head = make_box(card, LV_FLEX_FLOW_ROW, M.gap);
    lv_obj_set_width(head, LV_PCT(100));
    lv_obj_set_flex_align(head, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* La fenetre affichee vaut HISTORY_POINTS * REFRESH_PERIOD_MS : on la
     * calcule au lieu de la reecrire, pour qu'elle ne mente jamais. */
    lv_obj_t *title = make_label(head, "", M.f_title, COL_MUTED);
    lv_label_set_text_fmt(title, M.compact ? "CHARGE  -  %d s"
                                           : "CHARGE SYSTEME  -  %d DERNIERES SECONDES",
                          (HISTORY_POINTS * REFRESH_PERIOD_MS) / 1000);

    lv_obj_t *legend = make_box(head, LV_FLEX_FLOW_ROW, M.compact ? 12 : 18);
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
    lv_obj_set_style_line_width(chart, M.chart_line, LV_PART_ITEMS);
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

/* Renseigne une jauge. L'arc est borne a sa propre plage ; la valeur affichee
 * reste la mesure reelle, seule la position de l'aiguille est ecretee. */
static void set_gauge(gauge_t *gauge, float value, const char *caption)
{
    if (value < 0) {
        lv_arc_set_value(gauge->arc, gauge->range_min);
        lv_label_set_text(gauge->value, "N/A");
        lv_label_set_text(gauge->caption, "indisponible");
        return;
    }

    int32_t rounded = (int32_t)(value + 0.5f);

    int32_t clamped = rounded;
    if (clamped > gauge->range_max) {
        clamped = gauge->range_max;
    }
    else if (clamped < gauge->range_min) {
        clamped = gauge->range_min;
    }

    lv_arc_set_value(gauge->arc, clamped);
    lv_label_set_text_fmt(gauge->value, "%d%s", (int)rounded, gauge->unit);
    if (caption != NULL) {
        lv_label_set_text(gauge->caption, caption);
    }
}

/* Noms francais sans accents, en dur : sur une image Buildroot minimale la
 * locale est "C" (strftime rendrait l'anglais), et une locale fr_FR produirait
 * des accents absents des polices Montserrat integrees, donc des carres vides.
 * Ne depend d'aucun setlocale(). */
static const char *const WEEKDAYS_FR[7] = {
    "dimanche", "lundi", "mardi", "mercredi", "jeudi", "vendredi", "samedi"
};

static const char *const MONTHS_FR[12] = {
    "janvier", "fevrier", "mars", "avril", "mai", "juin",
    "juillet", "aout", "septembre", "octobre", "novembre", "decembre"
};

static void format_date_fr(const struct tm *t, char *buf, size_t size)
{
    int wday = (t->tm_wday >= 0 && t->tm_wday < 7) ? t->tm_wday : 0;
    int mon = (t->tm_mon >= 0 && t->tm_mon < 12) ? t->tm_mon : 0;

    snprintf(buf, size, "%s %d %s %d", WEEKDAYS_FR[wday], t->tm_mday,
             MONTHS_FR[mon], t->tm_year + 1900);
}

static void update_header(const sysinfo_t *info)
{
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);

    char time_buf[16];
    char date_buf[64];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &local);
    format_date_fr(&local, date_buf, sizeof(date_buf));

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
    set_gauge(&ui.cpu, info.cpu_usage_pct, caption);

    if (info.ram_total_mb > 0) {
        snprintf(caption, sizeof(caption), "%.1f / %.1f Go",
                 (double)info.ram_used_mb / 1024.0,
                 (double)info.ram_total_mb / 1024.0);
    }
    else {
        snprintf(caption, sizeof(caption), "-");
    }
    set_gauge(&ui.ram, info.ram_usage_pct, caption);

    set_gauge(&ui.temp, info.soc_temp_c, temp_caption(info.soc_temp_c));
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
    /* Le layout est derive de la resolution reelle du display, connue seulement
     * une fois le backend initialise. */
    pick_metrics(lv_display_get_horizontal_resolution(lv_display_get_default()));

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, COL_BG, 0);
    lv_obj_set_style_pad_all(scr, M.pad, 0);
    lv_obj_set_style_pad_row(scr, M.gap, 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollable(scr, false);

    build_header(scr);

    /* Rangee de KPI : chaque carte prend une part egale de la largeur. */
    lv_obj_t *kpi_row = make_box(scr, LV_FLEX_FLOW_ROW, M.gap);
    lv_obj_set_width(kpi_row, LV_PCT(100));
    lv_obj_set_height(kpi_row, kpi_row_height());

    make_gauge_card(kpi_row, "PROCESSEUR", COL_CPU, 0, 100, "%", &ui.cpu);
    make_gauge_card(kpi_row, "MEMOIRE", COL_RAM, 0, 100, "%", &ui.ram);
    make_gauge_card(kpi_row, M.compact ? "TEMP SOC" : "TEMPERATURE SOC", COL_OK,
                    TEMP_RANGE_MIN_C, TEMP_RANGE_MAX_C, " C", &ui.temp);
    build_network_card(kpi_row);

    build_chart_card(scr);

    refresh_cb(NULL);
    lv_timer_create(refresh_cb, REFRESH_PERIOD_MS, NULL);
}
