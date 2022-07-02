#include <stdio.h>
#include <esp_timer.h>

void timer_cb(void *args)
{
    printf("timer fired 1 seconds\n");
}

void app_main(void)
{

    const esp_timer_create_args_t esp_timer_create_args = {
        .callback = timer_cb,
        .name = "my timer"
    };

    esp_timer_handle_t timer;

    esp_timer_create(&esp_timer_create_args, &timer);

    //esp_timer_start_once(timer,20);
    esp_timer_start_periodic(timer,1000000);

}
