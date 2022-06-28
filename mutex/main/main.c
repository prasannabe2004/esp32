#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

xSemaphoreHandle mutexBus;

void writeToBus(char *message)
{
    printf(message);
}

void task1(void *params)
{
    while(true)
    {
        printf("%s: Inside reading temperature\n", (char*) params);
        if(xSemaphoreTake(mutexBus, 1000/portTICK_PERIOD_MS))
        {
            writeToBus("writing temperature\n");
            xSemaphoreGive(mutexBus);
        }
        else
        {
            printf("writing temparature timed out\n");
        }
        
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

void task2(void *params)
{
    while(true)
    {
        printf("%s: Inside reading humidity\n", (char*) params);
        if(xSemaphoreTake(mutexBus, 1000/portTICK_PERIOD_MS))
        {
            writeToBus("writing humidity\n");
            xSemaphoreGive(mutexBus);
        }
        else
        {
            printf("writing humidity timed out\n");
        }
        vTaskDelay(2000/portTICK_PERIOD_MS);
    }
}
void app_main(void)
{
    mutexBus = xSemaphoreCreateMutex();
    xTaskCreate(&task1, "temperature reader", 2048, "task 1", 2, NULL);
    xTaskCreate(&task2, "humidity reader", 2048, "task 2", 2, NULL);
}
