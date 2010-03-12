// Mobility(tm) Include files.
#include <carmen/carmen.h>

#include <mobilitycomponents_i.h>
#include <mobilitydata_i.h>
#include <mobilityactuator_i.h>
#include <mobilitygeometry_i.h>
#include <tmath.h>
#include <mobilityutil.h>

#include "b21r_main.h"


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

typedef struct {
  float source;
  float regulator;
} BatteryStatusType;

typedef struct {
  float x;
  float y;
  float o;
  float tvel;
  float rvel;
} RobotOdoStatusType;

// Global state access for keyboard interface routine.
MobilityActuator::ActuatorState *m_pdriveCommand;

/************************************************************************/
/* COMPATIBILITY BLOCK : Functions, handlers, and global constants and
** variables needed for the BeeSoft backwards compatibility handling.
*/
/************************************************************************/


// Configuring define
#define MAX_NUM_SENSOR_DATA 64

// Constants for user interface clamping of values.
#define MAX_TRANSLATIONAL_VELOCITY          130.0
#define MAX_ROTATIONAL_VELOCITY             360.0

// Global variables for keyboard interface handling.

static carmen_base_odometry_message odometry;
static double last_published_update = 0;
static int use_sonar = 0;
static carmen_base_sonar_message sonar;


static int exitP = FALSE;

static int recieveOdo                 = FALSE;
static int recieveRawOdo              = FALSE;
static int recieveSonar               = FALSE;
static int recieveBaseSonar           = FALSE;
static int recieveBaseContact         = FALSE;
static int recieveEnclosureContact    = FALSE;
static int recievePower               = FALSE;

static struct timeval now = {0,0};
static struct timeval lastUpdate = {0,0};

// Global variables used to communicate motions to/from BeeSoft library

static double command_tvel = 0;
static double command_rvel = 0;
static int set_velocity = 0;

static void
velocity_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
		 void *clientData)
{
  IPC_RETURN_TYPE err;
  carmen_base_velocity_message vel;

  FORMATTER_PTR formatter;
  
  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &vel,
			   sizeof(carmen_base_velocity_message));
  IPC_freeByteArray(callData);

  clientData = clientData;

  carmen_test_ipc_return(err, "Could not unmarshall", 
			 IPC_msgInstanceName(msgRef));

  command_tvel = vel.tv;
  command_rvel = vel.rv;

  gettimeofday(&lastUpdate, NULL);

  set_velocity = 1;
}

