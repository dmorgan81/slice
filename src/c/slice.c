#include "common.h"

static Window *s_window;

static void window_load(Window *window) {
    log_func();
    window_set_background_color(window, GColorBlack);
}

static void window_unload(Window *window) {
    log_func();
}

static void init(void) {
    log_func();
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
}

int main(void) {
    log_func();
    init();
    app_event_loop();
    deinit();
}
