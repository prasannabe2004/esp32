#include <stdio.h>
#include <esp_log.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"
#include "math.h"

#define TAG "DICE"

int dice_role()
{
    int random = esp_random();
    int positive_number = abs(random);
    int dice = (positive_number % 6) + 1;
    return dice;
}

void app_main(void)
{
    while(1)
    {
        ESP_LOGI(TAG, "random number %d", dice_role());
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}