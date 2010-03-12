#include <carmen/carmen.h>
#include <carmen/walker_interface.h>

int main(int argc, char **argv) {

  int guidance_config;

  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  if (argc < 3)
    carmen_die("usage: imp_goto <guidance config> <goal name>\n"
	       "guidance config:\n"
	       "    baseline"
	       "    arrow"
	       "    text_ai"
	       "    text_woz"
	       "    audio"
	       "    audio_arrow"
	       "    audio_text_ai"
	       "    audio_text_woz\n");

  guidance_config = SQUEEZE_HANDLES_DELAY;
  if (strstr(argv[1], "arrow"))
    guidance_config |= ARROW_GUIDANCE;
  else if (strstr(argv[1], "text"))
    guidance_config |= TEXT_GUIDANCE;
  if (strstr(argv[1], "audio"))
    guidance_config |= AUDIO_GUIDANCE;
  if (strstr(argv[1], "woz"))
    guidance_config |= WOZ_GUIDANCE;

  carmen_walker_guidance_config(guidance_config);
  carmen_walker_guidance_goto(argv[2]);

  return 0;
}
