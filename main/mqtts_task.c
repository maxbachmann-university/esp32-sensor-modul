#include "mqtts_task.h"
#include "TaskCommunication.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "TaskCommunication.h"

#define MQTT_TOPIC "blindcontrol/#"
#define MQTT_BLINDS_TOPIC "blindcontrol"

static const char *TAG = "MQTTS_TASK";

/*  mqtt tls certificate */
extern const char tls_cert_pem_start[]   asm("_binary_mqtt_tls_cert_pem_start");
extern const char tls_cert_pem_end[]   asm("_binary_mqtt_tls_cert_pem_end");

uint16_t value = 0;

/**@brief Function handles all MQTT events
 * 
 * @details handles events like receiving data
 */
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch (event->event_id) {
        /*  when connected subscribe to a topic */
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            char buffer[20];
            snprintf(buffer, 20, "{\"value\":%d}", value);
            esp_mqtt_client_publish(client, "blindcontrol/control/brightness", buffer, 0, 0, 0);
            /*  set bit for deep sleep */
            xEventGroupSetBits(sleep_event_handle, MESSAGE_SEND_BIT);
            /*  destroy connection */
            esp_mqtt_client_destroy(client);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

/**@brief Function for initializing the MQTTS Connection
 * 
 * @details starts a MQTTS Connection using Username + Password and TLS.
 */
esp_err_t mqtts_task_init(uint16_t brightness)
{
    value = brightness;

    /*  set all config parameters */
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URI,
        .username = CONFIG_BROKER_USERNAME,
        .password = CONFIG_BROKER_PASSWORD,
        .event_handle = mqtt_event_handler,
        /*  tls not activated on the mqtt broker yet */
        //.cert_pem = (const char *)tls_cert_pem_start,
    };

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_err_t error_code = esp_mqtt_client_start(client);
    ESP_LOGI(TAG, "[APP] Error %d", error_code);
    return error_code;
}
