#ifndef SOUND_SERVER_INTERFACE_H
#define SOUND_SERVER_INTERFACE_H

#include "sound_server_messages.h"

#ifdef __cplusplus
extern "C" {
#endif


void carmen_sound_play_file(char *filename);
void carmen_sound_speech_synth(char *text);
void carmen_sound_set_volume(int volume);

#ifdef __cplusplus
}
#endif

#endif
