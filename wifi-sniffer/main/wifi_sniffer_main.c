#include "utils.h"

static const char *TAG = "WIFI_SNIFFER";
static TaskHandle_t xHandle_led = NULL;

static void blink_task(void *pvParameter)
{
    gpio_pad_select_gpio(BLINK_GPIO);

    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    while(true){
        /* Blink off (output low) */
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(BLINK_TIME_OFF / portTICK_PERIOD_MS);

        /* Blink on (output high) */
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(BLINK_TIME_ON / portTICK_PERIOD_MS);
    }
}

void set_blink_led(int state)
{
	switch(state){
		case BLINK_MODE: //blink
			BLINK_TIME_OFF = 1000;
			BLINK_TIME_ON = 1000;
			break;
		case ON_MODE: //always on
			BLINK_TIME_OFF = 5;
			BLINK_TIME_ON = 2000;
			break;
		case OFF_MODE: //always off
			BLINK_TIME_OFF = 2000;
			BLINK_TIME_ON = 5;
			break;
		case STARTUP_MODE: //fast blink
			BLINK_TIME_OFF = 100;
			BLINK_TIME_ON = 100;
			break;
		default:
			break;
	}
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		set_blink_led(OFF_MODE);
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		set_blink_led(ON_MODE);
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
		ESP_LOGI(TAG, "Station Info:");
		wifi_config_t wifi_config;
		esp_err_t ret = esp_wifi_get_config(WIFI_IF_STA, &wifi_config);
		if (ret == ESP_OK) {
			ESP_LOGI(TAG, "Wi-Fi SSID: %s", wifi_config.sta.ssid);
			ESP_LOGI(TAG, "Wi-Fi Password: %s", wifi_config.sta.password);
			ESP_LOGI(TAG, "Wi-Fi Scan Method: %d", wifi_config.sta.scan_method);

		} else {
			printf("Failed to get Wi-Fi config, error: %d\n", ret);
		}
        ESP_LOGI(TAG, "IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
		wifi_ap_record_t ap_info;
		ret = esp_wifi_sta_get_ap_info(&ap_info);
		ESP_LOGI(TAG, "AP Info:");
		ESP_LOG_BUFFER_HEX("MAC Address\t", ap_info.bssid, sizeof(ap_info.bssid));
		ESP_LOG_BUFFER_CHAR("SSID\t\t", ap_info.ssid, sizeof(ap_info.ssid));
		ESP_LOGI(TAG, "Primary Channel: %d\t", ap_info.primary);
		ESP_LOGI(TAG, "RSSI: %d\t\t", ap_info.rssi);
		ESP_LOGI(TAG, "Bandwidth: %d\t\t", ap_info.bandwidth);
		ESP_LOG_BUFFER_CHAR("Country Code: \t", ap_info.country.cc, sizeof(ap_info.country.cc));
    }
}


void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{
	int pkt_len, fc, sn=0;
	char ssid[SSID_MAX_LEN] = "\0", hash[MD5_LEN] = "\0", htci[5] = "\0";
	uint8_t ssid_len;
	time_t ts;

	wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buff;
	wifi_mgmt_hdr *mgmt = (wifi_mgmt_hdr *)pkt->payload;

	fc = ntohs(mgmt->fctl);

	if((fc & 0xFF00) == 0x4000){ //only look for probe request packets
		time(&ts);

		ssid_len = pkt->payload[25];
		if(ssid_len > 0)
			get_ssid(pkt->payload, ssid, ssid_len);

		pkt_len = pkt->rx_ctrl.sig_len;
		//get_hash(pkt->payload, pkt_len-4, hash);

		sn = get_sn(pkt->payload);

		get_ht_capabilites_info(pkt->payload, htci, pkt_len, ssid_len);
			ESP_LOGI(TAG, "ADDR=%02x:%02x:%02x:%02x:%02x:%02x, "
				"SSID=%s, "
				"CHANNEL=%d, "
				"TIMESTAMP=%d, "
				"HASH=%s, "
				"RSSI=%02d, "
				"SN=%d, "
				"HT CAP. INFO=%s",
				mgmt->sa[0], mgmt->sa[1], mgmt->sa[2], mgmt->sa[3], mgmt->sa[4], mgmt->sa[5],
				ssid,
				pkt->rx_ctrl.channel,
				(int)ts,
				hash,
				pkt->rx_ctrl.rssi,
				sn,
				htci);
	}
}

void sniffer_task(void *pvParameters)
{
	// Set promiscuous mode
	esp_wifi_set_promiscuous(true);
	esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);

	// Not useful because whichever the channel it is associated
	// as STA only it can sniff
	esp_wifi_set_channel(CONFIG_CUSTOM_WIFI_SNIFF_CHANNEL, WIFI_SECOND_CHAN_NONE); //only set the primary channel

    while (1) {
        // Your sniffer logic here
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize TCP/IP network interface
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default station
    esp_netif_create_default_wifi_sta();

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handler
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    // Configure WiFi station
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_CUSTOM_WIFI_SSID,
            .password = CONFIG_CUSTOM_WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "Starting Blink task...");
	xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, &xHandle_led);
	set_blink_led(STARTUP_MODE);

	ESP_LOGI(TAG, "Starting Sniffing task...");
	xTaskCreate(&sniffer_task, "sniffing_task", 2048, NULL, 5, NULL);

	ESP_LOGI(TAG, "Starting Main task...");
	while(1){ //every 0.5s check if fatal error occurred
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}