#ifndef _HTTP_POST_SERVICE_H
#define _HTTP_POST_SERVICE_H

#define HTTP_POST_ENABLE 1

// void http_post_task(void *pvParameters);
void http_post_add_device_task();
void http_post_send_info_task(char id[]);


#endif