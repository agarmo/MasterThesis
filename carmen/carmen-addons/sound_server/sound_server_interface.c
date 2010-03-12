#include <carmen/carmen.h>
#include "sound_server_interface.h"


void carmen_sound_play_file(char *filename) {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_sound_file_msg msg;

  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  msg.filename = filename;

  err = IPC_defineMsg(CARMEN_SOUND_FILE_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_SOUND_FILE_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_SOUND_FILE_MSG_NAME);

  err = IPC_publishData(CARMEN_SOUND_FILE_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_SOUND_FILE_MSG_NAME);
}

void carmen_sound_speech_synth(char *text) {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_sound_speech_synth_msg msg;

  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  msg.text = text;

  err = IPC_defineMsg(CARMEN_SOUND_SPEECH_SYNTH_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_SOUND_SPEECH_SYNTH_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_SOUND_SPEECH_SYNTH_MSG_NAME);

  err = IPC_publishData(CARMEN_SOUND_SPEECH_SYNTH_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_SOUND_SPEECH_SYNTH_MSG_NAME);
}

void carmen_sound_set_volume(int volume) {

  IPC_RETURN_TYPE err = IPC_OK;
  carmen_sound_volume_msg msg;

  msg.timestamp = carmen_get_time_ms();
  strcpy(msg.host, carmen_get_tenchar_host_name());

  msg.volume = volume;

  err = IPC_defineMsg(CARMEN_SOUND_VOLUME_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_SOUND_VOLUME_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_SOUND_VOLUME_MSG_NAME);

  err = IPC_publishData(CARMEN_SOUND_VOLUME_MSG_NAME, &msg);
  carmen_test_ipc(err, "Could not publish", CARMEN_SOUND_VOLUME_MSG_NAME);
}
