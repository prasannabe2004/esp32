#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static TaskHandle_t receiverHandle = NULL;
void sender(void *params)
{
    while(true)
    {
        xTaskNotifyGive(receiverHandle);
        vTaskDelay(5000/portTICK_PERIOD_MS);
    }
}

void receiver(void *params)
{
    while(true)
    {
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
        printf("received notification\n");
    }
}
void app_main(void)
{
    printf("in main task\n");
    xTaskCreate(&receiver, "receiver", 2048, NULL, 2, &receiverHandle);
    xTaskCreate(&sender, "sender", 2048, NULL, 2, NULL);
}
