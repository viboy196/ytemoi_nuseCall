#include "i2c_display.h"
//I2C-Display
#include "sh1106.h"
#include "font8x8_basic.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "i2s_stream.h"
#include "esp_peripherals.h"
#include "periph_wifi.h"
#include "board.h"
#include "input_key_service.h"

#include "audio_mem.h"
#include "raw_stream.h"
#include "filter_resample.h"
#include "esp_sip.h"
#include "g711_decoder.h"
#include "g711_encoder.h"
#include <stdio.h>

static const char *I2C_TAG = "I2C_ESP32";

typedef struct periph_wifi *periph_wifi_handle_t;

struct periph_wifi
{
    periph_wifi_state_t wifi_state;
    bool disable_auto_reconnect;
    bool is_open;
    uint8_t max_recon_time;
    char *ssid;
    char *password;
    EventGroupHandle_t state_event;
    int reconnect_timeout_ms;
    periph_wifi_config_mode_t config_mode;
    periph_wpa2_enterprise_cfg_t *wpa2_e_cfg;
};

#define SDA_PIN GPIO_NUM_18
#define SCL_PIN GPIO_NUM_23

#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */

static i2c_port_t i2c_port = I2C_NUM_0; //default is I2C_NUM_0

bool port_installed = true; //Hacked!! Check it manually!!!

#define BATTERY_DISPLAY false

extern bool get_call_ringing_started();

void i2c_master_init()
{
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 500000};

    if (port_installed)
    {
        i2c_driver_delete(i2c_port);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    i2c_param_config(i2c_port, &i2c_config);
    esp_err_t err = i2c_driver_install(i2c_port, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    if (err == ESP_OK)
    {
        ESP_LOGI(I2C_TAG, "I2C driver installed successfully");
    }
    else
    {
        ESP_LOGI(I2C_TAG, "I2C driver NOT installed successfully");
    }
}

void sh1106_set_display_start_line(i2c_cmd_handle_t cmd, uint_fast8_t start_line)
{
    //Requires: 0<=start_line<=63
    //if (start_line >= 0 && start_line <= 63)
    {
        i2c_master_write_byte(cmd, OLED_CMD_SET_DISPLAY_START_LINE | start_line, true);
    }
}

void sh1106_init()
{
    esp_err_t err;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);

    i2c_master_write_byte(cmd, OLED_CMD_SET_CHARGE_PUMP_CTRL, true);
    i2c_master_write_byte(cmd, OLED_CMD_SET_CHARGE_PUMP_ON, true);

    i2c_master_write_byte(cmd, OLED_CMD_SET_SEGMENT_REMAP_INVERSE, true); // reverse left-right mapping
    i2c_master_write_byte(cmd, OLED_CMD_SET_COM_SCAN_MODE_REVERSE, true); // reverse up-bottom mapping

    i2c_master_write_byte(cmd, OLED_CMD_DISPLAY_ON, true);

    i2c_master_write_byte(cmd, 0x00, true); // reset column low bits
    i2c_master_write_byte(cmd, 0x10, true); // reset column high bits
    i2c_master_write_byte(cmd, 0xB0, true); // reset page
    i2c_master_write_byte(cmd, 0x40, true); // set start line
    i2c_master_write_byte(cmd, OLED_CMD_SET_DISPLAY_OFFSET, true);
    i2c_master_write_byte(cmd, 0x00, true);

    i2c_master_stop(cmd);

    err = i2c_master_cmd_begin(i2c_port, cmd, 100 / portTICK_PERIOD_MS); //10
    if (err == ESP_OK)
    {
        ESP_LOGI(I2C_TAG, "OLED configured successfully");
    }
    else
    {
        ESP_LOGE(I2C_TAG, "OLED configuration failed. code: 0x%.2X", err);
    }
    i2c_cmd_link_delete(cmd);
}

void sh1106_display_pattern_task(void *ignore)
{
    i2c_cmd_handle_t cmd;

    for (uint8_t i = 0; i < 8; i++)
    {
        cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_SINGLE, true);
        i2c_master_write_byte(cmd, 0xB0 | i, true);
        i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
        for (uint8_t j = 0; j < 132; j++)
        {
            i2c_master_write_byte(cmd, 0xFF >> (j % 8), true);
        }
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(i2c_port, cmd, 100 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
    }
}

