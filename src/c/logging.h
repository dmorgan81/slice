#pragma once

//#define TRACE
//#define DEBUG

#ifdef TRACE
#define logt(fmt, ...) APP_LOG(APP_LOG_LEVEL_DEBUG_VERBOSE, fmt, ##__VA_ARGS__)
#ifndef DEBUG
#define DEBUG
#endif
#else
#define logt(fmt, ...)
#endif

#ifdef DEBUG
#define logd(fmt, ...) APP_LOG(APP_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#else
#define logd(fmt, ...)
#endif

#define logi(fmt, ...) APP_LOG(APP_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define logw(fmt, ...) APP_LOG(APP_LOG_LEVEL_WARNING, fmt, ##__VA_ARGS__)
#define loge(fmt, ...) APP_LOG(APP_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)

#define log_func(void) logt("%s", __func__);
