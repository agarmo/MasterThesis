#include "cpp_pantilt.h"


// - constructors  ---------------------------------------

ScanmarkMessage::ScanmarkMessage()
  : AbstractMessage() {
  m_msg = NULL;
  init();
}

ScanmarkMessage::ScanmarkMessage(const ScanmarkMessage& x) 
  : AbstractMessage(x) {
  m_msg = NULL;
  clone(x);
}

ScanmarkMessage::ScanmarkMessage(const carmen_pantilt_scanmark_message& x) {
  m_msg = NULL;
  clone(x);
}

ScanmarkMessage::ScanmarkMessage(carmen_pantilt_scanmark_message* x) {
  m_msg = NULL;
  setScanmarkMessage(x);
}

ScanmarkMessage::~ScanmarkMessage() {
  this->free();
}


// - accessors  --------------------------------------------

bool ScanmarkMessage::isStart()  {
  return (m_msg->type == ScanmarkMessage::scanmark_start); 
}



// - I/O  --------------------------------------------------

void ScanmarkMessage::save(carmen_FILE *logfile, double logger_timestamp) {
  carmen_logwrite_write_pantilt_scanmark(m_msg, logfile, logger_timestamp);
}

char* ScanmarkMessage::fromString(char* s) {
  if (m_msg == NULL) {
    m_msg = new carmen_pantilt_scanmark_message;
    carmen_erase_structure(m_msg, sizeof(carmen_pantilt_scanmark_message));
  }
  return carmen_string_to_pantilt_scanmark_message(s, m_msg);
}

ScanmarkMessage::ScanmarkMessage(char* s) {
  m_msg = NULL;
  fromString(s);
}


// - memory access  ----------------------------------------

AbstractMessage* ScanmarkMessage::clone() const {
  return new ScanmarkMessage(*this);
}

void ScanmarkMessage::init() {

  if (m_msg != NULL) {
    this->free();
  }
  m_msg = new carmen_pantilt_scanmark_message;
  carmen_test_alloc(m_msg);
  carmen_erase_structure(m_msg, sizeof(carmen_pantilt_scanmark_message));
}

void ScanmarkMessage::free() {
  if (m_msg != NULL) {
    delete m_msg;
    m_msg = NULL;
  }
}

void ScanmarkMessage::setScanmarkMessage( carmen_pantilt_scanmark_message* x) {
  m_msg = x;
}


void ScanmarkMessage::clone(const ScanmarkMessage& x) {
  clone(*(x.m_msg));
}

void ScanmarkMessage::clone(const carmen_pantilt_scanmark_message& x) {
  m_msg->type = x.type;
  m_msg->laserid = x.laserid;

  m_msg->timestamp = x.timestamp;
  m_msg->host = x.host;
}



// #############################################################
// #############################################################
// #############################################################




// - constructors  ---------------------------------------

LaserposMessage::LaserposMessage()
  : AbstractMessage() {
  m_msg = NULL;
  init();
}

LaserposMessage::LaserposMessage(const LaserposMessage& x) 
  : AbstractMessage(x) {
  m_msg = NULL;
  clone(x);
}

LaserposMessage::LaserposMessage(const carmen_pantilt_laserpos_message& x) {
  m_msg = NULL;
  clone(x);
}

LaserposMessage::LaserposMessage(carmen_pantilt_laserpos_message* x) {
  m_msg = NULL;
  setLaserposMessage(x);
}

LaserposMessage::~LaserposMessage() {
  this->free();
}


// - accessors  --------------------------------------------




// - I/O  --------------------------------------------------

void LaserposMessage::save(carmen_FILE *logfile, double logger_timestamp) {
  carmen_logwrite_write_pantilt_laserpos(m_msg, logfile, logger_timestamp);
}

char* LaserposMessage::fromString(char* s) {
  if (m_msg == NULL) {
    m_msg = new carmen_pantilt_laserpos_message;
    carmen_erase_structure(m_msg, sizeof(carmen_pantilt_laserpos_message));
  }
  return carmen_string_to_pantilt_laserpos_message(s, m_msg);
}

LaserposMessage::LaserposMessage(char* s) {
  m_msg = NULL;
  fromString(s);
}


// - memory access  ----------------------------------------

AbstractMessage* LaserposMessage::clone() const {
  return new LaserposMessage(*this);
}

void LaserposMessage::init() {

  if (m_msg != NULL) {
    this->free();
  }
  m_msg = new carmen_pantilt_laserpos_message;
  carmen_test_alloc(m_msg);
  carmen_erase_structure(m_msg, sizeof(carmen_pantilt_laserpos_message));
}

void LaserposMessage::free() {
  if (m_msg != NULL) {
    delete m_msg;
    m_msg = NULL;
  }
}

void LaserposMessage::setLaserposMessage( carmen_pantilt_laserpos_message* x) {
  m_msg = x;
}


void LaserposMessage::clone(const LaserposMessage& x) {
  clone(*(x.m_msg));
}

void LaserposMessage::clone(const carmen_pantilt_laserpos_message& other) {

  m_msg->id = other.id;
  m_msg->x = other.x;
  m_msg->y = other.y;
  m_msg->z = other.z;
  m_msg->phi = other.phi;
  m_msg->theta = other.theta;
  m_msg->psi = other.psi;

  m_msg->timestamp = other.timestamp;
  m_msg->host = other.host;
}