void sh1106_display_clear_task(void *ignore)
{
    i2c_cmd_handle_t cmd;

    uint8_t zero[132];
    memset(zero, 0, 132);
    for (uint8_t i = 0; i < 8; i++)
    {
        cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_SINGLE, true);
        i2c_master_write_byte(cmd, 0xB0 | i, true);

        i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
        i2c_master_write(cmd, zero, 132, true);
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(i2c_port, cmd, 100 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
    }

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
    i2c_master_write_byte(cmd, 0x00, true); // reset column
    i2c_master_write_byte(cmd, 0x10, true);
    i2c_master_write_byte(cmd, 0xB0, true); // reset page
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(i2c_port, cmd, 100 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

void sh1106_contrast_task(void *ignore)
{
    i2c_cmd_handle_t cmd;

    uint8_t contrast = 0;
    uint8_t direction = 1;
    while (true)
    {
        cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
        i2c_master_write_byte(cmd, OLED_CMD_SET_CONTRAST, true);
        i2c_master_write_byte(cmd, contrast, true);
        i2c_master_stop(cmd);
        i2c_master_cmd_begin(i2c_port, cmd, 100 / portTICK_PERIOD_MS);
        i2c_cmd_link_delete(cmd);
        vTaskDelay(1 / portTICK_PERIOD_MS);

        contrast += direction;
        if (contrast == 0xFF)
        {
            direction = -1;
        }
        if (contrast == 0x0)
        {
            direction = 1;
        }
    }
    vTaskDelete(NULL);
}

void sh1106_display_text_task(const void *arg_text)
{
    char *text = (char *)arg_text;
    uint8_t text_len = strlen(text);

    i2c_cmd_handle_t cmd;

    uint8_t cur_page = 0;

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);

    i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
    i2c_master_write_byte(cmd, 0x08, true); // reset column
    i2c_master_write_byte(cmd, 0x10, true);
    i2c_master_write_byte(cmd, 0xB0 | cur_page, true); // reset page

    i2c_master_stop(cmd);
    i2c_master_cmd_begin(i2c_port, cmd, 100 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    for (uint8_t i = 0; i < text_len; i++)
    {
        if (text[i] == '\n')
        {
            cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);

            i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
            i2c_master_write_byte(cmd, 0x08, true); // reset column
            i2c_master_write_byte(cmd, 0x10, true);
            i2c_master_write_byte(cmd, 0xB0 | ++cur_page, true); // increment page

            i2c_master_stop(cmd);
            i2c_master_cmd_begin(i2c_port, cmd, 100 / portTICK_PERIOD_MS);
            i2c_cmd_link_delete(cmd);
        }
        else
        {
            cmd = i2c_cmd_link_create();
            i2c_master_start(cmd);
            i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);

            i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
            i2c_master_write(cmd, font8x8_basic_tr[(uint8_t)text[i]], 8, true);

            i2c_master_stop(cmd);
            i2c_master_cmd_begin(i2c_port, cmd, 100 / portTICK_PERIOD_MS);
            i2c_cmd_link_delete(cmd);
        }
    }
}

#if BATTERY_DISPLAY
void pcf8591_init()
{
    esp_err_t espRc;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (0x48 << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x40, true);
    i2c_master_stop(cmd);

    espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_PERIOD_MS);
    if (espRc == ESP_OK)
    {
        ESP_LOGI(I2C_TAG, "pcf8591 configured successfully");
    }
    else
    {
        ESP_LOGE(I2C_TAG, "pcf8591 configuration failed. code: 0x%.2X", espRc);
    }
    i2c_cmd_link_delete(cmd);
}
#define MAX_V_PIN 163
#define MIN_V_PIN 116
#define OFFSET_V_PIN 4

