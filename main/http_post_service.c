#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "esp_http_client.h"
#include "http_post_service.h"

/*Constants related to HTTP API Connection*/
// #define WEB_SERVER "27.71.228.66:8099" //"ytemoi.com"
// #define WEB_PORT "443"
// #define WEB_URL "https://ytemoi.com/api/iot"
#define WEB_URL_ADD_DEVICE "http://27.71.228.66:8099/api/Device/add"

//// global
#define WEB_URL_SEND_INFO "https://ytemoi.com/api/ncb"

// local
//#define WEB_URL_SEND_INFO "http://192.168.137.1:8080/api/ListenBox/"

// #define WEB_API "/api/iot"
// #define WEB_API_ADD_DEVICE "/api/Device/add"

// #define WEB_API_SEND_INFO "/api/Device/send-information"

// #define device_name "ncall-5f-5a-4b-2d"
// #define device_code "300@5f5a4b2d"

extern bool get_set_btn_status();

static const char *HTTP_TAG = "HTTP_POST_ESP32";

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // Write out data
            // printf("%.*s", evt->data_len, (char*)evt->data);
        }

        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(HTTP_TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    }
    return ESP_OK;
}

//void http_post_task(void *pvParameters)
void http_post_add_device_task()
{
    // char buf[1];
    // int ret, len;

    // char post_data[256];
    //char post_body[50] = "{\"number\":\"100:100\"}";
    // char post_body[100] = "{\"id\":\"5f5a4b363cd2f414d0115892\",\"type\":\"nurse-call\",\"value\":\"100@100\"}";
    // sprintf(post_data, "POST " WEB_API " HTTP/1.1\r\n"
    //                    "Host: " WEB_SERVER "\r\n"
    //                    "User-Agent: esp32-ncall\r\n"
    //                    "Content-Type: application/json\r\n"
    //                    "Accept: text/plain\r\n"
    //                    "Charset=utf-8\r\n"
    //                    "Content-Length: %d\r\n"
    //                    "\r\n"
    //                    "%s\r\n",
    //         strlen(post_body), post_body);

    char post_body[120] = "{\"id\":\"NCB0000001\", \"value\":\"800\", \"type\":\"NCBG_SVG_ButtonSignal_Click\"}"; //khoa tim mach - benh vien thu nghiem

    esp_http_client_config_t config = {
        .url = WEB_URL_ADD_DEVICE,
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // if (!get_set_btn_status())
    // {
    //     vTaskDelay(500 / portTICK_PERIOD_MS);
    //     // continue;
    // }
    // else
    {
        // ESP_LOGI(HTTP_TAG, "Post Request:\r\n%s", post_data);
        ESP_LOGI(HTTP_TAG, "Post Request:\r\n%s", post_body);

        esp_http_client_set_method(client, HTTP_METHOD_POST);

        esp_http_client_set_header(client, "content-type", "application/json");

        esp_http_client_set_header(client, "charset", "utf-8");

        esp_http_client_set_header(client, "accept", "text/plain");

        esp_http_client_set_post_field(client, post_body, strlen(post_body));

        esp_err_t err = esp_http_client_perform(client);

        if (err == ESP_OK)
        {
            ESP_LOGI(HTTP_TAG, "HTTP POST Status = %d, content_length = %d",
                     esp_http_client_get_status_code(client),
                     esp_http_client_get_content_length(client));
        }
        else
        {
            ESP_LOGE(HTTP_TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
            //goto exit;

            for (int countdown = 60; countdown >= 0; countdown--)
            {
                ESP_LOGI(HTTP_TAG, "%d...", countdown);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
            }
            // gpio_set_level(LED_GPIO_OUTPUT, 1);
            //}
            //set_btn_pressed = false;

            ESP_LOGI(HTTP_TAG, "Trying again!");
        }

        esp_http_client_cleanup(client);
    }
}

void http_post_send_info_task(char id[])
{
    ESP_LOGI(HTTP_TAG, "http_post_send_info_task start");
    char post_body[120] =  "{\"id\":\"" ; 
    char post_bodyl[120] = "\", \"value\":\"800\", \"type\":\"NCBG_SVG_ButtonSignal_Click\"}"; 
    strcat(post_body,id);
    strcat(post_body,post_bodyl);
    esp_http_client_config_t config = {
        .url = WEB_URL_SEND_INFO,
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // if (!get_set_btn_status())
    // {
    //     vTaskDelay(500 / portTICK_PERIOD_MS);
    //     ESP_LOGI(HTTP_TAG, "khong dc phep gui");
    //     // continue;
    // }
    // else
    // {
    ESP_LOGI(HTTP_TAG, "duoc phep gui");
    // ESP_LOGI(HTTP_TAG, "Post Request:\r\n%s", post_data);
    ESP_LOGI(HTTP_TAG, "Post Request:\r\n%s", post_body);

    esp_http_client_set_method(client, HTTP_METHOD_POST);

    esp_http_client_set_header(client, "content-type", "application/json");

    esp_http_client_set_header(client, "charset", "utf-8");

    esp_http_client_set_header(client, "accept", "text/plain");

    esp_http_client_set_post_field(client, post_body, strlen(post_body));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK)
    {
        ESP_LOGI(HTTP_TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(HTTP_TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        //goto exit;

        for (int countdown = 60; countdown >= 0; countdown--)
        {
            ESP_LOGI(HTTP_TAG, "%d...", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        // gpio_set_level(LED_GPIO_OUTPUT, 1);
        //}
        //set_btn_pressed = false;

        ESP_LOGI(HTTP_TAG, "Trying again!");
    }

    esp_http_client_cleanup(client);
    // }
}
