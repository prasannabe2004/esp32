#include "utils.h"

static const char *TAG = "WIFI_SNIFFER";

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();
}

void obtain_time(void)
{
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}

void get_ssid(unsigned char *data, char ssid[SSID_MAX_LEN], uint8_t ssid_len)
{
	int i, j;

	for(i=26, j=0; i<26+ssid_len; i++, j++){
		ssid[j] = data[i];
	}

	ssid[j] = '\0';
}

void dumb(unsigned char *data, int len)
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

int get_sn(unsigned char *data)
{
	int sn;
    char num[5] = "\0";

	sprintf(num, "%02x%02x", data[22], data[23]);
    sscanf(num, "%x", &sn);

    return sn;
}

void get_ht_capabilites_info(unsigned char *data, char htci[5], int pkt_len, int ssid_len)
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