#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

#define WIFI_SSID "POCO5G"
#define WIFI_PASSWORD ""
#define WIFI_CONNECTED_BIT BIT0
const char *TAG = "ESP32_MQTT";

static EventGroupHandle_t wifi_events_h;

static void wifi_event_handler( void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data ) {
	if ( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START ) {
		ESP_LOGI( TAG, "ESP32 WIFI started" );
		esp_wifi_connect();
	}
	else if ( event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED ) {
		ESP_LOGI( TAG, "ESP32 MQTT connection start." );
		esp_wifi_connect();
	}
	else if ( event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP ) {
			ip_event_got_ip_t *event = ( ip_event_got_ip_t* )event_data;
			ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
			xEventGroupSetBits(wifi_events_h, WIFI_CONNECTED_BIT);
	}
}

esp_err_t wifi_init_station() {
	wifi_events_h = xEventGroupCreate();

	esp_netif_init();
	esp_err_t err = esp_event_loop_create_default();
	if ( err != ESP_OK ) {
		ESP_LOGE( TAG, "default event loop creation failed." );
		return err;
	}
	esp_netif_create_default_wifi_sta();
	esp_event_handler_instance_register( WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL );
	esp_event_handler_instance_register( IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL );
	
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	err = esp_wifi_init( &cfg );
	if ( err != ESP_OK ) {
		ESP_LOGE( TAG, "wifi init config failed." );
		return err;
	}
	wifi_config_t wifi_cfg = {
			.sta = {
					.ssid = WIFI_SSID,
					.password ="",
					.threshold.authmode = WIFI_AUTH_OPEN,
			},
	};
	esp_wifi_set_mode( WIFI_MODE_STA );
	esp_wifi_set_config( ESP_IF_WIFI_STA, &wifi_cfg );
	esp_wifi_start();

	ESP_LOGI( TAG, "WIFI init started." );
	xEventGroupWaitBits(wifi_events_h,
	                        WIFI_CONNECTED_BIT,
	                        pdFALSE,
	                        pdFALSE,
	                        portMAX_DELAY);
	return ESP_OK;
}

void app_main(void)
{
	ESP_LOGI( TAG, "ESP32 MQTT connection start." );
	esp_err_t err = ESP_OK;
	err = nvs_flash_init();
	if( err != ESP_OK ) {
		ESP_LOGE( TAG, "esp32 flash init failed." );
	}
	err = wifi_init_station();
	if ( err != ESP_OK ) {
		ESP_LOGE( TAG, "Connection to the wifi failed!" );
	}
	while( 1 ) {
		ESP_LOGI( TAG, "ESP32..");
		vTaskDelay( pdMS_TO_TICKS( 1000 ) );
	}

}

