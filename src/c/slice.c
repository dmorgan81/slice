#include "common.h"

static Window *s_window;
static Layer *s_hour_layer;
static Layer *s_minute_layer;
static Layer *s_center_layer;

static struct tm s_tick_time;
static GFont s_font;

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

static void hour_update_proc(Layer *layer, GContext *ctx) {
    log_func();
    GRect bounds = layer_get_bounds(layer);
    int hour = s_tick_time.tm_hour > 12 ? s_tick_time.tm_hour % 12 : s_tick_time.tm_hour;

    int32_t angle = TRIG_MAX_ANGLE * hour / 12;
    GRect rect = grect_from_point(grect_center_point(&bounds), (GSize) { .w = 20, .h = 20 });
    GPoint origin = gpoint_from_polar(rect, GOvalScaleModeFitCircle, angle);
    bounds = grect_from_point(origin, bounds.size);

    int32_t angle_start = TRIG_MAX_ANGLE * (hour - 0.35) / 12;
    int32_t angle_end = TRIG_MAX_ANGLE * (hour + 0.35) / 12;

    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, bounds.size.w / 2, angle_start, angle_end);

    rect = grect_centered_from_polar(grect_crop(bounds, 25), GOvalScaleModeFitCircle, angle, (GSize) {
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

    graphics_context_set_stroke_color(ctx, GColorWhite);
    graphics_context_set_stroke_width(ctx, 4);
    graphics_draw_line(ctx, grect_center_point(&bounds), point);
}

static void center_update_proc(Layer *layer, GContext *ctx) {
    log_func();
    GRect bounds = layer_get_bounds(layer);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, bounds.size.w, 0, DEG_TO_TRIGANGLE(360));

    bounds = grect_from_point(grect_center_point(&bounds), (GSize) { .w = 5, .h = 5 });
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, bounds.size.w, 0, DEG_TO_TRIGANGLE(360));
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    log_func();
    memcpy(&s_tick_time, tick_time, sizeof(struct tm));
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

    s_minute_layer = layer_create(grect_crop(bounds, 30));
    layer_set_update_proc(s_minute_layer, minute_update_proc);
    layer_add_child(root_layer, s_minute_layer);

    s_center_layer = layer_create(grect_from_point(grect_center_point(&bounds), (GSize) { .w = 11, .h = 11 }));
    layer_set_update_proc(s_center_layer, center_update_proc);
    layer_add_child(root_layer, s_center_layer);

    time_t now = time(NULL);
    tick_handler(localtime(&now), MINUTE_UNIT);
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void window_unload(Window *window) {
    log_func();
    tick_timer_service_unsubscribe();

    layer_destroy(s_center_layer);
    layer_destroy(s_minute_layer);
    layer_destroy(s_hour_layer);
}

static void init(void) {
    log_func();
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
}

int main(void) {
    log_func();
    init();
    app_event_loop();
    deinit();
}
