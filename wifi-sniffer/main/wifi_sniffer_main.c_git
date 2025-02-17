#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stddef.h>
#include <_ansi.h>

#include "driver/gpio.h"
#include "rom/gpio.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "esp_system.h"

#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"

#include "apps/sntp/sntp.h"

#include "mbedtls/md5.h"
#include "mqtt_client.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

/* LED RED ON: ESP32 turned on
 * LED BLUE FAST BLINK: startup phase
 * LED BLUE ON: ESP32 connected to the wifi, but not to the MQTT broker
 * LED BLUE BLINK: ESP32 connected to the broker */

/* --- LED variables --- */
#define BLINK_GPIO 2 //LED pin definition
#define BLINK_MODE 0
#define ON_MODE 1
#define OFF_MODE 2
#define STARTUP_MODE 3
static int BLINK_TIME_ON = 5; //LED blink time init on
static int BLINK_TIME_OFF = 1000; //LED blink time init off

 /* --- Some configurations --- */
#define SSID_MAX_LEN (32+1) //max length of a SSID
#define MD5_LEN (32+1) //length of md5 hash
#define BUFFSIZE 1024 //size of buffer used to send data to the server
#define NROWS 11 //max rows that buffer can have inside send_data, it can be changed modifying BUFFSIZE
#define MAX_FILES 3 //max number of files in SPIFFS partition

/* TAG of ESP32 for I/O operation */
static const char *TAG = "WIFI-SNIFFER";

/* Always set as true, when a fatal error occurs in task the variable will be set as false */
static bool RUNNING = true;
/* Only used in startup: if time_init() can't set current time for the first time -> reboot() */
static bool ONCE = true;
/* True if ESP is connected to the wifi, false otherwise */
static bool WIFI_CONNECTED = false;
/* True if ESP is connected to the MQTT broker, false otherwise */
static bool MQTT_CONNECTED = false;
/* If the variable is true the sniffer_task() will write on FILENAME1, otherwise on FILENAME2
 * The value of this variable is changed only by the function send_data() */
static bool WHICH_FILE = false;
 /* True when the wifi-task lock a file (to be send) and set the other file for the sniffer-task*/
static bool FILE_CHANGED = true;
/* Lock used for mutual exclusion for I/O operation in the files */
static _lock_t lck_file;
/* Lock used for MQTT connection to access to the MQTT_CONNECTED variable */
static _lock_t lck_mqtt;

/* Handle for blink task */
static TaskHandle_t xHandle_led = NULL;
/* Handle for sniff task */
static TaskHandle_t xHandle_sniff = NULL;
/* Handle for wifi task */
static TaskHandle_t xHandle_wifi = NULL;
/* Client variable for MQTT connection */
static esp_mqtt_client_handle_t client;
/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

typedef struct {
	int16_t fctl; //frame control
	int16_t duration; //duration id
	uint8_t da[6]; //receiver address
	uint8_t sa[6]; //sender address
	uint8_t bssid[6]; //filtering address
	int16_t seqctl; //sequence control
	unsigned char payload[]; //network data
} __attribute__((packed)) wifi_mgmt_hdr;

static void vfs_spiffs_init(void);
static void time_init(void);
static void initialize_sntp(void);
static void obtain_time(void);

static void blink_task(void *pvParameter);
static void set_blink_led(int state);

static void sniffer_task(void *pvParameter);
static void wifi_sniffer_init(void);
static void wifi_sniffer_deinit(void);
static void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);
static void get_hash(unsigned char *data, int len_res, char hash[MD5_LEN]);
static void get_ssid(unsigned char *data, char ssid[SSID_MAX_LEN], uint8_t ssid_len);
static int get_sn(unsigned char *data);
static void get_ht_capabilites_info(unsigned char *data, char htci[5], int pkt_len, int ssid_len);
static void dumb(unsigned char *data, int len);
static void save_pkt_info(uint8_t address[6], char *ssid, time_t timestamp, char *hash, int8_t rssi, int sn, char htci[5]);
static int get_start_timestamp(void);

