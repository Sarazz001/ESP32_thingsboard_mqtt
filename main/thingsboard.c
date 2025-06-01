#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "lwip/sys.h"
#include "lwip/err.h"

#define WIFI_SSID "POCO5G"
#define WIFI_PASSWORD "63807327"
#define WIFI_CONNECTED_BIT BIT0

static EventGroupHandle_t wifiEventGroup;
static const char *TAG = "ESP32_MQTT";

static void wifi_event_handler( void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data ) 
{    
    if ( event_id == WIFI_EVENT_STA_START ) {
        ESP_LOGI( TAG, "ESP32 WIFI started, connecting...");
        esp_err_t err = esp_wifi_connect();
	if ( err != ESP_OK ) {
		ESP_LOGE( TAG, " failed to connect wifi ");
	}
    }
    else if ( event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "connected from WiFi");
    }
   else if ( event_id == IP_EVENT_STA_GOT_IP ) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
	xEventGroupSetBits( wifiEventGroup, WIFI_CONNECTED_BIT );
    }
}

void wifi_init_sta( void ) {
	wifiEventGroup = xEventGroupCreate();
	if ( wifiEventGroup == NULL ) {
		ESP_LOGE( TAG, "Event group creation failed. no space available" );
	}
	esp_err_t err = esp_netif_init();
	if ( err != ESP_OK ) {
		ESP_LOGE( TAG, "Netif TCP/IP initialization failed." );
	}
	err = esp_event_loop_create_default();
	if ( err != ESP_OK ) {
		ESP_LOGE( TAG, "Cannot create default event loop" );
	}
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t wifiCfg = WIFI_INIT_CONFIG_DEFAULT();
	err = esp_wifi_init( &wifiCfg );
	if ( err != ESP_OK ) {
		ESP_LOGE( TAG, "Cannot init wifi configuration" );
	}

	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;

	err = esp_event_handler_instance_register( WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);
	if ( err != ESP_OK ) {
		ESP_LOGE( TAG, "Cannot register wifi handler" );
	}
	err = esp_event_handler_instance_register( IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip);
	if ( err != ESP_OK ) {
		ESP_LOGE( TAG, "Cannot register ip handler" );
	}

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = WIFI_SSID,
			.password = WIFI_PASSWORD,
		},
	};

	err = esp_wifi_set_mode(WIFI_MODE_STA);
	if ( err != ESP_OK ) {
		ESP_LOGE( TAG, "Cannot set wifi sta mode" );
	}
	err = esp_wifi_set_config( WIFI_IF_STA, &wifi_config );
	if ( err != ESP_OK ) {
		ESP_LOGE( TAG, "Cannot set wifi config" );
	}
	err = esp_wifi_start();
	if ( err != ESP_OK ) {
		ESP_LOGE( TAG, "Cannot start wifi " );
	}
	ESP_LOGI( TAG, "wifi station init finished" );

	EventBits_t bits = xEventGroupWaitBits( wifiEventGroup, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY );
	if ( bits & WIFI_CONNECTED_BIT ) {
		ESP_LOGI( TAG , "connected to ssid: %s, password: %s", WIFI_SSID, WIFI_PASSWORD );
	}
}

int app_main(void) {
    esp_err_t err = ESP_OK;
    err  = nvs_flash_init();
    if ( err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND ) {
        ESP_LOGE( TAG, "Fail to init nvs flash memory" );
	err = nvs_flash_erase();
	if ( err != ESP_OK ) {
		ESP_LOGE( TAG, "failed to init nvs again" );
		return 1;
	}
    }
    wifi_init_sta();
    while (1) {
        ESP_LOGI(TAG, "Running main loop...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

