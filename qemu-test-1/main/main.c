#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

void app_main(void)
{
    int count = 0;
    while(1)
    {
        ESP_LOGI("QEMU", "running QEMU count %d\n", count++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
