/* Reader class (header) for hokuyo URG-04LX and UTM-30LX driver

    Aleksandr Kushleyev <akushley(at)seas(dot)upenn(dot)edu>
    University of Pennsylvania, 2008

    BSD license.
		--------------------------------------------------------------------
		Copyright (c) 2008 Aleksandr Kushleyev
		All rights reserved.

		Redistribution and use in source and binary forms, with or without
		modification, are permitted provided that the following conditions
		are met:
		1. Redistributions of source code must retain the above copyright
			notice, this list of conditions and the following disclaimer.
		2. Redistributions in binary form must reproduce the above copyright
			notice, this list of conditions and the following disclaimer in the
			documentation and/or other materials provided with the distribution.
		3. The name of the author may not be used to endorse or promote products
			derived from this software without specific prior written permission.

			THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
			IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
			OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
			IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
			INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
			NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
			DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
			THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
			(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
			THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef HOKUYO_READER_HH
#define HOKUYO_READER_HH

#include <string>
#include "Hokuyo.hh"
#include <pthread.h>

#ifndef __APPLE__
#include <signal.h>
#endif


#define HOKUYO_READER_NUM_STOP_LASER_RETRIES 3
#define HOKUYO_READER_MAX_NUM_ERRORS_BEFORE_RESTART 3
#define HOKUYO_READER_GET_SCAN_TIMEOUT 200
#define HOKUYO_READER_SET_SCAN_PARAMS_TIMEOUT 500

class HokuyoReader : public Hokuyo{

public:
  
  HokuyoReader();
  
  ~HokuyoReader();

  int Connect(const std::string dev_str, const int baud_rate);
  int Disconnect();
  
  int startHokuyoReaderThread();
  
  int GetScan(unsigned int * range, int & n_range);

  int SetScanParams(char * newScanType, int newScanStart, int newScanEnd, int newScanSkip, int newEncoding, int newSpecialScan);

  
	static void *DummyMain(void * pHokuyoReader);

  int PauseSensor();
  int ResumeSensor();

  bool _active;
  bool _connected;
  //std::string m_device;
  //int m_baud;
  //bool _scan_ready;
  bool _scan_settings_changed;
  bool _need_to_stop_laser;
  char _scan_type_name[100];

  

  

	void LockSettingsMutex();
	void UnlockSettingsMutex();
	void SettingsCondSignal();
	int SettingsCondWait(int timeout_ms);
  
  int _scan_start_new, _scan_end_new, _scan_skip_new, _encoding_new, _scan_type_new;
  
  void lockSettingsMutex();
  void unlockSettingsMutex();
  
  int HokuyoReaderThreadId;
  void Main();

  unsigned int m_data[HOKUYO_MAX_DATA_LENGTH];
  int m_numData;
  //Hokuyo * m_pHokuyo;

private:
	
	pthread_cond_t _settings_cond;
  pthread_mutex_t _settings_mutex;

	pthread_t HokuyoReaderThread;
	bool m_threadRunning;
  
};

#endif //HOKUYO_READER_HH
