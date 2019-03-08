#include "TaskCommunication.h"

EventGroupHandle_t sleep_event_handle = NULL;
xQueueHandle brightness_queue = NULL;

const int MESSAGE_SEND_BIT = BIT0;
const int UPDATE_CHECKED_BIT = BIT1;

esp_err_t communication_init(void)
{
    sleep_event_handle = xEventGroupCreate();
    brightness_queue = xQueueCreate(1, sizeof(uint8_t));
    xEventGroupClearBits(sleep_event_handle, MESSAGE_SEND_BIT | UPDATE_CHECKED_BIT);
    return ESP_OK;
}
