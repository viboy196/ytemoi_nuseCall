/* VoIP Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "audio_mem.h"
#include "i2s_stream.h"
#include "esp_peripherals.h"
#include "periph_wifi.h"
#include "board.h"
#include "input_key_service.h"

#include "raw_stream.h"
#include "filter_resample.h"
#include "esp_sip.h"
#include "g711_decoder.h"
#include "g711_encoder.h"
#include "algorithm_stream.h"

#include "wifi_service.h"
#include "smart_config.h"

#include "audio_tone_uri.h"
#include "audio_player_int_tone.h"

#include "http_post_service.h"

#if __has_include("esp_idf_version.h")
#include "esp_idf_version.h"
#else
#define ESP_IDF_VERSION_VAL(major, minor, patch) 1
#endif

#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0))
#include "esp_netif.h"
#else
#include "tcpip_adapter.h"
#endif
// display
#include "i2c_display.h"
// display
static const char *TAG = "VOIP_EXAMPLE";

#define I2S_SAMPLE_RATE 16000
#define I2S_CHANNELS 2
#define I2S_BITS 16

#define CODEC_SAMPLE_RATE 8000
#define CODEC_CHANNELS 1

static sip_handle_t sip;
static audio_element_handle_t raw_read, raw_write;
static audio_pipeline_handle_t recorder, player;
static bool is_smart_config;
static display_service_handle_t disp;
static periph_service_handle_t wifi_serv;
static char NCB_ID[40];
static esp_err_t recorder_pipeline_open()
{
    audio_element_handle_t i2s_stream_reader;
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    recorder = audio_pipeline_init(&pipeline_cfg);
    AUDIO_NULL_CHECK(TAG, recorder, return ESP_FAIL);

    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_READER;
    i2s_cfg.uninstall_drv = false;
#if defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    i2s_cfg.i2s_port = 1;
    i2s_cfg.task_core = 1;
#endif
    i2s_cfg.i2s_config.sample_rate = I2S_SAMPLE_RATE;
    i2s_stream_reader = i2s_stream_init(&i2s_cfg);

#if defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    algorithm_stream_cfg_t algo_config = ALGORITHM_STREAM_CFG_DEFAULT();
    algo_config.input_type = ALGORITHM_STREAM_INPUT_TYPE1;
    algo_config.task_core = 1;
    audio_element_handle_t element_algo = algo_stream_init(&algo_config);
#endif

    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_cfg.src_rate = I2S_SAMPLE_RATE;
#if defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    rsp_cfg.src_ch = 1;
#else
    rsp_cfg.src_ch = I2S_CHANNELS;
#endif
    rsp_cfg.dest_rate = CODEC_SAMPLE_RATE;
    rsp_cfg.dest_ch = CODEC_CHANNELS;
    rsp_cfg.complexity = 5;
    rsp_cfg.task_core = 1;
    audio_element_handle_t filter = rsp_filter_init(&rsp_cfg);

    g711_encoder_cfg_t g711_cfg = DEFAULT_G711_ENCODER_CONFIG();
    g711_cfg.task_core = 1;
    audio_element_handle_t sip_encoder = g711_encoder_init(&g711_cfg);

    raw_stream_cfg_t raw_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_cfg.type = AUDIO_STREAM_READER;
    raw_read = raw_stream_init(&raw_cfg);
    audio_element_set_output_timeout(raw_read, portMAX_DELAY);

    audio_pipeline_register(recorder, i2s_stream_reader, "i2s");
    audio_pipeline_register(recorder, filter, "filter");
    audio_pipeline_register(recorder, sip_encoder, "sip_enc");
    audio_pipeline_register(recorder, raw_read, "raw");

#if defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    audio_pipeline_register(recorder, element_algo, "algo");
    algo_stream_set_record_rate(element_algo, I2S_CHANNELS, I2S_SAMPLE_RATE);
    const char *link_tag[5] = {"i2s", "algo", "filter", "sip_enc", "raw"};
    audio_pipeline_link(recorder, &link_tag[0], 5);
#else
    const char *link_tag[4] = {"i2s", "filter", "sip_enc", "raw"};
    audio_pipeline_link(recorder, &link_tag[0], 4);
#endif

    audio_pipeline_run(recorder);
    ESP_LOGI(TAG, " SIP recorder has been created");
    return ESP_OK;
}

static esp_err_t player_pipeline_open()
{
    audio_element_handle_t i2s_stream_writer;
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    player = audio_pipeline_init(&pipeline_cfg);
    AUDIO_NULL_CHECK(TAG, player, return ESP_FAIL);

    raw_stream_cfg_t raw_cfg = RAW_STREAM_CFG_DEFAULT();
    raw_cfg.type = AUDIO_STREAM_WRITER;
    raw_write = raw_stream_init(&raw_cfg);

    g711_decoder_cfg_t g711_cfg = DEFAULT_G711_DECODER_CONFIG();
    audio_element_handle_t sip_decoder = g711_decoder_init(&g711_cfg);

    rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
    rsp_cfg.src_rate = CODEC_SAMPLE_RATE;
    rsp_cfg.src_ch = CODEC_CHANNELS;
    rsp_cfg.dest_rate = I2S_SAMPLE_RATE;
    rsp_cfg.dest_ch = I2S_CHANNELS;
    rsp_cfg.complexity = 5;
    audio_element_handle_t filter = rsp_filter_init(&rsp_cfg);

    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    i2s_cfg.uninstall_drv = false;
    i2s_cfg.i2s_config.sample_rate = I2S_SAMPLE_RATE;
    i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    audio_pipeline_register(player, raw_write, "raw");
    audio_pipeline_register(player, sip_decoder, "sip_dec");
    audio_pipeline_register(player, filter, "filter");
    audio_pipeline_register(player, i2s_stream_writer, "i2s");
    const char *link_tag[4] = {"raw", "sip_dec", "filter", "i2s"};
    audio_pipeline_link(player, &link_tag[0], 4);
    audio_pipeline_run(player);
    ESP_LOGI(TAG, "SIP player has been created");
    return ESP_OK;
}

static ip4_addr_t _get_network_ip()
{
    tcpip_adapter_ip_info_t ip;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
    return ip.ip;
}

static int ringring_count = 0;

static int _sip_event_handler(sip_event_msg_t *event)
{
    ip4_addr_t ip;
    switch ((int)event->type)
    {
    case SIP_EVENT_REQUEST_NETWORK_STATUS:
        ESP_LOGD(TAG, "SIP_EVENT_REQUEST_NETWORK_STATUS");
        ip = _get_network_ip();
        if (ip.addr)
        {
            return true;
        }
        return ESP_OK;
    case SIP_EVENT_REQUEST_NETWORK_IP:
        ESP_LOGD(TAG, "SIP_EVENT_REQUEST_NETWORK_IP");
        ip = _get_network_ip();
        int ip_len = sprintf((char *)event->data, "%s", ip4addr_ntoa(&ip));
        return ip_len;
    case SIP_EVENT_REGISTERED:
        ESP_LOGI(TAG, "SIP_EVENT_REGISTERED");
        // k???t n???i th??nh c??ng sip
        audio_player_int_tone_play(tone_uri[TONE_TYPE_NUSECALL_CONNECT_SUCCES]);
        break;
    case SIP_EVENT_RINGING:
        ESP_LOGI(TAG, "ringing... RemotePhoneNum %s", (char *)event->data);
        // nh???n g???i tho???i
        if (ringring_count == 0)
        {
            display_ringing_info();
            audio_player_int_tone_play(tone_uri[TONE_TYPE_RING_SOUND]);
        }
        ringring_count++;
        if (ringring_count == 4)
        {
            oled_display_connect(NCB_ID);
            audio_player_int_tone_stop();
            esp_sip_uas_answer(sip, true);
        }

        break;
    case SIP_EVENT_INVITING:
        ESP_LOGI(TAG, "SIP_EVENT_INVITING Remote Ring...");
        break;
    case SIP_EVENT_BUSY:
        ESP_LOGI(TAG, "SIP_EVENT_BUSY");
        break;
    case SIP_EVENT_HANGUP:
        ringring_count = 0;
        ESP_LOGI(TAG, "SIP_EVENT_HANGUP");
        break;
    case SIP_EVENT_AUDIO_SESSION_BEGIN:
        ESP_LOGI(TAG, "SIP_EVENT_AUDIO_SESSION_BEGIN");
        player_pipeline_open();
        recorder_pipeline_open();
        break;
    case SIP_EVENT_AUDIO_SESSION_END:
        ringring_count = 0;
        ESP_LOGI(TAG, "SIP_EVENT_AUDIO_SESSION_END");
        audio_pipeline_stop(player);
        audio_pipeline_wait_for_stop(player);
        audio_pipeline_deinit(player);
        audio_pipeline_stop(recorder);
        audio_pipeline_wait_for_stop(recorder);
        audio_pipeline_deinit(recorder);
        break;
    case SIP_EVENT_READ_AUDIO_DATA:
        return raw_stream_read(raw_read, (char *)event->data, event->data_len);
    case SIP_EVENT_WRITE_AUDIO_DATA:
        return raw_stream_write(raw_write, (char *)event->data, event->data_len);
    case SIP_EVENT_READ_DTMF:
        ESP_LOGI(TAG, "SIP_EVENT_READ_DTMF ID : %d ", ((char *)event->data)[0]);
        break;
    }
    return 0;
}

static esp_err_t input_key_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx)
{
    audio_board_handle_t board_handle = (audio_board_handle_t)ctx;
    int player_volume;
    if (evt->type == INPUT_KEY_SERVICE_ACTION_CLICK_RELEASE)
    {
        ESP_LOGD(TAG, "[ * ] input key id is %d", (int)evt->data);
        sip_state_t sip_state = esp_sip_get_state(sip);
        if (sip_state < SIP_STATE_REGISTERED)
        {
            return ESP_OK;
        }
        switch ((int)evt->data)
        {
        case INPUT_KEY_USER_ID_REC:
            //                 ESP_LOGI(TAG, "[ * ] [Rec] Set MIC Mute or not");
            //                 if (mute) {
            //                     mute = false;
            //                     display_service_set_pattern(disp, DISPLAY_PATTERN_TURN_OFF, 0);
            //                 } else {
            //                     mute = true;
            //                     display_service_set_pattern(disp, DISPLAY_PATTERN_TURN_ON, 0);
            //                 }
            // #if defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
            //                 audio_hal_set_mute(board_handle->adc_hal, mute);
            // #endif
            //                 break;
            //             case INPUT_KEY_USER_ID_PLAY:
            //                 ESP_LOGI(TAG, "[ * ] [Play] input key event");
            //                 if (sip_state == SIP_STATE_RINGING) {
            //                     audio_player_int_tone_stop();
            //                     esp_sip_uas_answer(sip, true);
            //                 } else if (sip_state == SIP_STATE_REGISTERED) {
            //                     esp_sip_uac_invite(sip, "101");
            //                 }ESP_LOGW(TAG, "nhan nut");
            if (sip_state == SIP_STATE_RINGING || sip_state == SIP_STATE_ON_CALL)
            {
            }
            else
            {
                display_send_noti();
                http_post_send_info_task(NCB_ID);
                audio_player_int_tone_play(tone_uri[TONE_TYPE_NUSECALL_NOTI]);
                oled_display_connect(NCB_ID);
            }
            break;
        case INPUT_KEY_USER_ID_MODE:
            if (sip_state & SIP_STATE_RINGING)
            {
                audio_player_int_tone_stop();
                esp_sip_uas_answer(sip, false);
            }
            else if (sip_state & SIP_STATE_ON_CALL)
            {
                esp_sip_uac_bye(sip);
            }
            else if ((sip_state & SIP_STATE_CALLING) || (sip_state & SIP_STATE_SESS_PROGRESS))
            {
                esp_sip_uac_cancel(sip);
            }
            break;
        case INPUT_KEY_USER_ID_VOLUP:
            ESP_LOGD(TAG, "[ * ] [Vol+] input key event");
            audio_hal_get_volume(board_handle->audio_hal, &player_volume);
            player_volume += 10;
            if (player_volume > 100)
            {
                player_volume = 100;
            }
            audio_hal_set_volume(board_handle->audio_hal, player_volume);
            ESP_LOGI(TAG, "[ * ] Volume set to %d %%", player_volume);
            break;
        case INPUT_KEY_USER_ID_VOLDOWN:
            ESP_LOGD(TAG, "[ * ] [Vol-] input key event");
            audio_hal_get_volume(board_handle->audio_hal, &player_volume);
            player_volume -= 10;
            if (player_volume < 0)
            {
                player_volume = 0;
            }
            audio_hal_set_volume(board_handle->audio_hal, player_volume);
            ESP_LOGI(TAG, "[ * ] Volume set to %d %%", player_volume);
            break;
        }
    }
    else if (evt->type == INPUT_KEY_SERVICE_ACTION_PRESS)
    {
        switch ((int)evt->data)
        {
        case INPUT_KEY_USER_ID_SET:
            is_smart_config = true;
            esp_sip_destroy(sip);
            wifi_service_setting_start(wifi_serv, 0);
            // v??o ch??? ????? smart config
            //audio_player_int_tone_play(tone_uri[TONE_TYPE_UNDER_SMARTCONFIG]);
            break;
        }
    }

    return ESP_OK;
}
int esp_count_disconnect = 0;
static esp_err_t wifi_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx)
{
    ESP_LOGD(TAG, "event type:%d,source:%p, data:%p,len:%d,ctx:%p",
             evt->type, evt->source, evt->data, evt->len, ctx);
    if (evt->type == WIFI_SERV_EVENT_CONNECTED)
    {
        oled_display_connect(NCB_ID);
        ESP_LOGI(TAG, "PERIPH_WIFI_CONNECTED [%d]", __LINE__);
        is_smart_config = false;
        ESP_LOGI(TAG, "[ 5 ] Create SIP Service");
        sip_config_t sip_cfg = {
            .uri = "udp://800:800ytm@171.244.133.171:5060",
            .event_handler = _sip_event_handler,
            .send_options = true,
#ifdef CONFIG_SIP_CODEC_G711A
            .acodec_type = SIP_ACODEC_G711A,
#else
            .acodec_type = SIP_ACODEC_G711U,
#endif
        };
        sip = esp_sip_init(&sip_cfg);
        esp_sip_start(sip);
    }
    else if (evt->type == WIFI_SERV_EVENT_DISCONNECTED)
    {
        esp_count_disconnect++;
        printf("chi so esp_count_disconnect : %d",esp_count_disconnect);
        ESP_LOGI(TAG, "PERIPH_WIFI_DISCONNECTED [%d]", __LINE__);
        oled_display_disconnect(NCB_ID);
        if (is_smart_config == false)
        {
            // b??? m???t m???ng g???i c??i ?????t l???i
            //audio_player_int_tone_play(tone_uri[TONE_TYPE_PLEASE_SETTING_WIFI]);
        }
        esp_sip_destroy(sip);
        //wifi_service_setting_start(wifi_serv, 0);
    }
    else if (evt->type == WIFI_SERV_EVENT_SETTING_TIMEOUT)
    {
        ESP_LOGW(TAG, "WIFI_SERV_EVENT_SETTING_TIMEOUT [%d]", __LINE__);
        oled_display_disconnect(NCB_ID);
        // b??? m???t m???ng g???i c??i ?????t l???i
        //audio_player_int_tone_play(tone_uri[TONE_TYPE_PLEASE_SETTING_WIFI]);
        is_smart_config = false;
        esp_restart();
    }

    return ESP_OK;
}

void setup_wifi()
{
    int reg_idx = 0;
    wifi_service_config_t cfg = WIFI_SERVICE_DEFAULT_CONFIG();
    cfg.evt_cb = wifi_service_cb;
    cfg.setting_timeout_s = 15;
    cfg.max_retry_time = 2;
    wifi_serv = wifi_service_create(&cfg);

    smart_config_info_t info = SMART_CONFIG_INFO_DEFAULT();
    esp_wifi_setting_handle_t h = smart_config_create(&info);
    esp_wifi_setting_regitster_notify_handle(h, (void *)wifi_serv);
    wifi_service_register_setting_handle(wifi_serv, h, &reg_idx);

    wifi_config_t sta_cfg = {0};
    strncpy((char *)&sta_cfg.sta.ssid, CONFIG_WIFI_SSID, sizeof(sta_cfg.sta.ssid));
    strncpy((char *)&sta_cfg.sta.password, CONFIG_WIFI_PASSWORD, sizeof(sta_cfg.sta.password));
    wifi_service_set_sta_info(wifi_serv, &sta_cfg);
    wifi_service_connect(wifi_serv);
}
// read wite//
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
// read wite
void Wite_String(char type[], char type_length[], char value[])
{
    nvs_handle my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("wite Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        //char anhdan[8] = "12345678";
        err = nvs_set_str(my_handle, type, value);
        printf((err != ESP_OK) ? " wite ghi anh dan Failed!\n" : "wite ghi anh dan Done\n");

        ESP_ERROR_CHECK(err);
        err = nvs_set_i32(my_handle, type_length, strlen(value));
        printf((err != ESP_OK) ? "wite ghi anh dan_lengh Failed!\n" : "wite ghi anh dan_lengt Done\n");
        ESP_ERROR_CHECK(err);
        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        printf("wite Committing updates in NVS ... ");

        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "wite Failed!\n" : "wite Done\n");
        // Close
        nvs_close(my_handle);
    }
}

void app_main()
{
    esp_log_level_set("*", ESP_LOG_INFO);

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0))
    ESP_ERROR_CHECK(esp_netif_init());
#else
    tcpip_adapter_init();
#endif

    Wite_String("NCB_ID", "NCB_ID_Length", "NCB0000001");
    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        int32_t length = 0;
        err = nvs_get_i32(my_handle, "NCB_ID_Length", &length);

        char ssid_t[length + 1]; //include '\0' - NULL terminator
        size_t length_t = sizeof(ssid_t);
        err = nvs_get_str(my_handle, "NCB_ID", ssid_t, &length_t);

        switch (err)
        {
        case ESP_OK:
            printf("Done ssid_t da nhan dc ??c ??c %s \n", ssid_t);
            strncpy(NCB_ID, ssid_t, length_t);
            printf("Done NCB_ID da nhan dc ??c ??c %s \n", NCB_ID);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            printf("The value is not initialized yet!\n");
            break;
        default:
            printf("Error (%s) reading!\n", esp_err_to_name(err));
        }

        nvs_close(my_handle);
    }

    ESP_LOGI(TAG, "[1.0] Initialize peripherals management");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    ESP_LOGI(TAG, "[1.1] Initialize and start peripherals");
    audio_board_key_init(set);

    ESP_LOGI(TAG, "[1.2] Create and start input key service");
    input_key_service_info_t input_key_info[] = INPUT_KEY_DEFAULT_INFO();
    input_key_service_cfg_t input_cfg = INPUT_KEY_SERVICE_DEFAULT_CONFIG();
    input_cfg.handle = set;
    input_cfg.based_cfg.task_stack = 4 * 1024;
    periph_service_handle_t input_ser = input_key_service_create(&input_cfg);

    ESP_LOGI(TAG, "[ 1.3 ] Create display service instance");
    disp = audio_board_led_init();

    ESP_LOGI(TAG, "[ 2 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);
    audio_hal_set_volume(board_handle->audio_hal, 70);

    input_key_service_add_key(input_ser, input_key_info, INPUT_KEY_NUM);
    periph_service_set_callback(input_ser, input_key_service_cb, (void *)board_handle);

    ESP_LOGI(TAG, "[ 3 ] Initialize tone player");
    audio_player_int_tone_init();

    ESP_LOGI(TAG, "[ 4 ] Create Wi-Fi service instance");
    setup_wifi();
    oled_display_task(NCB_ID);
}
