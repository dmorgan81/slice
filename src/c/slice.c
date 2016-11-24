#include <pebble.h>
#include <pebble-events/pebble-events.h>
#include <pebble-connection-vibes/connection-vibes.h>
#include <pebble-hourly-vibes/hourly-vibes.h>
#include "logging.h"
#include "enamel.h"

static Window *s_window;
static Layer *s_hour_layer;
static Layer *s_minute_layer;
static Layer *s_center_layer;

static struct tm s_tick_time;
static GFont s_font;

static EventHandle s_tick_timer_event_handle;
static EventHandle s_settings_event_handle;

static inline GRect grect_from_point(GPoint point, GSize size) {
    log_func();
    return (GRect) {
        .origin = (GPoint) {
            .x = point.x - size.w / 2,
            .y = point.y - size.h / 2
        },
        .size = size
    };
}

static void settings_handler(void *context) {
    log_func();
    connection_vibes_set_state(atoi(enamel_get_CONNECTION_VIBE()));
    hourly_vibes_set_enabled(enamel_get_HOURLY_VIBE());
#ifdef PBL_HEALTH
    connection_vibes_enable_health(enamel_get_ENABLE_HEALTH());
    hourly_vibes_enable_health(enamel_get_ENABLE_HEALTH());
#endif
    layer_mark_dirty(window_get_root_layer(s_window));
}

static void hour_update_proc(Layer *layer, GContext *ctx) {
    log_func();
    GRect bounds = layer_get_bounds(layer);
    int hour = s_tick_time.tm_hour > 12 ? s_tick_time.tm_hour % 12 : s_tick_time.tm_hour;

    int32_t angle = TRIG_MAX_ANGLE * hour / 12;
    GRect rect = grect_from_point(grect_center_point(&bounds), (GSize) { .w = 20, .h = 20 });
    GPoint origin = gpoint_from_polar(rect, GOvalScaleModeFitCircle, angle);
    bounds = grect_from_point(origin, grect_crop(bounds, 10).size);

    int32_t angle_start = TRIG_MAX_ANGLE * (hour - 0.45) / 12;
    int32_t angle_end = TRIG_MAX_ANGLE * (hour + 0.45) / 12;

    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, bounds.size.w / 2, angle_start, angle_end);

    GRect crop = grect_crop(bounds, PBL_IF_ROUND_ELSE(15, 10));
    rect = grect_centered_from_polar(crop, GOvalScaleModeFitCircle, angle, (GSize) {
        .w = 25,
        .h = 25
    });
    static char buf[3];
    snprintf(buf, sizeof(buf), "%d", hour);

    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(ctx, buf, s_font, rect, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

static void minute_update_proc(Layer *layer, GContext *ctx) {
    log_func();
    GRect bounds = layer_get_bounds(layer);
    int min = s_tick_time.tm_min;

    int32_t angle = TRIG_MAX_ANGLE * min / 60;
    GPoint point = gpoint_from_polar(bounds, GOvalScaleModeFitCircle, angle);

    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_stroke_width(ctx, 6);
    graphics_draw_line(ctx, grect_center_point(&bounds), point);

    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(enamel_get_COLOR_MINUTE_HAND(), GColorWhite));
    graphics_context_set_stroke_width(ctx, 4);
    graphics_draw_line(ctx, grect_center_point(&bounds), point);
}

static void center_update_proc(Layer *layer, GContext *ctx) {
    log_func();
    GRect bounds = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(enamel_get_COLOR_MINUTE_HAND(), GColorWhite));
    graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, bounds.size.w, 0, DEG_TO_TRIGANGLE(360));

    bounds = grect_from_point(grect_center_point(&bounds), (GSize) { .w = 5, .h = 5 });
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, bounds.size.w, 0, DEG_TO_TRIGANGLE(360));
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    log_func();
    memcpy(&s_tick_time, tick_time, sizeof(struct tm));
#ifdef DEMO
    s_tick_time.tm_hour = 11;
    s_tick_time.tm_min = 10;
#endif
    layer_mark_dirty(window_get_root_layer(s_window));
}

static void window_load(Window *window) {
    log_func();
    window_set_background_color(window, GColorBlack);

    Layer *root_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(root_layer);

    s_hour_layer = layer_create(bounds);
    layer_set_update_proc(s_hour_layer, hour_update_proc);
    layer_add_child(root_layer, s_hour_layer);

    s_minute_layer = layer_create(grect_crop(bounds, PBL_IF_ROUND_ELSE(30, 20)));
    layer_set_update_proc(s_minute_layer, minute_update_proc);
    layer_add_child(root_layer, s_minute_layer);

    s_center_layer = layer_create(grect_from_point(grect_center_point(&bounds), (GSize) { .w = 11, .h = 11 }));
    layer_set_update_proc(s_center_layer, center_update_proc);
    layer_add_child(root_layer, s_center_layer);

    time_t now = time(NULL);
    tick_handler(localtime(&now), MINUTE_UNIT);
    s_tick_timer_event_handle = events_tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

    settings_handler(NULL);
    s_settings_event_handle = enamel_settings_received_subscribe(settings_handler, NULL);
}

static void window_unload(Window *window) {
    log_func();
    enamel_settings_received_unsubscribe(s_settings_event_handle);
    events_tick_timer_service_unsubscribe(s_tick_timer_event_handle);

    layer_destroy(s_center_layer);
    layer_destroy(s_minute_layer);
    layer_destroy(s_hour_layer);
}

static void init(void) {
    log_func();
    enamel_init();
    connection_vibes_init();
    hourly_vibes_init();
    uint32_t const pattern[] = { 100 };
    hourly_vibes_set_pattern((VibePattern) {
        .durations = pattern,
        .num_segments = 1
    });

    events_app_message_open();

    s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_GILROY_22));

    s_window = window_create();
    window_set_window_handlers(s_window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload
    });
    window_stack_push(s_window, true);
}

static void deinit(void) {
    log_func();
    window_destroy(s_window);

    fonts_unload_custom_font(s_font);

    hourly_vibes_deinit();
    connection_vibes_deinit();
    enamel_deinit();
}

int main(void) {
    log_func();
    init();
    app_event_loop();
    deinit();
}
