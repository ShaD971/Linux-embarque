#include "dashboard.h"
#include "sysinfo.h"
#include "lvgl.h"

#include <time.h>

#define CHART_POINT_COUNT 30
#define REFRESH_PERIOD_MS 1000

#define COLOR_BG        lv_color_hex(0x121212)
#define COLOR_CARD_BG   lv_color_hex(0x1e1e1e)
#define COLOR_TRACK     lv_color_hex(0x333333)
#define COLOR_MUTED     lv_color_hex(0x9e9e9e)
#define COLOR_TEXT      lv_color_hex(0xffffff)

typedef struct {
    lv_obj_t *arc;
    lv_obj_t *value_label;
} gauge_t;

static struct {
    gauge_t cpu;
    gauge_t ram;
    gauge_t temp;

    lv_obj_t *net_state_label;
    lv_obj_t *net_detail_label;

    lv_obj_t *clock_time_label;
    lv_obj_t *clock_date_label;

    lv_obj_t *chart;
    lv_chart_series_t *chart_series;
} dash;

static lv_obj_t *create_card_base(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_style_bg_color(card, COLOR_CARD_BG, 0);
    lv_obj_set_style_radius(card, 16, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 14, 0);
    lv_obj_set_style_pad_row(card, 4, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

static lv_obj_t *create_card_title(lv_obj_t *card, const char *title)
{
    lv_obj_t *label = lv_label_create(card);
    lv_label_set_text(label, title);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label, COLOR_MUTED, 0);
    return label;
}

/* Carte avec jauge circulaire (CPU, RAM, temperature) */
static lv_obj_t *create_gauge_card(lv_obj_t *parent, const char *title, lv_color_t accent,
                                    gauge_t *out)
{
    lv_obj_t *card = create_card_base(parent);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    create_card_title(card, title);

    lv_obj_t *arc = lv_arc_create(card);
    lv_obj_set_size(arc, 84, 84);
    lv_arc_set_rotation(arc, 135);
    lv_arc_set_bg_angles(arc, 0, 270);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(arc, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, COLOR_TRACK, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, accent, LV_PART_INDICATOR);

    lv_obj_t *value_label = lv_label_create(arc);
    lv_obj_center(value_label);
    lv_obj_set_style_text_font(value_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(value_label, COLOR_TEXT, 0);
    lv_label_set_text(value_label, "--");

    out->arc = arc;
    out->value_label = value_label;
    return card;
}

static lv_obj_t *create_network_card(lv_obj_t *parent)
{
    lv_obj_t *card = create_card_base(parent);
    create_card_title(card, "Reseau");

    lv_obj_t *state_label = lv_label_create(card);
    lv_obj_set_style_text_font(state_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(state_label, COLOR_TEXT, 0);
    lv_label_set_text(state_label, "Recherche...");

    lv_obj_t *detail_label = lv_label_create(card);
    lv_obj_set_style_text_font(detail_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(detail_label, COLOR_MUTED, 0);
    lv_label_set_text(detail_label, "-");

    dash.net_state_label = state_label;
    dash.net_detail_label = detail_label;
    return card;
}

static lv_obj_t *create_clock_card(lv_obj_t *parent)
{
    lv_obj_t *card = create_card_base(parent);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    create_card_title(card, "Horloge");

    lv_obj_t *time_label = lv_label_create(card);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(time_label, COLOR_TEXT, 0);
    lv_label_set_text(time_label, "--:--:--");

    lv_obj_t *date_label = lv_label_create(card);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(date_label, COLOR_MUTED, 0);
    lv_label_set_text(date_label, "-");

    dash.clock_time_label = time_label;
    dash.clock_date_label = date_label;
    return card;
}

static lv_obj_t *create_chart_card(lv_obj_t *parent)
{
    lv_obj_t *card = create_card_base(parent);
    lv_obj_set_width(card, LV_PCT(100));
    lv_obj_set_flex_grow(card, 1);
    create_card_title(card, "Charge CPU (temps reel)");

    lv_obj_t *chart = lv_chart_create(card);
    lv_obj_set_flex_grow(chart, 1);
    lv_obj_set_width(chart, LV_PCT(100));
    lv_obj_set_style_bg_color(chart, COLOR_CARD_BG, 0);
    lv_obj_set_style_border_width(chart, 0, 0);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, CHART_POINT_COUNT);
    lv_chart_set_div_line_count(chart, 3, 0);
    lv_chart_set_axis_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);

    lv_chart_series_t *series = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BLUE),
                                                      LV_CHART_AXIS_PRIMARY_Y);

    dash.chart = chart;
    dash.chart_series = series;
    return card;
}

static lv_color_t temp_color(float temp_c)
{
    if (temp_c < 0) {
        return COLOR_MUTED;
    }
    if (temp_c < 60.0f) {
        return lv_palette_main(LV_PALETTE_GREEN);
    }
    if (temp_c < 75.0f) {
        return lv_palette_main(LV_PALETTE_ORANGE);
    }
    return lv_palette_main(LV_PALETTE_RED);
}

