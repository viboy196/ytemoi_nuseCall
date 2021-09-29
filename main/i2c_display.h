#ifndef _I2C_DISPLAY_H_
#define _I2C_DISPLAY_H_

void oled_display_task(char id[]);

void oled_display_connect(char id[]);
void oled_display_disconnect(char id[]);
void display_ringing_info();
void display_send_noti();

#endif