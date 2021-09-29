/*This is tone file*/

const char* tone_uri[] = {
   "flash://tone/0_nusecall_connect_succes.mp3",
   "flash://tone/1_nusecall_disconnect.mp3",
   "flash://tone/2_nusecall_noti.mp3",
   "flash://tone/3_ring_sound.mp3",
};

int get_tone_uri_num()
{
    return sizeof(tone_uri) / sizeof(char *) - 1;
}