static void wifi_task(void *pvParameter);
static void wifi_connect_init(void);
static void wifi_connect_deinit(void);
static void mqtt_app_start(void);
static int set_waiting_time(void);
static void send_data(void);
static void file_init(char *filename);

static void reboot(char *msg_err); //called only by main thread

static esp_event_handler_instance_t ip_event_handler;
static esp_event_handler_instance_t wifi_event_handler;

static void wifi_event_cb(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Handling Wi-Fi event, event code 0x%" PRIx32, event_id);

    switch (event_id)
    {
    	case (WIFI_EVENT_WIFI_READY):
        	ESP_LOGI(TAG, "Wi-Fi ready");
        	break;
    	case (WIFI_EVENT_SCAN_DONE):
        	ESP_LOGI(TAG, "Wi-Fi scan done");
        	break;
    	case (WIFI_EVENT_STA_START):
        	ESP_LOGI(TAG, "Wi-Fi started, connecting to AP...");
        	esp_wifi_connect();
        	break;
    	case (WIFI_EVENT_STA_STOP):
        	ESP_LOGI(TAG, "Wi-Fi stopped");
        	break;
    	case (WIFI_EVENT_STA_CONNECTED):
        	ESP_LOGI(TAG, "Wi-Fi connected");
        	break;
    	case (WIFI_EVENT_STA_DISCONNECTED):
			ESP_LOGI(TAG, "[WI-FI] Disconnected");
			if(WIFI_CONNECTED == false)
				ESP_LOGW(TAG, "[WI-FI] Impossible to connect to wifi: wrong password and/or SSID or Wi-Fi down");
			WIFI_CONNECTED = false;
			set_blink_led(OFF_MODE);
			if(RUNNING){
				ESP_ERROR_CHECK(esp_wifi_connect());
			}
			else
				xEventGroupClearBits(wifi_event_group, BIT0);
			break;
    	case (WIFI_EVENT_STA_AUTHMODE_CHANGE):
        	ESP_LOGI(TAG, "Wi-Fi authmode changed");
        	break;
    	default:
        	ESP_LOGI(TAG, "Wi-Fi event not handled");
        	break;
    }
}

static void ip_event_cb(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Handling IP event, event code 0x%" PRIx32, event_id);
    switch (event_id)
    {
		case IP_EVENT_STA_GOT_IP: //wifi connected
			ESP_LOGI(TAG, "[WI-FI] Connected");
			WIFI_CONNECTED = true;
			set_blink_led(ON_MODE);
			xEventGroupSetBits(wifi_event_group, BIT0);
			break;
    case (IP_EVENT_STA_LOST_IP):
        ESP_LOGI(TAG, "Lost IP");
        break;
    case (IP_EVENT_GOT_IP6):
        break;
    default:
        ESP_LOGI(TAG, "IP event not handled");
        break;
    }
}


#define CONFIG_VERBOSE 1
#define CONFIG_WIFI_SSID "Mobile"
#define CONFIG_SNIFFING_TIME 60
#define CONFIG_WIFI_PSW "Crimping@@2020"
#define CONFIG_CHANNEL 11
#define CONFIG_FILENAME1 "/spiffs/probreq.log"
#define CONFIG_FILENAME2 "/spiffs/probreq2.log"

void wificonnect()
{

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = CONFIG_WIFI_SSID,
			.password = CONFIG_WIFI_PSW,
			//.bssid_set = false,
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Waiting for connection to the WiFi network...");
    xEventGroupWaitBits(wifi_event_group, BIT0, false, true, portMAX_DELAY);
}

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

static void set_blink_led(int state)
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

