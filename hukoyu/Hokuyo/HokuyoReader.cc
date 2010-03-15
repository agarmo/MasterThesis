/* Reader class for hokuyo URG-04LX and UTM-30LX driver

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

#include "HokuyoReader.hh"


//Constructor
HokuyoReader::HokuyoReader()
{
  _scan_ready=false;
  _scan_settings_changed=false;
  _need_to_stop_laser=false;
  _active=false;
	_connected=false;
  m_threadRunning=false;
  strcpy(_scan_type_name,"range");

	pthread_mutex_init(&_settings_mutex,NULL);
	pthread_cond_init(&_settings_cond,NULL);
}

HokuyoReader::~HokuyoReader()
{
	Disconnect();

	pthread_mutex_destroy(&_settings_mutex);
	pthread_cond_destroy(&_settings_cond);
}

int HokuyoReader::Connect(const std::string dev_str, const int baud_rate)
{

  if (((Hokuyo*)this)->Connect(dev_str.c_str(),baud_rate))
	{
    std::cerr << "HokuyoReader::Connect: Unable to initialize Hokuyo!!!" << std::endl;
    fflush(stdout);
    return -1;
  }
  _connected=true;  

  //pthread_cond_init(&m_settingsCond,NULL);

  //start the thread
  if (startHokuyoReaderThread())
	{
    std::cerr << "HokuyoReader::Connect: Unable to start the HokuyoReader thread!!!" << std::endl;
    fflush(stdout);
    return -1;
  }
  m_threadRunning=true;
  return 0;
}

int HokuyoReader::Disconnect()
{
  if (m_threadRunning)
	{
    std::cout<<"HokuyoReader::Disconnect: Stopping HokuyoReader thread..."; fflush(stdout); 
    pthread_cancel(HokuyoReaderThread);
    pthread_join(HokuyoReaderThread,NULL);
    std::cout<<" done"<<std::endl; fflush(stdout); 
    m_threadRunning=false;
  }
  
  if (_connected)
	{
		std::cout<<"HokuyoReader::Disconnect: Disconnecting from device..."; fflush(stdout); 
    ((Hokuyo*)this)->Disconnect();
		std::cout<<" done"<<std::endl; fflush(stdout); 
    _connected=false;
  }
  return 0;
}


int HokuyoReader::startHokuyoReaderThread()
{
  std::cout<<"Starting HokuyoReader Thread...";

  if (pthread_create(&HokuyoReaderThread, NULL, this->DummyMain, (void *)this)) 
	{
    std::cerr<<"HokuyoReader:startHokuyoReaderThread: Could not create HokuyoReader thread"<<std::endl;
    return -1;
  }
  std::cout<<"done"<<std::endl;
  return 0;
}


void *HokuyoReader::DummyMain(void * pHokuyoReader)
{
	((HokuyoReader*)pHokuyoReader)->Main();
	return NULL;
}

void HokuyoReader::Main()
{
  sigset_t sigs;
	sigfillset(&sigs);
	pthread_sigmask(SIG_BLOCK,&sigs,NULL);
  timeval timeStart,timeStop,totalTimeStart,totalTimeStop;
  unsigned int cntr=0;

  int scan_start, scan_end, scan_skip, encoding, scan_type;
  bool connected, active;
  
  int numErrors=0;
  
  gettimeofday(&totalTimeStart,NULL);
  gettimeofday(&totalTimeStop,NULL);

  while(1)
	{
    cntr++;
    gettimeofday(&timeStart,NULL);
    pthread_testcancel();
    
    LockSettingsMutex();

		//create a local copy of the scan params to be used later
    scan_start   = _scan_start_new;
    scan_end     = _scan_end_new;
    scan_skip    = _scan_skip_new;
    encoding     = _encoding_new;
    connected    = _connected;
    active       = _active;
    scan_type    = _scan_type_new;

		//double check the scan settings
		if ( (_scan_start_new != _scan_start) || (_scan_end_new != _scan_end) || 
				 (_scan_skip_new != _scan_skip) || (_encoding_new != _encoding) ||
				 (_scan_type_new != _scan_type)  )
		{
			_scan_settings_changed=true;
			//FIXME: need to uncomments this probably
			if (_streaming)
			{			
				_need_to_stop_laser=true;
			}
		}

		//if the scan settings changed, we dont want to use the previous scan,
		//since it will contain data from old sensor params
    if (_scan_settings_changed)
		{
      LockDataMutex();      
      _scan_ready=false;
      UnlockDataMutex();;
    }

		
    if (_need_to_stop_laser)
		{
      if (LaserOff()==0)
			{
        std::cout<<"HokuyoReader::HokuyoReaderThreadFunc: Laser has been shut off"<<std::endl;
        _need_to_stop_laser=false;
        //usleep(100000);
      }
      
      else 
			{
        std::cerr<<"HokuyoReader::HokuyoReaderThreadFunc: ERROR: Wasn't able to shut off the laser"<<std::endl;
      }
      
      //FIXME: count number of times errors occur
    }
		
    _scan_settings_changed=false;
		SettingsCondSignal();
    UnlockSettingsMutex();
    
    //std::cerr<<"reader: settings locked"<<std::endl;
    if (connected && active)
		{
      if (!((Hokuyo*)this)->GetScan(m_data, m_numData, scan_start, scan_end, scan_skip, encoding,scan_type, 0))
			{
        numErrors=0;
      } 

			else 
			{
        numErrors++;
        std::cout << "HokuyoReaderThreadFunc: Could not read a scan from the sensor" <<std::endl; fflush(stdout);
        //usleep(100000);
        if (!FindPacketStart())
				{
          std::cout << "HokuyoReaderThreadFunc: Resynchronized with the stream." <<std::endl; fflush(stdout);
        }
        
        if (numErrors>=HOKUYO_READER_MAX_NUM_ERRORS_BEFORE_RESTART)
				{
          _need_to_stop_laser=true;
          numErrors=0;
        }
        //FIXME: need to do some error handling here
        //pthread_exit(NULL);
      }
    }
		
		else 
		{
      //just idle
      usleep(100000);
    }

    gettimeofday(&timeStop,NULL);
    //double loopTime=(timeStop.tv_sec+timeStop.tv_usec*0.000001)-(timeStart.tv_sec+timeStart.tv_usec*0.000001);    
    //std::cerr << loopTime << std::endl; fflush(stdout);

		//calculate the scan rate
    if (active)
		{
      if (_type==HOKUYO_TYPE_URG_04LX)
			{
        if (cntr%25==0)
				{
          gettimeofday(&totalTimeStop,NULL);        
          double totalTime=(totalTimeStop.tv_sec+totalTimeStop.tv_usec*0.000001) - 
													 (totalTimeStart.tv_sec+totalTimeStart.tv_usec*0.000001);
          std::cout << "Hokuyo URG-04LX rate is "<<25/totalTime<<std::endl;
          gettimeofday(&totalTimeStart,NULL);
        }
      }
      else if (_type==HOKUYO_TYPE_UTM_30LX)
			{
        if (cntr%100==0)
				{
          gettimeofday(&totalTimeStop,NULL);        
          double totalTime=(totalTimeStop.tv_sec+totalTimeStop.tv_usec*0.000001) -
													 (totalTimeStart.tv_sec+totalTimeStart.tv_usec*0.000001);
          std::cout<< "Hokuyo UTM-30LX rate is "<<100/totalTime<<std::endl;
          //printf("Hokuyo UTM-30LX rate is %3.2f\n",40/totalTime);
          gettimeofday(&totalTimeStart,NULL);
        }
      }
      fflush(stdout);
      fflush(stderr);
    }
  }
}

void HokuyoReader::LockSettingsMutex()
{
	pthread_mutex_lock(&_settings_mutex);
}

void HokuyoReader::UnlockSettingsMutex()
{
	pthread_mutex_unlock(&_settings_mutex);
}

void HokuyoReader::SettingsCondSignal()
{
	pthread_cond_signal(&_settings_cond);
}

int HokuyoReader::SettingsCondWait(int timeout_ms)
{ 
	timeval time_start;
  timespec max_wait_time;
  gettimeofday(&time_start,NULL);
  
  max_wait_time.tv_sec=time_start.tv_sec + ((time_start.tv_usec < 1000000-timeout_ms*1000) ? 0:1);
  max_wait_time.tv_nsec=(time_start.tv_usec*1000 + timeout_ms*1000000)%1000000000;
	return pthread_cond_timedwait(&_settings_cond,&(_settings_mutex),&max_wait_time);
}

int HokuyoReader::PauseSensor()
{
  std::cerr<<"ResumeSensor: entered SetScanParams... waiting to lock the mutex"<<std::endl;
  LockSettingsMutex();
  std::cerr<<"ResumeSensor: settings locked"<<std::endl;
  
  _active=false;
  _need_to_stop_laser=true;
  
  UnlockSettingsMutex();
  std::cerr<<"ResumeSensor: settings unlocked"<<std::endl;
  return 0;
}

int HokuyoReader::ResumeSensor()
{
  std::cerr<<"ResumeSensor: entered SetScanParams... waiting to lock the mutex"<<std::endl;
  LockSettingsMutex();
  std::cerr<<"ResumeSensor: settings locked"<<std::endl;
  
  _active=true;
  
  UnlockSettingsMutex();
  std::cerr<<"ResumeSensor: settings unlocked"<<std::endl;
  return 0;
}

int HokuyoReader::SetScanParams(char * scan_type_new_name,int scan_start_new, 
																int scan_end_new, int scan_skip_new, 
																int encoding_new, int scan_type_new)
{
  std::cerr<<"HokuyoReader::SetScanParams: entered SetScanParams... waiting to lock the mutex"<<std::endl;
  LockSettingsMutex();
  std::cerr<<"HokuyoReader::SetScanParams: settings locked"<<std::endl;
  
  //copy the new values
  _scan_start_new=scan_start_new;
  _scan_end_new=scan_end_new;
  _scan_skip_new=scan_skip_new;
  _encoding_new=encoding_new;
  _scan_type_new=scan_type_new;
  _need_to_stop_laser=true;
  strncpy(_scan_type_name,scan_type_new_name,100);

  _active=true;
  _scan_settings_changed=true;

  timeval timeStart,timeStop;
  gettimeofday(&timeStart,NULL);
  gettimeofday(&timeStop,NULL);
  double waitedTime=(timeStop.tv_sec+timeStop.tv_usec*0.000001)-(timeStart.tv_sec+timeStart.tv_usec*0.000001);

  std::cerr <<"HokuyoReader::SetScanParams: set the settings.. waiting for the next scan read before returning" <<std::endl;

  //this loop makes sure that timedwait does not return before the time expires - recommended in manual for this function..
  while ( (waitedTime*1000 < HOKUYO_READER_SET_SCAN_PARAMS_TIMEOUT) && (_scan_settings_changed) )
	{         
		//int ret=SettingsCondWait(&maxWaitTime);    
		int ret=SettingsCondWait(HOKUYO_READER_SET_SCAN_PARAMS_TIMEOUT);
		if (ret==ETIMEDOUT)
		{
      std::cerr <<"HokuyoReader::SetScanParams: timeout!!!!"<<std::endl;
    }

    gettimeofday(&timeStop,NULL);
    waitedTime=(timeStop.tv_sec+timeStop.tv_usec*0.000001)-(timeStart.tv_sec+timeStart.tv_usec*0.000001);
    if ( (waitedTime*1000 < HOKUYO_READER_SET_SCAN_PARAMS_TIMEOUT) && (_scan_settings_changed) )
		{      
      usleep(1000);
    }
  }
  
	//need to make sure that the settings changed
  if (_scan_settings_changed)
	{
    std::cerr << "HokuyoReader::SetScanParams: could not set the parameters!! Waited time="<<waitedTime << std::endl;
    UnlockSettingsMutex();
    return -1;
  }

  UnlockSettingsMutex();
  std::cerr<<"HokuyoReader::SetScanParams: waited time="<< waitedTime <<std::endl;
  std::cerr<<"HokuyoReader::SetScanParams: settings unlocked"<<std::endl;
  
  return 0;
}

int HokuyoReader::GetScan(unsigned int * range, int & n_range)
{
  timeval timeStart,timeStop;
  gettimeofday(&timeStart,NULL);
	gettimeofday(&timeStop,NULL);
  double waitedTime=(timeStop.tv_sec+timeStop.tv_usec*0.000001)-(timeStart.tv_sec+timeStart.tv_usec*0.000001);

  LockDataMutex();
  while ( (waitedTime*1000 < HOKUYO_READER_GET_SCAN_TIMEOUT) && (!_scan_ready) )
	{         
		//int ret=DataCondWait(&maxWaitTime);    
		int ret=DataCondWait(HOKUYO_READER_GET_SCAN_TIMEOUT);
		if (ret==ETIMEDOUT)
		{
      std::cerr <<"HokuyoReader::GetScan: timeout!!!!"<<std::endl;
    }

    gettimeofday(&timeStop,NULL);
    waitedTime=(timeStop.tv_sec+timeStop.tv_usec*0.000001)-(timeStart.tv_sec+timeStart.tv_usec*0.000001);
    if ( (waitedTime*1000 < HOKUYO_READER_GET_SCAN_TIMEOUT) && (!_scan_ready) )
		{      
      usleep(1000);
    }
  }

	//need to verify that the scan is actually ready
  if (_scan_ready)
	{
    //new scan is ready!
    memcpy(range,m_data,m_numData*sizeof(unsigned int));
    n_range=m_numData;
    _scan_ready=false;
      
    UnlockDataMutex();
    gettimeofday(&timeStop,NULL);
    //double loopTime=(timeStop.tv_sec+timeStop.tv_usec*0.000001)-(timeStart.tv_sec+timeStart.tv_usec*0.000001);
    //std::cout <<"GetScan time="<< loopTime << std::endl; fflush(stdout);      
    return 0;
  } 
	else 
	{
    //scan is not ready
    UnlockDataMutex();
    gettimeofday(&timeStop,NULL);
    double loopTime=(timeStop.tv_sec+timeStop.tv_usec*0.000001)-(timeStart.tv_sec+timeStart.tv_usec*0.000001);
    std::cout <<"GetScan time (failed)="<< loopTime << std::endl; fflush(stdout);
    std::cerr << "HokuyoReader::getScan: Error: unable to get a scan from the sensor" << std::endl;
    return -1;  
  }
}

