/**
 * 
 * Author : DABOUSSI MED AMINE 
 * Created : 19/08/25
 * 
 * 
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"


 /**
  * 
  * Brief :
  * 
  * This project will teach you how to:
  *    1) Connect the ESP32 to Wi-Fi (station mode).
  *    2) Print the IP address on UART.
  *    3) Toggle an LED when Wi-Fi connects/disconnects.
  *    4) Use a button to disconnect/reconnect Wi-Fi.
  * 
  * 
  */


  #define WIFI_SSID      "AQUATEC_HD2"
  #define WIFI_PASS      "5210008499"
  #define LED_PIN        GPIO_NUM_2
  #define BUTTON_PIN     GPIO_NUM_15

  static const char *TAG = "WiFi_Button_LED";
  static EventGroupHandle_t s_wifi_event_group;
  static int s_retry_num = 0;

  #define WIFI_CONNECTED_BIT BIT0
  #define WIFI_FAIL_BIT      BIT1

  // WiFi event handler
  static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) 
  {
      if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
      {
          esp_wifi_connect();
      } 
      else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
      {
          if (s_retry_num < 5) {
              esp_wifi_connect();
              s_retry_num++;
              ESP_LOGI(TAG, "Retrying to connect...");
          } else {
              xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
              gpio_set_level(LED_PIN, 0); // Turn LED off
          }
      } 
      else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
      {
          ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
          ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
          s_retry_num = 0;
          xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
          gpio_set_level(LED_PIN, 1); // Turn LED on
      }
  }

  // WiFi init function
  void wifi_init_sta(void) 
  {
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = 
    {
        .sta = 
        {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WiFi init finished.");
  }

  // Button task
  void button_task(void *pvParameter)
  {
      gpio_reset_pin(BUTTON_PIN);
      gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
      gpio_pulldown_en(BUTTON_PIN) ;
      while (1) {
          if (gpio_get_level(BUTTON_PIN) == 1) { // Button pressed
              ESP_LOGI(TAG, "Button pressed: restarting WiFi...");
              esp_wifi_stop();
              gpio_set_level(LED_PIN, 0); // Turn LED off
              vTaskDelay(1000 / portTICK_PERIOD_MS);
              esp_wifi_start();
              gpio_set_level(LED_PIN, 1); // Turn LED on
              vTaskDelay(1000 / portTICK_PERIOD_MS);
          }
          vTaskDelay(100 / portTICK_PERIOD_MS);
      }
  }

  void app_main ()
  {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    wifi_init_sta();

    xTaskCreate(button_task, "button_task", 2048, NULL, 5, NULL);

  }