static void obtain_time()
{
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 15;

    initialize_sntp();

    //wait for time to be set
    while(timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if(retry >= retry_count){ //can't set time
    	if(ONCE) //if it is first time -> reboot: no reason to sniff with wrong time
    		reboot("No response from server after several time. Impossible to set current time");
    }
    else{
    	ONCE = false;
    }
}

static void initialize_sntp()
{
    sntp_setoperatingmode(SNTP_OPMODE_POLL); //automatically request time after 1h
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

static esp_netif_t *wifi_netif = NULL;

static void wifi_connect_init()
{
	esp_log_level_set("wifi", ESP_LOG_NONE); //disable the default wifi logging

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize TCP/IP network stack");
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create default event loop");
    }

    ret = esp_wifi_set_default_wifi_sta_handlers();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set default handlers");
    }

    wifi_netif = esp_netif_create_default_wifi_sta();
    if (wifi_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default WiFi STA interface");
    }

	//tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate(); //create the event group to handle wifi events
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA)); //create soft-AP and station control block

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_cb,
                                                        NULL,
                                                        &wifi_event_handler));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &ip_event_cb,
                                                        NULL,
                                                        &ip_event_handler));

}

static void wifi_connect_deinit()
{
	ESP_ERROR_CHECK(esp_wifi_disconnect()); //disconnect the ESP32 WiFi station from the AP
	ESP_ERROR_CHECK(esp_wifi_stop()); //it stop station and free station control block
	ESP_ERROR_CHECK(esp_wifi_deinit()); //free all resource allocated in esp_wifi_init and stop WiFi task
}

static void sniffer_task(void *pvParameter)
{
	int sleep_time = CONFIG_SNIFFING_TIME*1000;

	ESP_LOGI(TAG, "[SNIFFER] Sniffer task created");

	ESP_LOGI(TAG, "[SNIFFER] Starting sniffing mode...");
	wifi_sniffer_init();
	ESP_LOGI(TAG, "[SNIFFER] Started. Sniffing on channel %d", CONFIG_CHANNEL);
	while(true){
		vTaskDelay(sleep_time / portTICK_PERIOD_MS);
	}
}

static void wifi_sniffer_init()
{
	//tcpip_adapter_init();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg)); //allocate resource for WiFi driver

	const wifi_country_t wifi_country = {
			.cc = "CN",
			.schan = 1,
			.nchan = 13,
			.policy = WIFI_COUNTRY_POLICY_AUTO
	};
	ESP_ERROR_CHECK(esp_wifi_set_country(&wifi_country)); //set country for channel range [1, 13]
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
	ESP_ERROR_CHECK(esp_wifi_start());

	const wifi_promiscuous_filter_t filt = {
			.filter_mask = WIFI_EVENT_MASK_AP_PROBEREQRECVED
	};
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filt)); //set filter mask
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(wifi_sniffer_packet_handler)); //callback function
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true)); //set 'true' the promiscuous mode

	esp_wifi_set_channel(CONFIG_CHANNEL, WIFI_SECOND_CHAN_NONE); //only set the primary channel
}

static void wifi_sniffer_deinit()
{
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false)); //set as 'false' the promiscuous mode
	ESP_ERROR_CHECK(esp_wifi_stop()); //it stop soft-AP and free soft-AP control block
	ESP_ERROR_CHECK(esp_wifi_deinit()); //free all resource allocated in esp_wifi_init() and stop WiFi task
}

static void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
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

		if(0){
			ESP_LOGI(TAG, "Dump");
			dumb(pkt->payload, pkt_len);
		}

		sn = get_sn(pkt->payload);

		get_ht_capabilites_info(pkt->payload, htci, pkt_len, ssid_len);
		if(ssid_len > 1) {
		ESP_LOGI(TAG, "ADDR=%02x:%02x:%02x:%02x:%02x:%02x, "
				"SSID=%s, "
				"TIMESTAMP=%d, "
				"HASH=%s, "
				"RSSI=%02d, "
				"SN=%d, "
				"HT CAP. INFO=%s",
				mgmt->sa[0], mgmt->sa[1], mgmt->sa[2], mgmt->sa[3], mgmt->sa[4], mgmt->sa[5],
				ssid,
				(int)ts,
				hash,
				pkt->rx_ctrl.rssi,
				sn,
				htci);
		}
		//save_pkt_info(mgmt->sa, ssid, ts, hash, pkt->rx_ctrl.rssi, sn, htci);
	}
}

