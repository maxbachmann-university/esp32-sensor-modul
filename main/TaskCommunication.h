#ifndef __EVENT_GROUP_TASK__
#define __EVENT_GROUP_TASK__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_event_loop.h"
#include "esp_err.h"

/*  inform c++ compilers that the function should be compiled in C Style */
#ifdef __cplusplus
extern "C" {
#endif

extern EventGroupHandle_t sleep_event_handle;
extern xQueueHandle brightness_queue;
extern const int MESSAGE_SEND_BIT;
extern const int UPDATE_CHECKED_BIT;

esp_err_t communication_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __EVENT_GROUP_TASK__ */
