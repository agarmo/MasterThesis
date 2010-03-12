#ifndef CARMEN_CPP_BASE_MESSAGE_H
#define CARMEN_CPP_BASE_MESSAGE_H

#include <carmen/cpp_global.h>
#include <carmen/cpp_abstractmessage.h>
#include <carmen/cpp_point.h>


class OdometryMessage : public AbstractMessage {
 public:
  OdometryMessage();
  OdometryMessage(const OrientedPoint& robotPose, double vTrans=0.0, double vRot=0.0, double acc=0.0);
  OdometryMessage(const OdometryMessage& x);
  OdometryMessage(const carmen_base_odometry_message& x);
  //! attention: this constructor does NOT copy x,
  //! but it is deleted when this OdometryMessage is deleted
  OdometryMessage(carmen_base_odometry_message* x);
  OdometryMessage(char* s);
  virtual ~OdometryMessage();

  OdometryMessage& operator=(const OdometryMessage &x);
  OdometryMessage& operator=(const carmen_base_odometry_message &x);

  void init();
  void clone(const OdometryMessage& x);
  void clone(const carmen_base_odometry_message& x);
  //! attention: this function does NOT copy x
  //! but it is deleted when this OdometryMessage is deleted
  void setOdometryMessage(carmen_base_odometry_message* x);
  void free();

  OrientedPoint getRobotPose() const;
  void setRobotPose(const OrientedPoint& robotPose );

  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, double, X, x);
  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, double, Y, y);
  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, double, Theta, theta);
  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, double, TV, tv);
  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, double, RV, rv);
  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, double, Acceleration, acceleration);
  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, char*, Host, host);
  STRUCT_PARAM_VIRTUAL_SET_GET(*m_msg, double, Timestamp, timestamp, public, public);

  carmen_inline carmen_base_odometry_message* toCarmenMsg() {return m_msg;}
  carmen_inline operator carmen_base_odometry_message*() {return m_msg;}
  carmen_inline operator const carmen_base_odometry_message&() const {return *m_msg;}
  carmen_inline operator carmen_base_odometry_message&() {return *m_msg;}

  virtual const char* getMessageID() const {
    return "ODOM";
  };

  virtual void save(carmen_FILE *logfile, double logger_timestamp);
  virtual char*  fromString(char* s);
  virtual AbstractMessage* clone() const;

 protected:
  carmen_base_odometry_message * m_msg;
};

#endif 
