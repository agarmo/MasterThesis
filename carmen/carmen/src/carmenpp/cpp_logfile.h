#ifndef CARMEN_CPP_LOGFILE_H
#define CARMEN_CPP_LOGFILE_H

#include <list>
#include <vector>
#include <carmen/cpp_global.h>
#include <carmen/cpp_abstractmessage.h>
#include <carmen/cpp_laser.h>
#include <carmen/cpp_base.h>
#include <carmen/cpp_robot.h>
#include <carmen/cpp_simulator.h>
#include <carmen/cpp_imu.h>
#include <carmen/cpp_pantilt.h>
#include <carmen/cpp_unknownmessage.h>

typedef  std::vector<AbstractMessage*> Carmen_Cpp_LogFile_Collection;

class LogFile : public Carmen_Cpp_LogFile_Collection {
 public:
  LogFile();
  LogFile(const LogFile& x);
  LogFile(const char* filename);
  virtual ~LogFile();

  bool load(const char* filename, bool verbose = true);
  bool save(const char* filename, bool verbose = true) const;

 public:
  typedef Carmen_Cpp_LogFile_Collection Collection;
};

#endif
