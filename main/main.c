#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "lcd.c"

/** DEFINES **/
#define WIFI_SUCCESS 1 
#define WIFI_FAILURE 2
#define TCP_SUCCESS 1 
#define TCP_FAILURE 2
#define MAX_FAILURES 5
#define BUF_SIZE 1024

//Global variables

// event group to contain status information
static EventGroupHandle_t wifi_event_group;

//wifi retry counter
static int w_retry_num = 0;

//socket retry counter
static int s_retry_num = 0;

//socket
static int sock = 0;

// task tag
static const char *TAG = "Network";

//LCD message
char msg[55];

//event handler for wifi events
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
	{
		ESP_LOGI(TAG, "Connecting to AP...");
		esp_wifi_connect();
	} else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
	{
		if (w_retry_num < MAX_FAILURES)
		{
			ESP_LOGI(TAG, "Reconnecting to AP...");
			esp_wifi_connect();
			w_retry_num++;
		} else {
			xEventGroupSetBits(wifi_event_group, WIFI_FAILURE);
		}
	}
}

//event handler for ip events
static void ip_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
	if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
	{
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "STA IP: " IPSTR, IP2STR(&event->ip_info.ip));
        w_retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_SUCCESS);
    }

}

// connect to wifi and return the result
esp_err_t connect_wifi()
{
	int status = WIFI_FAILURE;
	//initialize the esp network interface
	ESP_ERROR_CHECK(esp_netif_init());

	//initialize default esp event loop
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	//create wifi station in the wifi driver
	esp_netif_create_default_wifi_sta();

	//setup wifi station with the default wifi configuration
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // wifi and ip event handling
	wifi_event_group = xEventGroupCreate();

    esp_event_handler_instance_t wifi_handler_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &wifi_handler_event_instance));

    esp_event_handler_instance_t got_ip_event_instance;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &ip_event_handler,
                                                        NULL,
                                                        &got_ip_event_instance));

    /** START THE WIFI DRIVER **/
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "TELUSWiFi5985",
            .password = "4nuzn29fse",
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    // set the wifi controller to be a station
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );

    // set the wifi config
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );

    // start the wifi driver
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "STA initialization complete");
    //waiting
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_SUCCESS | WIFI_FAILURE,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);
            
    if (bits & WIFI_SUCCESS) {
        ESP_LOGI(TAG, "Connected to ap");
        status = WIFI_SUCCESS;
    } else if (bits & WIFI_FAILURE) {
        ESP_LOGI(TAG, "Failed to connect to ap");
        status = WIFI_FAILURE;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        status = WIFI_FAILURE;
    }

    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, got_ip_event_instance));
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_handler_event_instance));
    //vEventGroupDelete(wifi_event_group);

    return status;
}

// connect to the server and return the result
esp_err_t connect_tcp_server(void)
{
	struct sockaddr_in serverInfo = {0};
    esp_err_t status = TCP_SUCCESS;

	serverInfo.sin_family = AF_INET;
	serverInfo.sin_addr.s_addr = 0x5801A8C0;
	serverInfo.sin_port = htons(12345);


	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		ESP_LOGE(TAG, "Failed to create a socket..?");
        strcpy(msg, "Failed to create a socket..?");
        lcd_message(msg);
		return TCP_FAILURE;
	}


	if (connect(sock, (struct sockaddr *)&serverInfo, sizeof(serverInfo)) != 0)
	{
        if (s_retry_num < MAX_FAILURES) {
                ESP_LOGE(TAG, "Failed to connect to %s!", inet_ntoa(serverInfo.sin_addr.s_addr));
                strcpy(msg, "Failed to connect to");
                strcat(msg, inet_ntoa(serverInfo.sin_addr.s_addr));
                lcd_message(msg);
                ESP_LOGE(TAG, "Attempting to connect...");
                s_retry_num++;
                status = connect_tcp_server();
        }
        else {
            close(sock);
            status = TCP_FAILURE;
        }
	}
    else {
        ESP_LOGI(TAG, "Connected to TCP server.");
        strcpy(msg,"Connected to TCP Server");
        lcd_message(msg);
        return status;
    }

    return status;
}

esp_err_t readBattery() {
    lcd_cmd(0x01);
    esp_err_t status = TCP_SUCCESS;
    char readBuffer[1024] = {0};
    while (1) {

        bzero(readBuffer, sizeof(readBuffer));
        int len = read(sock, readBuffer, sizeof(readBuffer)-1);
        if (len < 0) {
            ESP_LOGE(TAG, "No data from server, attempting to reconnect...");
            status = connect_tcp_server();
        }
        else {
            readBuffer[len] = 0; 
            for (int i = 0; i < len; i++) {
                if (!isdigit((unsigned char)readBuffer[i])) {
                    readBuffer[i] = '\0';
                    break;
                }
            }
            s_retry_num = 0;
            int batteryPercentage = atoi((const char*)readBuffer);

            if (batteryPercentage >= 0 && batteryPercentage <= 100) {
                strcpy(msg, "Battery status: ");
                strcat(msg, readBuffer);
                lcd_percentage(msg);
                ESP_LOGI(TAG, "Received data: %d", batteryPercentage);
            }
        }
        vTaskDelay(500 / portTICK_PERIOD_MS); 
        if (status == TCP_FAILURE) { 
            break;
        }
    }
    return TCP_FAILURE;
}


void app_main(void)
{
	esp_err_t status = WIFI_FAILURE;

	//initialize storage
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    lcd_init();
    strcpy(msg, "Initialized LCD");
    lcd_message(msg);

    // connect to wireless AP
	status = connect_wifi();
	if (WIFI_SUCCESS != status)
	{
		ESP_LOGI(TAG, "Failed to associate to AP, terminating...");
        strcpy(msg, "Failed to associate to AP, terminating...");
        lcd_message(msg);
		return;
	}
	
	status = connect_tcp_server();
	if (TCP_SUCCESS != status)
	{
		ESP_LOGI(TAG, "Failed to connect to remote server, terminating...");
        strcpy(msg, "Failed to connect to remote server, terminating...");
        lcd_message(msg);
		return;
	}

    status = readBattery();
    ESP_LOGI(TAG, "Failed to connect to remote server, terminating...");
    strcpy(msg, "Failed to connect to remote server, terminating...");
    lcd_message(msg);
	return;

    
}
