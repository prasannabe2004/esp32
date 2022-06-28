#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

xSemaphoreHandle binSem;

void listenForHttp(void *params)
{
    while(true)
    {
        printf("received HTTP message\n");
        xSemaphoreGive(binSem);
        printf("processing the message\n");
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}

void task1(void *params)
{
    while(true)
    {
        xSemaphoreTake(binSem,portMAX_DELAY);
        printf("doing something with http\n");
    }
}
void app_main(void)
{
    binSem = xSemaphoreCreateBinary();
    xTaskCreate(&listenForHttp, "get http", 2048, "task 1", 2, NULL);
    xTaskCreate(&task1, "do something with http", 2048, "task 2", 1, NULL);
}
