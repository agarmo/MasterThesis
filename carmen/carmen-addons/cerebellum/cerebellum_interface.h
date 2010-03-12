#ifndef __CEREBELLUM_INTERFACE_H__
#define __CEREBELLUM_INTERFACE_H__


typedef struct FireGunType{
  int fire;
}CerebellumFireGunMessage;
#define CEREBELLUM_FIRE_GUN_MESSAGE_SIZE (sizeof(CerebellumFireGunMessage))
#define CEREBELLUM_FIRE_GUN_MESSAGE_NAME "cerebellum_fire_gun_message"
#define CEREBELLUM_FIRE_GUN_MESSAGE_FMT "{int}"


typedef struct TiltGunType{
  int tilt_angle;
}CerebellumTiltGunMessage;
#define CEREBELLUM_TILT_GUN_MESSAGE_SIZE (sizeof(CerebellumTiltGunMessage))
#define CEREBELLUM_TILT_GUN_MESSAGE_NAME "cerebellum_tilt_gun_message"
#define CEREBELLUM_TILT_GUN_MESSAGE_FMT "{int}"

#endif