static void update_gauge(gauge_t *gauge, float value_pct)
{
    if (value_pct < 0) {
        lv_arc_set_value(gauge->arc, 0);
        lv_label_set_text(gauge->value_label, "N/A");
        return;
    }

    int32_t rounded = (int32_t)(value_pct + 0.5f);
    if (rounded > 100) {
        rounded = 100;
    }
    lv_arc_set_value(gauge->arc, rounded);
    lv_label_set_text_fmt(gauge->value_label, "%d%%", (int)rounded);
}

static void update_clock(void)
{
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);

    char time_buf[16];
    char date_buf[32];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", &local);
    strftime(date_buf, sizeof(date_buf), "%A %d %B %Y", &local);

    lv_label_set_text(dash.clock_time_label, time_buf);
    lv_label_set_text(dash.clock_date_label, date_buf);
}

static void update_network(const sysinfo_t *info)
{
    if (!info->net_available) {
        lv_label_set_text(dash.net_state_label, "Indisponible");
        lv_obj_set_style_text_color(dash.net_state_label, COLOR_MUTED, 0);
        lv_label_set_text(dash.net_detail_label, "Aucune interface");
        return;
    }

    if (info->net_up) {
        lv_label_set_text_fmt(dash.net_state_label, "%s : en ligne", info->net_ifname);
        lv_obj_set_style_text_color(dash.net_state_label, lv_palette_main(LV_PALETTE_GREEN), 0);
        lv_label_set_text_fmt(dash.net_detail_label, "%s %.1f Ko/s  %s %.1f Ko/s",
                               LV_SYMBOL_DOWNLOAD, info->net_rx_kbps,
                               LV_SYMBOL_UPLOAD, info->net_tx_kbps);
    }
    else {
        lv_label_set_text_fmt(dash.net_state_label, "%s : hors ligne", info->net_ifname);
        lv_obj_set_style_text_color(dash.net_state_label, lv_palette_main(LV_PALETTE_RED), 0);
        lv_label_set_text(dash.net_detail_label, "Pas de lien");
    }
}

static void refresh_cb(lv_timer_t *timer)
{
    LV_UNUSED(timer);

    sysinfo_t info;
    sysinfo_read(&info);

    update_clock();
    update_gauge(&dash.cpu, info.cpu_usage_pct);
    update_gauge(&dash.ram, info.ram_usage_pct);

    update_gauge(&dash.temp, info.soc_temp_c);
    lv_obj_set_style_arc_color(dash.temp.arc, temp_color(info.soc_temp_c), LV_PART_INDICATOR);

    update_network(&info);

    if (info.cpu_usage_pct >= 0) {
        int32_t rounded = (int32_t)(info.cpu_usage_pct + 0.5f);
        lv_chart_set_next_value(dash.chart, dash.chart_series, rounded);
    }
    else {
        lv_chart_set_next_value(dash.chart, dash.chart_series, LV_CHART_POINT_NONE);
    }
}

void create_dashboard(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, COLOR_BG, 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(scr, 20, 0);
    lv_obj_set_style_pad_row(scr, 16, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* Grille de cartes : horloge, CPU, RAM / temperature, reseau */
    lv_obj_t *grid = lv_obj_create(scr);
    lv_obj_set_width(grid, LV_PCT(100));
    lv_obj_set_height(grid, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_style_pad_all(grid, 0, 0);
    lv_obj_set_style_pad_row(grid, 16, 0);
    lv_obj_set_style_pad_column(grid, 16, 0);
    lv_obj_clear_flag(grid, LV_OBJ_FLAG_SCROLLABLE);

    static const int32_t grid_col_dsc[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static const int32_t grid_row_dsc[] = { 130, 130, LV_GRID_TEMPLATE_LAST };
    lv_obj_set_grid_dsc_array(grid, grid_col_dsc, grid_row_dsc);

    lv_obj_t *clock_card = create_clock_card(grid);
    lv_obj_set_grid_cell(clock_card, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    lv_obj_t *cpu_card = create_gauge_card(grid, "CPU", lv_palette_main(LV_PALETTE_BLUE), &dash.cpu);
    lv_obj_set_grid_cell(cpu_card, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    lv_obj_t *ram_card = create_gauge_card(grid, "RAM", lv_palette_main(LV_PALETTE_TEAL), &dash.ram);
    lv_obj_set_grid_cell(ram_card, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 0, 1);

    lv_obj_t *temp_card = create_gauge_card(grid, "Temperature", lv_palette_main(LV_PALETTE_ORANGE), &dash.temp);
    lv_obj_set_grid_cell(temp_card, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

    lv_obj_t *net_card = create_network_card(grid);
    lv_obj_set_grid_cell(net_card, LV_GRID_ALIGN_STRETCH, 1, 2, LV_GRID_ALIGN_STRETCH, 1, 1);

    /* Graphique temps reel */
    create_chart_card(scr);

    refresh_cb(NULL);
    lv_timer_create(refresh_cb, REFRESH_PERIOD_MS, NULL);
}
