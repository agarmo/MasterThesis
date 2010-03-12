#include "cpp_base.h"
#include <carmen/carmen.h>

OdometryMessage::OdometryMessage()
  : AbstractMessage() {
  m_msg = NULL;
  init();
}

OdometryMessage::OdometryMessage(const OrientedPoint& robotPose, double tv, double rv, double acc) 
  : AbstractMessage() {
  m_msg = NULL;
  init();
  m_msg->x = robotPose.x;
  m_msg->y = robotPose.y;
  m_msg->theta = robotPose.theta;
  m_msg->tv = tv;
  m_msg->rv = rv;
  m_msg->acceleration = acc;
}

OdometryMessage::OdometryMessage(const OdometryMessage& x) 
  : AbstractMessage(x) {
  m_msg = NULL;
  clone(x);
}

OdometryMessage::OdometryMessage(const carmen_base_odometry_message& x) {
  m_msg = NULL;
  clone(x);
}

OdometryMessage::OdometryMessage(carmen_base_odometry_message* x) {
  m_msg = NULL;
  setOdometryMessage(x);
}

OdometryMessage::~OdometryMessage() {
  this->free();
}

OdometryMessage& OdometryMessage::operator=(const OdometryMessage &x) {
  clone(x);
  return *this;
}

OdometryMessage& OdometryMessage::operator=(const carmen_base_odometry_message &x) {
  clone(x);
  return *this;
}

void OdometryMessage::save(carmen_FILE *logfile, double logger_timestamp) {
  carmen_logwrite_write_odometry(m_msg, logfile, logger_timestamp);
}

OdometryMessage::OdometryMessage(char* s) {
  m_msg = NULL;
  fromString(s);
}

char* OdometryMessage::fromString(char* s) {
  if (m_msg == NULL) {
    init();
  }
  return carmen_string_to_base_odometry_message(s, m_msg);
}

AbstractMessage* OdometryMessage::clone() const {
  return new OdometryMessage(*this);
}


void OdometryMessage::init() {
  free();
  m_msg = new carmen_base_odometry_message;
  carmen_test_alloc(m_msg);
  carmen_erase_structure(m_msg, sizeof(carmen_base_odometry_message));
}

void OdometryMessage::free() {
  if (m_msg != NULL) {
    delete m_msg->host;
    delete m_msg;
    m_msg = NULL;
  }
}

void OdometryMessage::setOdometryMessage( carmen_base_odometry_message* x) {
  m_msg = x;
}


void OdometryMessage::clone(const OdometryMessage& x) {
  clone(*(x.m_msg));
}

void OdometryMessage::clone(const carmen_base_odometry_message& x) {
  init();
  m_msg->x = x.x;
  m_msg->y = x.y;
  m_msg->theta = x.theta;
  m_msg->tv = x.tv;
  m_msg->rv = x.rv;
  m_msg->acceleration = x.acceleration;
  m_msg->timestamp = x.timestamp;
  if (x.host)
    m_msg->host = strdup(x.host);
  else
    m_msg->host = NULL;
}

OrientedPoint OdometryMessage::getRobotPose() const {
  return OrientedPoint(m_msg->x,m_msg->y,m_msg->theta);
}

void OdometryMessage::setRobotPose(const OrientedPoint& robotPose ) {
  m_msg->x = robotPose.x;
  m_msg->y = robotPose.y;
  m_msg->theta = robotPose.theta;
}
