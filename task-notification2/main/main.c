#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static TaskHandle_t receiverHandle = NULL;
void sender(void *params)
{
    while(true)
    {
        xTaskNotify(receiverHandle, (1<<0), eSetBits);
        vTaskDelay(1000/portTICK_PERIOD_MS);
        xTaskNotify(receiverHandle, (1<<1), eSetBits);
        vTaskDelay(1000/portTICK_PERIOD_MS);
        xTaskNotify(receiverHandle, (1<<2), eSetBits);
        vTaskDelay(1000/portTICK_PERIOD_MS);
        xTaskNotify(receiverHandle, (1<<3), eSetBits);
        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}

void receiver(void *params)
{
    unsigned int state;
    while(true)
    {
        xTaskNotifyWait(0xffffffff, 0, &state, portMAX_DELAY);
        printf("received state %d\n", state);
    }
}

void app_main(void)
{
    printf("in main task\n");
    xTaskCreate(&receiver, "receiver", 2048, NULL, 2, &receiverHandle);
    xTaskCreate(&sender, "sender", 2048, NULL, 2, NULL);
}
