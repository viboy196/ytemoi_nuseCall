#ifndef __AUDIO_TONEURI_H__
#define __AUDIO_TONEURI_H__

extern const char* tone_uri[];

typedef enum {
    TONE_TYPE_NUSECALL_CONNECT_SUCCES,
    TONE_TYPE_NUSECALL_DISCONNECT,
    TONE_TYPE_NUSECALL_NOTI,
    TONE_TYPE_RING_SOUND,
    TONE_TYPE_MAX,
} tone_type_t;

int get_tone_uri_num();

#endif
