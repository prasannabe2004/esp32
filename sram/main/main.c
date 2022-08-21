#include <stdio.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "Memory"

int i = 0;
int j;

void task(void *params)
{
    int stackSize = uxTaskGetStackHighWaterMark(NULL);
    printf("task stack size = %d\n", stackSize);

    while(true)
        vTaskDelay(1000);
}
void app_main(void)
{
    printf("%d %d", i , j);
    ESP_LOGI(TAG, "Free Heap Size = %d\n", xPortGetFreeHeapSize());

    int DRAM = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    int SRAM = heap_caps_get_free_size (MALLOC_CAP_32BIT) - heap_caps_get_free_size(MALLOC_CAP_8BIT); 

    ESP_LOGI(TAG, "DRAM = %d SRAM = %d\n", DRAM, SRAM);
    int stackSize = uxTaskGetStackHighWaterMark(NULL);
    printf("main stack size = %d\n", stackSize);

    xTaskCreate(task, "task", 12000, NULL, 1, NULL);
}
