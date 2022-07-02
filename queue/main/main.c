#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

xQueueHandle queue;

void listenForHttp(void *params)
{
    int count = 0;
    while(true)
    {
        count++;
        printf("Sending %d\n", count);
        long ok = xQueueSend(queue, &count, 1000/portTICK_PERIOD_MS);
        if(ok) printf("Added HTTP message to queue\n");
        else printf("Failed to add HTTP message to queue\n");
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

void task1(void *params)
{
    while(true)
    {
        int rx;
        if(xQueueReceive(queue, &rx,5000/portTICK_PERIOD_MS))
            printf("received %d\n", rx);
        else
            printf("timed out\n");
    }
}
void app_main(void)
{
    queue = xQueueCreate(3, sizeof(int));
    xTaskCreate(&listenForHttp, "get http", 2048, "task 2", 2, NULL);
    xTaskCreate(&task1, "do something with http", 2048, "task 1", 2, NULL);
}