static void get_ssid(unsigned char *data, char ssid[SSID_MAX_LEN], uint8_t ssid_len)
{
	int i, j;

	for(i=26, j=0; i<26+ssid_len; i++, j++){
		ssid[j] = data[i];
	}

	ssid[j] = '\0';
}

static int get_sn(unsigned char *data)
{
	int sn;
    char num[5] = "\0";

	sprintf(num, "%02x%02x", data[22], data[23]);
    sscanf(num, "%x", &sn);

    return sn;
}

static void get_ht_capabilites_info(unsigned char *data, char htci[5], int pkt_len, int ssid_len)
{
	int ht_start = 25+ssid_len+19;

	/* 1) data[ht_start-1] is the byte that says if HT Capabilities is present or not (tag length).
	 * 2) I need to check also that i'm not outside the payload: if HT Capabilities is not present in the packet,
	 * for this reason i'm considering the ht_start must be lower than the total length of the packet less the last 4 bytes of FCS */

	if(data[ht_start-1]>0 && ht_start<pkt_len-4){ //HT capabilities is present
		if(data[ht_start-4] == 1) //DSSS parameter is set -> need to shift of three bytes
			sprintf(htci, "%02x%02x", data[ht_start+3], data[ht_start+1+3]);
		else
			sprintf(htci, "%02x%02x", data[ht_start], data[ht_start+1]);
	}
}

static void dumb(unsigned char *data, int len)
{
	unsigned char i, j, byte;

	for(i=0; i<len; i++){
		byte = data[i];
		printf("%02x ", data[i]);

		if(((i%16)==15) || (i==len-1)){
			for(j=0; j<15-(i%16); j++)
				printf(" ");
			printf("| ");
			for(j=(i-(i%16)); j<=i; j++){
				byte = data[j];
				if((byte>31) && (byte<127))
					printf("%c", byte);
				else
					printf(".");
			}
			printf("\n");
		}
	}
}

static int get_start_timestamp()
{
	int stime;
	time_t clk;

	time(&clk);
	stime = (int)clk - (int)clk % CONFIG_SNIFFING_TIME;

	return stime;
}

static void reboot(char *msg_err)
{
	int i;

	ESP_LOGE(TAG, "%s", msg_err);
    for(i=3; i>=0; i--){
        ESP_LOGW(TAG, "Restarting in %d seconds...", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    ESP_LOGW(TAG, "Restarting now");
    fflush(stdout);

    esp_restart();
}


void app_main(void)
{
	ESP_LOGI(TAG, "[+] Startup...");

	ESP_ERROR_CHECK(nvs_flash_init()); //initializing NVS (Non-Volatile Storage)

    wifi_connect_init(); //both soft-AP and station


    //wificonnect();
  
	ESP_LOGI(TAG, "[!] Starting blink task...");
	xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, &xHandle_led);
	if(xHandle_led == NULL)
		reboot("Impossible to create LED task");
	set_blink_led(STARTUP_MODE);

	ESP_LOGI(TAG, "[!] Starting sniffing task...");
	xTaskCreate(&sniffer_task, "sniffing_task", 10000, NULL, 1, &xHandle_sniff);
	if(xHandle_sniff == NULL)
		reboot("Impossible to create sniffing task");

	while(RUNNING){ //every 0.5s check if fatal error occurred
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}

	ESP_LOGW(TAG, "Deleting led task...");
	vTaskDelete(xHandle_led);
	ESP_LOGW(TAG, "Deleting sniffing task...");
	vTaskDelete(xHandle_sniff);
	ESP_LOGW(TAG, "Deleting Wi-Fi task...");
	vTaskDelete(xHandle_wifi);

	ESP_LOGW(TAG, "Unmounting SPIFFS");
	esp_vfs_spiffs_unregister(NULL); //SPIFFS unmounted

	ESP_LOGW(TAG, "Stopping sniffing mode...");
	wifi_sniffer_deinit();
	ESP_LOGI(TAG, "Stopped");

	ESP_LOGW(TAG, "Disconnecting from %s...", CONFIG_WIFI_SSID);
	wifi_connect_deinit();

	reboot("Rebooting: Fatal error occurred in a task");
}
