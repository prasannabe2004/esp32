#ifndef __MGMT_H__
#define __MGMT_H__
#include <stdint.h>

typedef struct {
	int16_t fctl; //frame control
	int16_t duration; //duration id
	uint8_t da[6]; //receiver address
	uint8_t sa[6]; //sender address
	uint8_t bssid[6]; //filtering address
	int16_t seqctl; //sequence control
	unsigned char payload[]; //network data
} __attribute__((packed)) wifi_mgmt_hdr;

#define CONFIG_CUSTOM_WIFI_SSID      "CA2BLR1_4G"
#define CONFIG_CUSTOM_WIFI_PASSWORD      "minjursaranya"
#define SSID_MAX_LEN (32+1) //max length of a SSID
#define MD5_LEN (32+1) //length of md5 hash
#define BUFFSIZE 1024 //size of buffer used to send data to the server
#define NROWS 11 //max rows that buffer can have inside send_data, it can be changed modifying BUFFSIZE
#define MAX_FILES 3 //max number of files in SPIFFS partition
#endif