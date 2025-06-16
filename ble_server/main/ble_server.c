#include <stdio.h>
#include "esp_log.h"
#include "esp_bt.h"
#include "nvs_flash.h"
#include "esp_bt_main.h"
#include "esp_gatts_api.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_common_api.h"

#define TAG "BLE_SERVER"
#define DEVICE_NAME "MyESP32BLE"


#define GATTS_SERVICE_UUID   0x00FF
#define GATTS_CHAR_UUID      0xFF01
#define GATTS_NUM_HANDLE     4

static uint8_t char_value[1] = {0x00};
static esp_gatt_char_prop_t char_property = 0;

static uint16_t gatts_if_handle = 0;
static uint16_t conn_id = 0;
static uint16_t service_handle = 0;
static esp_gatt_srvc_id_t service_id;

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param) {
    if (event == ESP_GAP_BLE_ADV_START_COMPLETE_EVT) {
        ESP_LOGI(TAG, "Advertising started.");
    }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param) {
    switch (event) {
        case ESP_GATTS_REG_EVT: {
            ESP_LOGI(TAG, "ESP_GATTS_REG_EVT");

            service_id.is_primary = true;
            service_id.id.inst_id = 0x00;
            service_id.id.uuid.len = ESP_UUID_LEN_16;
            service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID;

            esp_ble_gatts_create_service(gatts_if, &service_id, GATTS_NUM_HANDLE);
            break;
        }
        case ESP_GATTS_CREATE_EVT: {
            ESP_LOGI(TAG, "Service created, handle: %d", param->create.service_handle);
            service_handle = param->create.service_handle;
            esp_ble_gatts_start_service(service_handle);

            esp_gatt_srvc_id_t *srvc_id = &param->create.service_id;
            esp_bt_uuid_t char_uuid = {
                .len = ESP_UUID_LEN_16,
                .uuid.uuid16 = GATTS_CHAR_UUID,
            };

            char_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
            esp_attr_value_t gatts_demo_char_val = {
                .attr_max_len = 0x40,
                .attr_len = sizeof(char_value),
                .attr_value = char_value,
            };

            esp_ble_gatts_add_char(service_handle, &char_uuid,
                                   ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                   char_property,
                                   &gatts_demo_char_val, NULL);
            break;
        }
        case ESP_GATTS_ADD_CHAR_EVT: {
            ESP_LOGI(TAG, "Characteristic added, handle: %d", param->add_char.attr_handle);
            esp_ble_gap_set_device_name(DEVICE_NAME);
            esp_ble_adv_data_t adv_data = {
                .set_scan_rsp = false,
                .include_name = true,  // No name in main adv packet
                .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
            };
            esp_ble_gap_config_adv_data(&adv_data);

            esp_ble_gap_start_advertising(&(esp_ble_adv_params_t) {
                .adv_int_min = 0x20,
                .adv_int_max = 0x40,
                .adv_type = ADV_TYPE_IND,
                .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
                .channel_map = ADV_CHNL_ALL,
                .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
            });
            break;
        }
        case ESP_GATTS_WRITE_EVT: {
            ESP_LOGI(TAG, "Write event received, value: %.*s",
                     param->write.len, param->write.value);
            break;
        }
        case ESP_GATTS_READ_EVT: {
            ESP_LOGI(TAG, "Read request received");
            break;
        }
        default:
            break;
    }
}

void app_main(void) {
    esp_err_t ret;

    // Init NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
    ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

    ESP_ERROR_CHECK(esp_bluedroid_init());
    ESP_ERROR_CHECK(esp_bluedroid_enable());

    ESP_ERROR_CHECK(esp_ble_gatts_register_callback(gatts_profile_event_handler));
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(gap_event_handler));
    ESP_ERROR_CHECK(esp_ble_gatts_app_register(0));
}
