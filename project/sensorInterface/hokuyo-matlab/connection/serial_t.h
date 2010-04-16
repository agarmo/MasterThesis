#ifndef QRK_C_SERIAL_T_H
#define QRK_C_SERIAL_T_H


/*!
  \file
  \brief

  \author Satofumi KAMIMURA

  $Id: serial_t_win.h 1559 2009-12-01 13:13:08Z satofumi $
*/

#include "ring_buffer.h"
#include <windows.h>


enum {
  SerialErrorStringSize = 256,
  RingBufferSizeShift = 10,
  RingBufferSize = 1 << RingBufferSizeShift,
};


/*!
  \brief
*/
typedef struct {
  int errno_;
  HANDLE hCom_;
  int current_timeout_;
  ringBuffer_t ring_;
  char buffer_[RingBufferSize];
  char has_last_ch_;
  char last_ch_;

} serial_t;


#endif /* !QRK_C_SERIAL_T_H */
