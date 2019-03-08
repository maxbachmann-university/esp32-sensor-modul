/* ULP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_sleep.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "esp32/ulp.h"
#include "ulp_main.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_wifi.h"

#include "TaskCommunication.h"
#include "nvs_flash_initialize.h"
#include "wifi_task.h"
#include "mqtts_task.h"

static const char *TAG = "MOTOR_CONTROL_MAIN";

#if CONFIG_OTA_UPDATE_ACTIVATED == 1
    #define OTA_UPDATE true
#else
    #define OTA_UPDATE false
#endif

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

/* This function is called once after power-on reset, to load ULP program into
 * RTC memory and configure the ADC.
 */
static void init_ulp_program();

/* This function is called every time before going into deep sleep.
 * It starts the ULP program and resets measurement counter.
 */
static void start_ulp_program();

#define ADC_RANGESIZE 500


void app_main()
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
    /*  set log levels */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);


    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        printf("Not ULP wakeup\n");
        init_ulp_program();
        printf("Entering deep sleep\n\n");
        start_ulp_program();
        ESP_ERROR_CHECK( esp_sleep_enable_ulp_wakeup() );
        esp_deep_sleep_start();
    } else {
        printf("Deep sleep wakeup\n");
        printf("ULP did %d measurements since last reset\n", ulp_sample_counter & UINT16_MAX);
        printf("Thresholds:  low=%d  high=%d\n", ulp_low_thr, ulp_high_thr);
        ulp_last_result &= UINT16_MAX;
        printf("Value=%d was %s threshold\n", ulp_last_result,
                ulp_last_result < ulp_low_thr ? "below" : "above");
            
        communication_init();
        /*  find out which range the result is in
        *   (cutting result behind, is intended)*/
        uint16_t range = ulp_last_result/ADC_RANGESIZE;

        /*  adjust ranges for adc results (measure in steps of RANGESIZE)*/
        ulp_low_thr = ADC_RANGESIZE * range;
        if (range != 0) ulp_low_thr -= 10;

        /*  so it does not wakeup when the value is very close to
         *   RANGESIZE * n */
        ulp_high_thr = ADC_RANGESIZE * (range + 1) + 10;
        nvs_flash_init();
        /*  start Wifi task (runs on core 0) */
        wifi_task_init();
        /*  start MQTT task */
        mqtts_task_init(ulp_last_result);

        /*  initialize over the air updates */
        if (OTA_UPDATE)
        {
            ota_update_task_init();
        } else {
            xEventGroupSetBits(sleep_event_handle, UPDATE_CHECKED_BIT);
        }

        xEventGroupWaitBits(sleep_event_handle, MESSAGE_SEND_BIT | UPDATE_CHECKED_BIT,
                     false, true, portMAX_DELAY);
        printf("Entering deep sleep\n\n");
        start_ulp_program();
        ESP_ERROR_CHECK( esp_sleep_enable_ulp_wakeup() );
        esp_deep_sleep_start();
    }
    
}

static void init_ulp_program()
{
    esp_err_t err = ulp_load_binary(0, ulp_main_bin_start,
            (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);

    /* Configure ADC channel */
    /* Note: when changing channel here, also change 'adc_channel' constant
       in adc.S */
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_ulp_enable();

    /* Set low and high thresholds out of range so a first measurement 
    * is transmitted after boot*/
    ulp_low_thr = 4096;
    ulp_high_thr = 4096;

    /* Set ULP wake up period to 20ms */
    ulp_set_wakeup_period(0, 20000);

    /* Disconnect GPIO12 and GPIO15 to remove current drain through
     * pullup/pulldown resistors.
     * GPIO12 may be pulled high to select flash voltage.
     */
    rtc_gpio_isolate(GPIO_NUM_12);
    rtc_gpio_isolate(GPIO_NUM_15);
    //esp_deep_sleep_disable_rom_logging(); // suppress boot messages
}

static void start_ulp_program()
{
    /* Reset sample counter */
    ulp_sample_counter = 0;

    /* Start the program */
    esp_err_t err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
    ESP_ERROR_CHECK(err);
}
