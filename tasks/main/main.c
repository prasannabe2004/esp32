#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void task1(void *params)
{
    while(true)
    {
        printf("%s: Inside reading temperature\n", (char*) params);
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

void task2(void *params)
{
    while(true)
    {
        printf("%s: Inside reading humidity\n", (char*) params);
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}
void app_main(void)
{
    xTaskCreate(&task1, "temperature reader", 2048, "task 1", 2, NULL);
    xTaskCreate(&task2, "humidity reader", 2048, "task 2", 2, NULL);
}
