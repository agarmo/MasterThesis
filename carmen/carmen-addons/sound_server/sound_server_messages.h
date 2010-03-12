
#ifndef SOUND_SERVER_MESSAGES_H
#define SOUND_SERVER_MESSAGES_H


#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
  char *filename;
  double timestamp;
  char host[10];
} carmen_sound_file_msg;

#define CARMEN_SOUND_FILE_MSG_NAME "carmen_sound_file_msg"
#define CARMEN_SOUND_FILE_MSG_FMT  "{string, double, [char:10]}"

typedef struct {
  char *text;
  double timestamp;
  char host[10];
} carmen_sound_speech_synth_msg;

#define CARMEN_SOUND_SPEECH_SYNTH_MSG_NAME "carmen_sound_speech_synth_msg"
#define CARMEN_SOUND_SPEECH_SYNTH_MSG_FMT  "{string, double, [char:10]}"

typedef struct {
  int volume;  // from 0 to 100 (percent)
  double timestamp;
  char host[10];
} carmen_sound_volume_msg;

#define CARMEN_SOUND_VOLUME_MSG_NAME "carmen_sound_volume_msg"
#define CARMEN_SOUND_VOLUME_MSG_FMT  "{int, double, [char:10]}"


#ifdef __cplusplus
}
#endif

#endif
