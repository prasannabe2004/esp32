#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

void fun()
{
    char buf[30];
    memset(buf,'x', 30);
    printf("%s", buf);
}
void task1()
{
    fun();
    vTaskDelete(NULL);
}

void app_main(void)
{
    xTaskCreate(task1, "task 1", 1024*2, NULL, 5, NULL);
}