void pcf8591_read()
{
    static uint16_t count = 0;
    static uint16_t sum = 0;
    esp_err_t espRc;
    uint8_t data = 0;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (0x48 << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &data, true);
    i2c_master_stop(cmd);

    espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10 / portTICK_PERIOD_MS);
    if (espRc == ESP_OK)
    {
        //ESP_LOGI(I2C_TAG, "data %d",data);
        count++;
        sum += data;
        if (count == 30)
        {
            //ESP_LOGI(I2C_TAG, "pin %d",(int)(sum/30.0));
            int pin = (int)((sum / 30.0 - OFFSET_V_PIN - MIN_V_PIN) / (MAX_V_PIN - MIN_V_PIN) * 100);
            if (pin < 0)
                pin = 0;
            if (pin > 100)
                pin = 100;
            char text[20];
            sprintf(text, "\n\n\n\n\nPin : %3d%%", pin);
            sh1106_display_text_task(text);
            count = 0;
            sum = 0;
        }
    }
    else
    {
        ESP_LOGE(I2C_TAG, "pcf8591 read fail. code: 0x%.2X", espRc);
    }
    i2c_cmd_link_delete(cmd);
}
#endif

//void oled_display_task(esp_periph_handle_t *param)
void oled_display_task(char id[])
{
    
    i2c_master_init();

    sh1106_init();

#if BATTERY_DISPLAY
    pcf8591_init();
#endif

    sh1106_display_pattern_task(NULL);

    sh1106_display_clear_task(NULL);
     char str[120] = "\nYTEMOI\n\nID:";
    strcat(str,id);
    sh1106_display_text_task(str);
}

void oled_display_connect(char id[])
{
    sh1106_display_clear_task(NULL);
     char str[120] = "\nYTEMOI\n\nID:";
    char str_l[120] = "\n\nCONNECT\n";
    strcat(str,id);
    strcat(str,str_l);
    sh1106_display_text_task(str);
}
void oled_display_disconnect(char id[]){
    sh1106_display_clear_task(NULL);
    char str[120] = "\nYTEMOI\n\nID:";
    char str_l[120] = "\n\nDISCONNECT\n";
    strcat(str,id);
    strcat(str,str_l);
    sh1106_display_text_task(str);
}

void display_ringing_info(){
    sh1106_display_clear_task(NULL);
    char str_l[120] = "\nCUOC GOI TU \n\n BS -YT";
    sh1106_display_text_task(str_l);
}
void display_send_noti(){
    sh1106_display_clear_task(NULL);
    char str_l[120] = "\nGUI THONG BAO \n\n BS -YT";
    sh1106_display_text_task(str_l);
}
// void display_ringing_info()
// {
//     // i2c_master_init();

//     // sh1106_init();

//     //vTaskDelay(1000 / portTICK_PERIOD_MS);

//     //TODO: Add display calling request here
//     if (get_call_ringing_started())
//     {
//         sh1106_display_clear_task(NULL);
//         sh1106_display_text_task("\nThong bao:\n\nCo cuoc goi tu\n\nY ta - Bac si\n");
//         ESP_LOGI(I2C_TAG, "Display calling information");

//         vTaskDelay(3000 / portTICK_PERIOD_MS);

//         // #if PLAY_SOUND_ENABLE
//         //         play_sound_mp3_task();
//         // #endif

//         sh1106_display_clear_task(NULL);
//         sh1106_display_text_task("\nBach Duong IoT\n\nINTERNET: ON\n");
//     }
// }

extern bool get_set_btn_status();

void display_connecting_info()
{
    if (get_set_btn_status())
    {
        sh1106_display_clear_task(NULL);
        sh1106_display_text_task("\n\nDang ket noi voi\n\nY ta - Bac si\n");
        ESP_LOGI(I2C_TAG, "Display calling information");

        vTaskDelay(3000 / portTICK_PERIOD_MS);

#if PLAY_SOUND_ENABLE
        //play_sound_mp3_task();
        //play_sound_notice_task();
#endif

        sh1106_display_clear_task(NULL);
        sh1106_display_text_task("\nBach Duong IoT\n\nINTERNET: ON\n");
    }
}
//I2C-Display
