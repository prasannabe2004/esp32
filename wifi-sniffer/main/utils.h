#include "mgmt.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "rom/gpio.h"

#include "lwip/err.h"
#include "lwip/sys.h"

/* --- LED variables --- */
#define BLINK_GPIO 2 //LED pin definition
#define BLINK_MODE 0
#define ON_MODE 1
#define OFF_MODE 2
#define STARTUP_MODE 3
static int BLINK_TIME_ON = 5; //LED blink time init on
static int BLINK_TIME_OFF = 1000; //LED blink time init off

void get_ssid(unsigned char *data, char ssid[SSID_MAX_LEN], uint8_t ssid_len);
int get_sn(unsigned char *data);
void get_ht_capabilites_info(unsigned char *data, char htci[5], int pkt_len, int ssid_len);
void dumb(unsigned char *data, int len);