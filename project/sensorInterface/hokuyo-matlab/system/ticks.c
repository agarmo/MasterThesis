/*!
  \file
  \brief タイムスタンプ取得関数

  \author Satofumi KAMIMURA

  $Id: ticks.c 1305 2009-09-16 05:40:53Z satofumi $
*/

#include "ticks.h"
#include "detect_os.h"
#if defined WINDOWS_OS
#include <time.h>
#else
#include <sys/time.h>
#include <stdio.h>
#endif


int ticks(void)
{
  int current_ticks = 0;

#if defined(LINUX_OS)
  // Linux で SDL がない場合の実装。最初の呼び出しは 0 を返す
  static int first_ticks = 0;
  struct timeval tvp;
  gettimeofday(&tvp, NULL);
  int global_ticks = tvp.tv_sec * 1000 + tvp.tv_usec / 1000;
  if (first_ticks == 0) {
    first_ticks = global_ticks;
  }
  current_ticks = global_ticks - first_ticks;

#else
  current_ticks = (int)(clock() / (CLOCKS_PER_SEC / 1000.0));
#endif
  return current_ticks;
}
