#ifndef CARMEN_CPP_PANTILT_MESSAGE_H
#define CARMEN_CPP_PANTILT_MESSAGE_H

#include <carmen/cpp_global.h>
#include <carmen/cpp_abstractmessage.h>


class ScanmarkMessage : public AbstractMessage {

 public:
  ScanmarkMessage();
  virtual ~ScanmarkMessage();
  ScanmarkMessage(const ScanmarkMessage& x);
  ScanmarkMessage(const carmen_pantilt_scanmark_message& x);
  ScanmarkMessage(carmen_pantilt_scanmark_message* x);
  ScanmarkMessage(char* s);
  
  void init();
  void clone(const ScanmarkMessage& x);
  void clone(const carmen_pantilt_scanmark_message& x);
  void setScanmarkMessage(carmen_pantilt_scanmark_message* x);
  void free();

  static const int scanmark_start = 0;
  static const int scanmark_stop  = 1;

  // accessors

  bool isStart();
  
  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, int, Type, type);
  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, int, LaserID, laserid);

  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, char*, Host, host);
  STRUCT_PARAM_VIRTUAL_SET_GET(*m_msg, double, Timestamp, timestamp, public, public);

  carmen_inline carmen_pantilt_scanmark_message* toCarmenMsg() {return m_msg;}
  carmen_inline operator carmen_pantilt_scanmark_message*() {return m_msg;}
  carmen_inline operator const carmen_pantilt_scanmark_message&() const {return *m_msg;}
  carmen_inline operator carmen_pantilt_scanmark_message&() {return *m_msg;}

  virtual const char* getMessageID() const {
    return "SCANMARK";
  };

  virtual void save(carmen_FILE *logfile, double logger_timestamp);
  virtual char*  fromString(char* s);
  virtual AbstractMessage* clone() const;

 protected:
  carmen_pantilt_scanmark_message * m_msg;
};


// ---------------------------------------------------------
// ---------------------------------------------------------

class LaserposMessage : public AbstractMessage {

 public:
  LaserposMessage();
  virtual ~LaserposMessage();
  LaserposMessage(const LaserposMessage& x);
  LaserposMessage(const carmen_pantilt_laserpos_message& x);
  LaserposMessage(carmen_pantilt_laserpos_message* x);
  LaserposMessage(char* s);
  
  void init();
  void clone(const LaserposMessage& x);
  void clone(const carmen_pantilt_laserpos_message& x);
  void setLaserposMessage(carmen_pantilt_laserpos_message* x);
  void free();


  // accessors

  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, int, LaserID, id);

  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, double, X, x);
  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, double, Y, y);
  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, double, Z, z);

  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, double, Phi, phi);
  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, double, Theta, theta);
  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, double, Psi, psi);

  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, double, Roll, phi);
  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, double, Pitch, theta);
  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, double, Yaw, psi);

  DEFAULT_STRUCT_PARAM_SET_GET(*m_msg, char*, Host, host);
  STRUCT_PARAM_VIRTUAL_SET_GET(*m_msg, double, Timestamp, timestamp, public, public);

  carmen_inline carmen_pantilt_laserpos_message* toCarmenMsg() {return m_msg;}
  carmen_inline operator carmen_pantilt_laserpos_message*() {return m_msg;}
  carmen_inline operator const carmen_pantilt_laserpos_message&() const {return *m_msg;}
  carmen_inline operator carmen_pantilt_laserpos_message&() {return *m_msg;}

  virtual const char* getMessageID() const {
    return "LASERPOS";
  };

  virtual void save(carmen_FILE *logfile, double logger_timestamp);
  virtual char*  fromString(char* s);
  virtual AbstractMessage* clone() const;

 protected:
  carmen_pantilt_laserpos_message * m_msg;
};


#endif 
