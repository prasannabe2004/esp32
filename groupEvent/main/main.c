#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

EventGroupHandle_t groupEvent;
const int gotHTTP = BIT0;
const int gotBLE = BIT1;

void listenForHttp(void *params)
{
    while(true)
    {
        xEventGroupSetBits(groupEvent, gotHTTP);
        printf("Got HTTP\n");
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}

void listenForBLE(void *params)
{
    while(true)
    {
        xEventGroupSetBits(groupEvent, gotBLE);
        printf("Got BLE\n");
        vTaskDelay(4000/portTICK_PERIOD_MS);
    }
}

void task1(void *params)
{
    while(true)
    {
        xEventGroupWaitBits(groupEvent, gotHTTP|gotBLE, true, true, portMAX_DELAY);
        printf("received http and BLE\n");
    }
}
void app_main(void)
{
    groupEvent = xEventGroupCreate();
    xTaskCreate(&listenForHttp, "get http", 2048, "task 1", 2, NULL);
    xTaskCreate(&listenForBLE, "get ble", 2048, "task 1", 2, NULL);
    xTaskCreate(&task1, "do something with http", 2048, "task 2", 1, NULL);
}
