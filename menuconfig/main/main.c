#include <stdio.h>
#include "esp_log.h"

#define TAG "CONFIG"

void app_main(void)
{
    ESP_LOGI(TAG, "Nax number IPv6 address %d", CONFIG_NUM_OF_IPV6_ADDR);
}
