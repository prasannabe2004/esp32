#include <stdio.h>
#include "esp_log.h"

void app_main(void)
{
  esp_log_level_set("log", ESP_LOG_INFO);
  ESP_LOGE("log", "This is an error");
  ESP_LOGW("log", "This is a warning");
  ESP_LOGI("log", "This is an info");
  ESP_LOGD("log", "This is a debug");
  ESP_LOGV("log", "This is a verbose");
}