int 
carmen_b21r_initialize_ipc(void)
{
  IPC_RETURN_TYPE err;

  /* define messages created by b21r */
  err = IPC_defineMsg(CARMEN_BASE_ODOMETRY_NAME, IPC_VARIABLE_LENGTH,
                      CARMEN_BASE_ODOMETRY_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_BASE_ODOMETRY_NAME);

  err = IPC_defineMsg(CARMEN_BASE_SONAR_NAME, IPC_VARIABLE_LENGTH,
                      CARMEN_BASE_SONAR_FMT);
  carmen_test_ipc_exit(err, "Could not define IPC message", CARMEN_BASE_SONAR_NAME);
  err = IPC_defineMsg(CARMEN_BASE_VELOCITY_NAME, IPC_VARIABLE_LENGTH,
                      CARMEN_BASE_VELOCITY_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_BASE_VELOCITY_NAME);

  /* setup incoming message handlers */

  err = IPC_subscribe(CARMEN_BASE_VELOCITY_NAME, velocity_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subscribe", CARMEN_BASE_VELOCITY_NAME);

  return IPC_No_Error;
}

static int
read_b21r_parameters(int argc, char **argv)
{
  argc = argc;
  argv = argv;
	
  return 0;
}

int
carmen_b21r_start(int argc, char **argv)
{
  if (read_b21r_parameters(argc, argv) < 0)
    return -1;

  if (carmen_b21r_initialize_ipc() < 0)
    {
      carmen_warn("\nError: Could not initialize IPC.\n");
      return -1;
    }

  /*
    if (initialize_b21r() < 0)
    {
    carmen_warn("\nError: Could not connect to robot. Did you remember to "
    "start b21r services?\n");
    return -1;
    }
  */

  /*
    #ifdef _REENTRANT
    pthread_create(&main_thread, NULL, start_thread, NULL);
    thread_is_running = 1;
    #endif
  */
  return 0;
}

int 
carmen_b21r_run(void) 
{
  IPC_RETURN_TYPE err;

  if (last_published_update < odometry.timestamp)
    {
      err = IPC_publishData(CARMEN_BASE_ODOMETRY_NAME, &odometry);
      carmen_test_ipc_exit(err, "Could not publish", 
			   CARMEN_BASE_ODOMETRY_NAME);
      
      if (use_sonar)
	{
	  err = IPC_publishData(CARMEN_BASE_SONAR_NAME, &sonar);
	  carmen_test_ipc_exit(err, "Could not publish", 
			       CARMEN_BASE_SONAR_NAME);
	}
      last_published_update = carmen_get_time_ms();
    }
  // #ifndef _REENTRANT
  //  command_robot();
  // #endif
  
  return 1;
}

void 
send_sensor_update( int num_eSonar,  float *eSonarValues,
		    int num_bSonar,  float *bSonarValues,
		    int num_eBumper, float *eBumperValues,
		    int num_bBumper, float *bBumperValues )
{
  num_eSonar = num_eSonar;
  eSonarValues = eSonarValues;
  num_bSonar = num_bSonar;
  bSonarValues = bSonarValues;

  num_eBumper = num_eBumper;
  eBumperValues = eBumperValues;
  num_bBumper = num_bBumper;
  bBumperValues = bBumperValues;
}

/************************************************************************/
/* MOBILITY SERVER : Top level classes and handler classes for the
** Mobility(tm) interface of this server.
*/
/************************************************************************/

class ProxyModule : public SystemModule_i
{
public:
  ProxyModule(const char *name, const char *source):
    SystemModule_i(name) 
  { 
    fprintf(stderr,"ProxyModule::Module\n");
    strcpy(m_source, source);
    create_string_property("BaseType","Proxy1", "Type of module.");
    m_total_calls = 0;
  }
  
  // Override base class methods to print progress
  
  /**
     This template function is called by init_module to
     allow derived classes to perform module specific setup.
  */
  virtual int on_init_module() {
    fprintf(stderr,"ProxyModule::on_init_module\n");
    return 0;
  }

  /**
     This template function is called by activate_module to allow
     derived classes to perform module specific setup.
  */
  virtual int on_setup_module();

  /**
     This template function is called be cleanup_module to allow
     derived classes to perform module specific cleanup.
  */
  virtual int on_cleanup_module() { 
    fprintf(stderr,"ProxyModule::on_cleanup_module\n");
    return 0;
  };

  /**
     This template function is called when a module is shutdown
     to allow derived classes to perform module specific cleanup.
  */
  virtual int on_shutdown_module() { 
    fprintf(stderr,"ProxyModule::on_shutdown_module\n");
    return 0; };

  virtual CORBA::Long initialize_object (
					 const MobilityBase::NTVSeq & params)
  {
    fprintf(stderr,"ProxyModule::initialize_object\n");
    return SystemModule_i::initialize_object(params);
  }

  /**
     Event handling function.
  */
  virtual CORBA::Long on_general_notify (
					 const char * event,
					 const MobilityBase::NTVSeq & params,
					 CORBA::Object_ptr source,
					 CORBA::Boolean up);

protected:
  
  // State objects reflected from base server.
  ActuatorState_i                    *m_pOdo;
  ActuatorState_i                    *m_pRawOdo;
  FVectorState_i                     *m_pSonar;
  FVectorState_i                     *m_pBaseSonar;
  FVectorState_i                     *m_pBaseContact;
  FVectorState_i                     *m_pEnclosureContact;
  PowerManagementState_i             *m_pPower;
  
  StringState_i *m_pDebug;

  // Active watchdog component.
  PeriodicActivity_i                 *m_pTimer;
  // Special "zero" watchdog command data.
  MobilityActuator::ActuatorData      m_wdCommandData;

  // Variables for command output interface for wheels.
  MobilityActuator::ActuatorState_var m_pCommand;
  MobilityActuator::ActuatorData      m_curCommandData;

  // ODO
  MobilityActuator::ActuatorData      m_curOdoData;
  MobilityActuator::ActuatorData      m_curRawOdoData;

  // SONAR
  MobilityData::FVectorData           m_curSonarData;
  MobilityData::FVectorData           m_curBaseSonarData;

  // BUMPER
  MobilityData::FVectorData           m_curBaseContactData;
  MobilityData::FVectorData           m_curEnclosureContactData;

  // POWER
  MobilityData::PowerManagementStatus m_curPowerData;

  // TIME
  MobilityBase::TimeStampData         m_LastCommandUpdate;

  char                                m_source[256];
  int                                 m_total_calls;     
};

static ProxyModule *pProxyModule;

int
ProxyModule::on_setup_module() {

  MobilityBase::Registration keys;
  MobilityBase::NTVSeq params;
  MobilityBase::StringSeq_var pathname;
  char pathnamebuf[255];
  
  fprintf(stderr,"ProxyModule::on_setup_module\n");

  // Odometry state component.
  m_pOdo = new ActuatorState_i("Odo");
  m_pOdo->_obj_is_ready(m_BOA);
  insert_object(m_pOdo->_this(), "owner", keys);

  // Raw odometry state component.
  m_pRawOdo = new ActuatorState_i("RawOdo");
  m_pRawOdo->_obj_is_ready(m_BOA);
  insert_object(m_pRawOdo->_this(), "owner", keys);

  // Raw sonar state component.
  m_pSonar = new FVectorState_i("Sonar");
  m_pSonar->_obj_is_ready(m_BOA);
  insert_object(m_pSonar->_this(), "owner", keys);

  // Raw base sonar state component.
  m_pBaseSonar = new FVectorState_i("BaseSonar");
  m_pBaseSonar->_obj_is_ready(m_BOA);
  insert_object(m_pBaseSonar->_this(), "owner", keys);

  // Raw base contact state component.
  m_pBaseContact = new FVectorState_i("BaseContact");
  m_pBaseContact->_obj_is_ready(m_BOA);
  insert_object(m_pBaseContact->_this(), "owner", keys);

  // Raw Enclosure contact state component.
  m_pEnclosureContact = new FVectorState_i("EnclosureContact");
  m_pEnclosureContact->_obj_is_ready(m_BOA);
  insert_object(m_pEnclosureContact->_this(), "owner", keys);

  // Power management state component.
  m_pPower = new PowerManagementState_i("Power");
  m_pPower->_obj_is_ready(m_BOA);
  insert_object(m_pPower->_this(), "owner", keys);

  // Debug state component
  m_pDebug = new StringState_i("Debug");
  m_pDebug->_obj_is_ready(SystemModule_i::m_BOA);
  insert_object(m_pDebug->_this(),  "owner", keys);

  // Put in our Timer component.
  m_pTimer = new PeriodicActivity_i("Timer",m_argc,m_argv);
  m_pTimer->_obj_is_ready(m_BOA);
  insert_object(m_pTimer->_this(), "owner", keys);

  MobilityBase::TimeStampData new_period;
  new_period = get_timestamp(0.05);
  m_pTimer->set_period(new_period); // ~5Hz operation of task.

  // Setup command data.
  m_wdCommandData.velocity.length(2);
  m_wdCommandData.position.length(2);
  m_wdCommandData.velocity[0] = 0.0;
  m_wdCommandData.velocity[1] = 0.0;
  m_wdCommandData.position[0] = 0.0;
  m_wdCommandData.position[1] = 0.0;

  // Drive command.
  m_curCommandData.velocity.length(2);
  m_curCommandData.velocity[0] = 0.0;
  m_curCommandData.velocity[1] = 0.0;

  // This should initialize child objects as well.
  params.length(1);

  initialize_object(params);

  fprintf(stderr,"Setup Port Connections...\n");

  // Connect hardware ports to relevant state objects.
  CORBA::Object_var tempobj;
  MobilityCore::ObjectDescriptor tempdesc;

  // Connect drive command:
  sprintf(pathnamebuf,"%s/Drive/Command",m_source);
  pathname = mbyUtility::parse_pathname(pathnamebuf);
  try {
    fprintf(stderr,"  Find %s\n",pathnamebuf);
    tempobj = find_object(pathname);
    m_pCommand = MobilityActuator::ActuatorState::_narrow(tempobj);
    if (m_pCommand == MobilityActuator::ActuatorState::_nil()) {
      fprintf( stderr, "ERROR: no B21r actuator system found !!!\n" );
      exit(0);
    }
  }
  catch(...) {
    fprintf(stderr,"  Can't drive command.\n");
  }

  // Connect drive odometry feedback.
  sprintf(pathnamebuf,"%s/Drive/State",m_source);
  pathname = mbyUtility::parse_pathname(pathnamebuf);
  try {
    fprintf(stderr,"  Find %s\n",pathnamebuf);
    tempobj = find_object(pathname);
    MobilityActuator::ActuatorState_var acttemp = m_pOdo->_this();
    if (acttemp != NULL)
      acttemp->attach_to_source(tempobj, MobilityComponents::ONEWAY_PUSH);
    else
      fprintf(stderr,"  acttemp NULL\n");
  }
  catch(...) {
    fprintf(stderr,"  Can't attach odo state.\n");
  }

  // Connect drive odometry feedback.
  sprintf(pathnamebuf,"%s/Drive/Raw",m_source);
  pathname = mbyUtility::parse_pathname(pathnamebuf);
  try {
    fprintf(stderr,"  Find %s\n",pathnamebuf);
    tempobj = find_object(pathname);
    MobilityActuator::ActuatorState_var acttemp = m_pRawOdo->_this();
    if (acttemp != NULL)
      acttemp->attach_to_source(tempobj, MobilityComponents::ONEWAY_PUSH);
    else
      fprintf(stderr,"  acttemp NULL\n");
  }
  catch(...) {
    fprintf(stderr,"  Can't attach raw odo state.\n");
  }

  // Connect raw sonar readings.

  sprintf(pathnamebuf,"%s/Sonar/Raw", m_source);
  pathname = mbyUtility::parse_pathname(pathnamebuf);
  try {
    fprintf(stderr,"  Find %s\n",pathnamebuf);
    tempobj = find_object(pathname);
    MobilityData::FVectorState_var pttemp = m_pSonar->_this();
    if (pttemp != NULL)
      pttemp->attach_to_source(tempobj, MobilityComponents::ONEWAY_PUSH);
    else
      fprintf(stderr,"  pttemp NULL\n");
  }
  catch (...) {
    fprintf(stderr,"  Can't attach sonar raw state.\n");
  }

  // Connect base sonar feedback.
  sprintf(pathnamebuf,"%s/BaseSonar/Raw", m_source);
  pathname = mbyUtility::parse_pathname(pathnamebuf);
  try {
    fprintf(stderr,"  Find %s\n",pathnamebuf);
    tempobj = find_object(pathname);
    MobilityData::FVectorState_var pttemp = m_pBaseSonar->_this();
    if (pttemp != NULL)
      pttemp->attach_to_source(tempobj, MobilityComponents::ONEWAY_PUSH);
    else
      fprintf(stderr,"  pttemp NULL\n");
  }
  catch (...) {
    fprintf(stderr,"  Can't attach base sonar raw state.\n");
  }

  // Connect base contact feedback.
  sprintf(pathnamebuf,"%s/BaseContact/Raw", m_source);
  pathname = mbyUtility::parse_pathname(pathnamebuf);
  try {
    fprintf(stderr,"  Find %s\n",pathnamebuf);
    tempobj = find_object(pathname);
    MobilityData::FVectorState_var pttemp = m_pBaseContact->_this();
    if (pttemp != NULL)
      pttemp->attach_to_source(tempobj, MobilityComponents::ONEWAY_PUSH);
    else
      fprintf(stderr,"  pttemp NULL\n");
  }
  catch (...) {
    fprintf(stderr,"  Can't attach base contact raw state.\n");
  }

  // Connect enclosure contact feedback.
  sprintf(pathnamebuf,"%s/EnclosureContact/Raw", m_source);
  pathname = mbyUtility::parse_pathname(pathnamebuf);
  try {
    fprintf(stderr,"  Find %s\n",pathnamebuf);
    tempobj = find_object(pathname);
    MobilityData::FVectorState_var pttemp = m_pEnclosureContact->_this();
    if (pttemp != NULL)
      pttemp->attach_to_source(tempobj, MobilityComponents::ONEWAY_PUSH);
    else
      fprintf(stderr,"  pttemp NULL\n");
  }
  catch (...) {
    fprintf(stderr,"  Can't attach Enclosure contact raw state.\n");
  }

  // Connect battery power feedback.
  sprintf(pathnamebuf,"%s/Power",m_source);
  pathname = mbyUtility::parse_pathname(pathnamebuf);
  try {
    fprintf(stderr,"  Find %s\n",pathnamebuf);
    tempobj = find_object(pathname);
    MobilityData::PowerManagementState_var pwrtemp = m_pPower->_this();
    if (pwrtemp != NULL)
      pwrtemp->attach_to_source(tempobj, MobilityComponents::ONEWAY_PUSH);
    else
      fprintf(stderr,"  acttemp NULL\n");
  }
  catch(...) {
    fprintf(stderr,"  Can't attach power state.\n");
  }

  fprintf(stderr,"Object / Port Connections Done.\n");

  m_LastCommandUpdate = get_current_time();
  m_pTimer->start_activity();

  return 0;
}


float
timeDiff( struct timeval* t1, struct timeval* t2)
{
  float diff=0.0;
  diff =  (float) (t1->tv_usec - t2->tv_usec) / 1000000.0;
  diff += (float) (t1->tv_sec - t2->tv_sec);
  return diff;
}

/*
  This notification handler looks for various updates and routes them
  appropriately.
*/
CORBA::Long ProxyModule::on_general_notify (const char * event,
					    const MobilityBase::NTVSeq &params,
					    CORBA::Object_ptr source,
					    CORBA::Boolean up)
{
  float tdiff;

  source = source;
  up = up;

  if (event != NULL) {
    // What event is this?
    if (strncmp(event,"Command",7)== 0) {
      // Stamp new command update times /w local time of receipt.
      m_LastCommandUpdate = get_current_time();
    } else if (strncmp(event,"Timer",5) == 0) {
      
      fprintf( stderr, "." );

      // This routine is entered by the single timer thread so,
      // TCX messaging routines and the bee_xx library are called
      // only from this timer thread.
      int i, rindex;
      float rpSonar[MAX_NUM_SENSOR_DATA];
      float rpBaseSonar[MAX_NUM_SENSOR_DATA]; 
      float rpEnclosureContact[MAX_NUM_SENSOR_DATA];
      float rpBaseContact[MAX_NUM_SENSOR_DATA];
      int   nrpSonar, nrpBaseSonar;
      int   nrpEnclosureContact, nrpBaseContact;
      BatteryStatusType battery;

      // Make sure we are getting data push.
      // fprintf( stderr, "(%d calls)", m_total_calls);
      m_total_calls = 0;
      
      if (recieveOdo) {
				m_pOdo->update_sample(0, m_curOdoData);
				odometry.x = m_curOdoData.position[0];
				odometry.y = m_curOdoData.position[1];
				odometry.theta = m_curOdoData.position[2];
				
				odometry.timestamp = carmen_get_time_ms();
      }

      if (recieveRawOdo) {
				m_pRawOdo->update_sample(0, m_curRawOdoData);
				odometry.tv = m_curRawOdoData.velocity[0];
				odometry.rv = m_curRawOdoData.velocity[1];
      }
      
      if (recievePower) {
				m_pPower->update_sample(0,m_curPowerData);
				battery.source = m_curPowerData.SourceVoltage[0];
				battery.regulator = m_curPowerData.RegulatorVoltage[0];
      }
      
      if (recieveSonar) {
				m_pSonar->update_sample(0,m_curSonarData);
				nrpSonar = (int) m_curSonarData.data.length();
				for (i=0; i<nrpSonar; i++)
					if (i<MAX_NUM_SENSOR_DATA)
						rpSonar[(35-i)%24] = 
							((m_curSonarData.data[i]*1000.0)+265);
					else
						nrpSonar = MAX_NUM_SENSOR_DATA;
      } else {
				nrpSonar = 0;
      }
      
      if (recieveBaseSonar) {
				m_pBaseSonar->update_sample(0,m_curBaseSonarData);
				nrpBaseSonar = (int) m_curBaseSonarData.data.length();
				for (i=0; i<nrpBaseSonar; i++)

	  if (i<MAX_NUM_SENSOR_DATA) {

	    // This math here is horribly, horribly wrong.

	    rindex = ((int)(24+(odometry.theta*24/360)))%24;
	    rpBaseSonar[(33+rindex-i)%24] = 
	      ((m_curBaseSonarData.data[i]*1000.0)+265);
	  } else
	    nrpBaseSonar = MAX_NUM_SENSOR_DATA;


      } else {
	nrpBaseSonar = 0;
      }

      if (recieveEnclosureContact) {
	m_pEnclosureContact->update_sample(0,m_curEnclosureContactData);
	nrpEnclosureContact = (int) m_curEnclosureContactData.data.length();
	for (i=0; i<nrpEnclosureContact; i++)
	  if (i<MAX_NUM_SENSOR_DATA)
	    rpEnclosureContact[i] = m_curEnclosureContactData.data[i];
	  else
	    nrpEnclosureContact = MAX_NUM_SENSOR_DATA;
      } else {
	nrpEnclosureContact = 0;
      }

      if (recieveBaseContact) {
	m_pBaseContact->update_sample(0,m_curBaseContactData);
	nrpBaseContact = (int) m_curBaseContactData.data.length();
	for (i=0; i<nrpBaseContact; i++)
	  if (i<MAX_NUM_SENSOR_DATA)
	    rpBaseContact[i] = m_curBaseContactData.data[i];
	  else
	    nrpBaseContact = MAX_NUM_SENSOR_DATA;
      } else {
	nrpBaseContact = 0;
      }

      /*
	send_sensor_update( nrpSonar, rpSonar,
	nrpBaseSonar, rpBaseSonar,
	nrpEnclosureContact, rpEnclosureContact,
	nrpBaseContact, rpBaseContact );
      */

      // If our actuator output is connected, then send it commands.
      if (m_pCommand != MobilityActuator::ActuatorState::_nil()) {

	gettimeofday(&now, NULL);
	tdiff = timeDiff(&now,&lastUpdate);

	if (tdiff<0.5) {
	  // m / sec
	  m_curCommandData.velocity[0] = command_tvel;
					
	  // radians / sec
	  m_curCommandData.velocity[1] = command_rvel;					
	} else {
	  // fprintf( stderr, "TimeOut for %.2f seconds!\n", tdiff );
	  m_curCommandData.velocity[0] = 0.0;
	  m_curCommandData.velocity[1] = 0.0;
	}
	  
				// Send robot new velocity command in real units.
	try {
	  m_pCommand->new_sample(m_curCommandData,0);
	}
	catch(...) {
	  fprintf(stderr,"Command error.\n");
	}
      }

    } else if (!strncmp(event,"Sonar",5)) {
      if (!recieveSonar)
	recieveSonar = TRUE;
    } else if (!strncmp(event,"BaseSonar",9)) {
      if (!recieveBaseSonar)
	recieveBaseSonar = TRUE;
    } else if (!strncmp(event,"Odo",3)) {
      if (!recieveOdo)
	recieveOdo =  TRUE;
    } else if (!strncmp(event,"RawOdo",6)) {
      if (!recieveRawOdo)
	recieveRawOdo = TRUE;
    } else if (!strncmp(event,"BaseContact",11)) {
      if (!recieveBaseContact)
	recieveBaseContact = TRUE;
    } else if (!strncmp(event,"EnclosureContact",16)) {
      if (!recieveEnclosureContact)
	recieveEnclosureContact = TRUE;
    } else if (!strncmp(event,"Power",5)) {
      if (!recievePower)
	recievePower =  TRUE;
    } 

    return 0;
  } else
    return -1;
}

void
shutdown_robot(int signal)
{
  signal = signal;

  exitP = TRUE;
  fprintf(stderr,"SHUTDOWN ROBOT!\n");
  fprintf(stderr,"Release Resources\n");
  pProxyModule->release_resources();

  fprintf(stderr, "shutting down module\n");
  pProxyModule->shutdown_module();

  fprintf(stderr, "sync_shutdown (10secs)\n");
  pProxyModule->sync_shutdown(10000000);

  fprintf( stderr,"sync_done\n");
  exit(0);
}

int
main( int argc, char* argv[])
{
  char * sourceName = "B21R";
  char * moduleName = "b21rProxy";
  
  // Initialize ORB for this thread, before creating any other things:

  SystemModule_i::init_orb(argc,argv);

  pProxyModule = new ProxyModule(moduleName,sourceName);
  if (pProxyModule == NULL) {
    fprintf(stderr,"Module creation error.\n");
    return -1;
  }

  fprintf( stderr, "*****************************************\n" );
  fprintf( stderr, "init_module\n");
  pProxyModule->init_module(argc,argv);

  fprintf( stderr, "*****************************************\n" );
  fprintf( stderr, "activate_module\n");
  pProxyModule->activate_module();

  fprintf( stderr, "*****************************************\n" );
  fprintf( stderr, "sync_ready (10secs)\n");
  pProxyModule->sync_ready(10000000);

  fprintf( stderr, "*****************************************\n" );
  fprintf( stderr, "synchronized /w module\n");

  signal(SIGINT,  shutdown_robot);

  carmen_initialize_ipc(argv[0]);

  carmen_b21r_start(argc, argv);

  while (1) {
    sleep_ipc(0.1);
    carmen_b21r_run();
  }
  
  return 0;
}

