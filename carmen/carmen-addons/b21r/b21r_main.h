#ifndef CARMEN_B21R_MAIN_H
#define CARMEN_B21R_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

int carmen_b21r_start(int argc, char **argv);
void carmen_b21r_usage(char *progname, char *fmt, ...);
void carmen_b21r_add_ant_callbacks(void);
int carmen_b21r_run(void);
void carmen_b21r_shutdown(int x);
void carmen_b21r_emergency_crash(int x);

#ifdef __cplusplus
}
#endif

#endif
