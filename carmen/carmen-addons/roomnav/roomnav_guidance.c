#include <carmen/carmen.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <theta.h>
#include <carmen/localize_interface.h>
#include "roomnav_interface.h"
#include "roomnav_guidance_messages.h"

static carmen_roomnav_guidance_msg guidance_msg;
static carmen_roomnav_goal_changed_msg goal_msg;
static carmen_roomnav_path_msg path_msg; 
static carmen_roomnav_room_msg room_msg;
static carmen_localize_globalpos_message globalpos_msg;
static carmen_point_t last_cmd_pose;
static carmen_rooms_topology_p topology;
static carmen_map_t map;

static int config_audio = 1; //determines whether audio is on
static int config_text = 1; //determines whether text (publishing) is on

static int new_goal = 0;
static int goal_set = 0; //whether goal is set or not
static int room_set = 0; //whether room has been set or not
static int wait_for_path = 0;
static int computing_command = 0;
static int corrupt_command = 0;
#define PATH_CORRUPT 1
#define ROOM_CORRUPT 2

const float PI_HALF = M_PI / 2.0;
const float PI_QUARTER = M_PI / 4.0;
const float PI_3QUARTER = 3.0 * M_PI / 4;
const float PI_TWELFTH = M_PI / 12.0;


//const float HALLWAY_ANGLE_DIFF_THRESHOLD = M_PI/8.0;
#define CORNER_DIST_THRESHOLD 2.0
#define DOOR_DIST_THRESHOLD 3.0
#define NEAR_DOOR 2.0 //parameter for saying "go down the hallway"
#define VERY_NEAR_DOOR 0.5 //for hallway->hallway, give guidance wrt next hallway rather than next_door
#define END_OF_HALLWAY_THRESHOLD 6.0 //parameter for saying "go down the hallway", and for being near corner or intersection
#define STRAIGHT_AHEAD_THRESHOLD 1.0


#define OPTION_DISTANCE 4.0
#define CONTINUE_DOWN_HALLWAY 4.0

#define LARGE_EPSILON 4.0
const float EPSILON =  M_PI / 8.0; 
const float EPSILON4 =  M_PI / 4.0; 

static int last_dir = -1;
static double last_time = -1;
#define DELAY_BETWEEN_COMMANDS 2 //2 seconds

static double last_goal_time = -1;
//#define NEW_GOAL 3.0 //3 sec


#define CARMEN_ROOMNAV_GUIDANCE_KEEP_GOING2 100 //<-- HACK!
static int keep_going_mode = 0; // <-- HACK!
static double last_room_state = 100; //for resolving crossing doors problem. Either >0 or <0



// assumes guidance_msg has already been filled in
static void publish_guidance_msg() {

  static int first = 1;
  IPC_RETURN_TYPE err;
  
  if (first) {
    strcpy(guidance_msg.host, carmen_get_tenchar_host_name());
    first = 0;
  }

  if (!config_text)
    return;

  guidance_msg.timestamp = carmen_get_time_ms();

  err = IPC_publishData(CARMEN_ROOMNAV_GUIDANCE_MSG_NAME, &guidance_msg);
  carmen_test_ipc_exit(err, "Could not publish", CARMEN_ROOMNAV_GUIDANCE_MSG_NAME);  
}




/////////////////////////////////////
// HELPER FUNCTIONS
/////////////////////////////////////

/* Rounds theta (in radians) to the nearest 90deg
   angle. Uses right-handed coord frame.
   Assumes theta is normalized.
*/
int round_dir(double theta)
{
  if(theta == 0) {
    return CARMEN_ROOMNAV_GUIDANCE_FORWARD;
  } else if(theta > 0) {
    if(theta < PI_QUARTER) {
      return CARMEN_ROOMNAV_GUIDANCE_FORWARD;
    } else if(theta < PI_3QUARTER) {
      return CARMEN_ROOMNAV_GUIDANCE_RIGHT;
    } else {
      return CARMEN_ROOMNAV_GUIDANCE_BACKWARD;
    }
  } else { // theta < 0
    if(theta > -1 * PI_QUARTER) {
      return CARMEN_ROOMNAV_GUIDANCE_FORWARD;
    } else if(theta > -1 * PI_3QUARTER) {
      return CARMEN_ROOMNAV_GUIDANCE_LEFT;
    } else {
      return CARMEN_ROOMNAV_GUIDANCE_BACKWARD;
    }
  }
  
  return -1;
}


static void fork_speech(char *s, int command, int alt, int please) {

  static cst_voice *vox;
  static theta_voice_desc *vlist;
  static int first = 1;
  char wav_filename[100];
  int speech_pid;
  int status;
  
  //s = NULL;  //dbug

  
  if (alt) {
    if (please)
      sprintf(wav_filename, "/home/jrod/wavs/GUIDANCE%dpa.wav", command);
    else
      sprintf(wav_filename, "/home/jrod/wavs/GUIDANCE%da.wav", command);
  }
  else {
    if (please)
      sprintf(wav_filename, "/home/jrod/wavs/GUIDANCE%dp.wav", command);
    else
      sprintf(wav_filename, "/home/jrod/wavs/GUIDANCE%d.wav", command);
  }

  if (!carmen_file_exists(wav_filename)) {
    if (first) {
      vlist = theta_enum_voices(NULL, NULL);
      first = 0;
    }
    
    vox = theta_load_voice(vlist);
    while (vox == NULL) {
      printf("ERROR: vox == NULL!\n");
      vox = theta_load_voice(vlist);
      usleep(100000);
    }
    
    //theta_set_pitch_shift(vox, 0.1, NULL);
    //theta_set_rate_stretch(vox, 1.2, NULL);
  
    theta_set_outfile(vox, wav_filename, "riff", 0, 2);
    theta_tts(s, vox);
    theta_unload_voice(vox);
  }

  speech_pid = fork();
  if (speech_pid == 0)
    {
      execlp("play", "play", wav_filename, NULL);
      carmen_die_syserror("Could not exec play");
    }
  else if (speech_pid == -1)
    carmen_die_syserror("Could not fork to start play");
  else
    waitpid(speech_pid, &status, 0);
}

/* Prints direction for user.
 */
void print_dir_for_user(int dir)
{
  char pls[10];
  char pls2[10];
  char guidance[200];
  int randn, please, altdir;
  double cur_time;

  please = altdir = 0;

  guidance_msg.cmd = dir;
  randn = rand() % 10;

  if(randn < 5) {
    please = 1;
    strcpy(pls, "Please ");
    strcpy(pls2, "Please\n");
  } else {
    please = 0;
    strcpy(pls, "");
    strcpy(pls2, "");
  }

  cur_time = carmen_get_time_ms();

  //Enforcing 2 second mandatory delay between commands
  if(cur_time < last_time + DELAY_BETWEEN_COMMANDS &&
     dir != CARMEN_ROOMNAV_GUIDANCE_DESTINATION_REACHED && !new_goal) {
    guidance_msg.cmd = last_dir;
    goto END;
  }


  /* Switch "GO FORWARD/GO STRAIGHT" to "KEEP GOING" IF
     SINCE MORE THAN 25 seconds
  */
 
  //printf("**cur_time is: %f**\n", cur_time);
  //printf("**last_time is: %f**\n", last_time);

  if(last_time != -1 && !new_goal) {
    //printf("-- %f\n", (cur_time - last_time));

    if((cur_time - last_time) > 25) { //<<-- "keep going" threshold
      
      if(dir == CARMEN_ROOMNAV_GUIDANCE_FORWARD) {
	dir = CARMEN_ROOMNAV_GUIDANCE_KEEP_GOING;
	keep_going_mode = 1;
      } 
    }
  }


  if(!((dir == CARMEN_ROOMNAV_GUIDANCE_FORWARD) || (dir == CARMEN_ROOMNAV_GUIDANCE_KEEP_GOING)
       || (dir == CARMEN_ROOMNAV_GUIDANCE_KEEP_GOING2))) {
    keep_going_mode = 0;
  }

  if(keep_going_mode == 1) {
    if(last_dir == CARMEN_ROOMNAV_GUIDANCE_KEEP_GOING) {
      if(cur_time - last_time > 20) {
	dir = CARMEN_ROOMNAV_GUIDANCE_KEEP_GOING2;
      } else {
	dir = CARMEN_ROOMNAV_GUIDANCE_KEEP_GOING;
      }
    } else if(last_dir == CARMEN_ROOMNAV_GUIDANCE_KEEP_GOING2) {
      if(cur_time - last_time > 20) {
	dir = CARMEN_ROOMNAV_GUIDANCE_KEEP_GOING;
      } else {
	dir = CARMEN_ROOMNAV_GUIDANCE_KEEP_GOING2;
      }
    } else { 
      dir = CARMEN_ROOMNAV_GUIDANCE_KEEP_GOING;
    }  
  }


  if (last_dir != dir) {
   
    if (guidance_msg.text)
      free(guidance_msg.text);
    guidance_msg.text = NULL;

    if(dir == CARMEN_ROOMNAV_GUIDANCE_FORWARD) {
      if(rand() % 10< 5) {
	altdir = 0;
	sprintf(guidance, "\"%sGO FORWARD\"", pls);
	guidance_msg.text = carmen_new_string("%sgo forward.", pls2);
      } else {
	altdir = 1;
	sprintf(guidance, "\"%sGO STRAIGHT\"", pls);
	guidance_msg.text = carmen_new_string("%sgo straight.", pls2);	
      }

    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_LEFT) {
      sprintf(guidance, "\"%sTURN LEFT\"", pls);
      guidance_msg.text = carmen_new_string("%sturn left.", pls2);
    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_BACKWARD) {
      sprintf(guidance, "\"%sTURN AROUND\"", pls);
      guidance_msg.text = carmen_new_string("%sturn around.", pls2);
    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_RIGHT) {
      sprintf(guidance, "\"%sTURN RIGHT\"", pls);
      guidance_msg.text = carmen_new_string("%sturn right.", pls2);
    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_GO_THROUGH_DOORWAY) {
      sprintf(guidance, "\"%sGO THROUGH THE DOORWAY\"", pls);
      guidance_msg.text = carmen_new_string("%sgo through\n the doorway.", pls2);
    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_ENTER_HALLWAY) {
      sprintf(guidance, "\"%sENTER THE HALLWAY\"", pls);
      guidance_msg.text = carmen_new_string("%senter\n the\n hallway.", pls2);
    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_ENTER_ROOM_IN_FRONT) {
      sprintf(guidance, "\"%sENTER THE ROOM IN FRONT OF YOU\"", pls);
      guidance_msg.text = carmen_new_string("%senter room\n in front\n of you.", pls2);
    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_LEFT_CORNER) {
      sprintf(guidance, "\"%sTURN LEFT AT THE CORNER\"", pls);
      guidance_msg.text = carmen_new_string("%sturn left\n at the\n corner.", pls2);
    } else if (dir == CARMEN_ROOMNAV_GUIDANCE_RIGHT_CORNER) {
      sprintf(guidance, "\"%sTURN RIGHT AT THE CORNER\"", pls);
      guidance_msg.text = carmen_new_string("%sturn right\n at the\n corner.", pls2);
    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_GO_TO_END_OF_HALLWAY) {
      sprintf(guidance, "\"%sGO TO THE END OF THE HALLWAY\"", pls);
      guidance_msg.text = carmen_new_string("%sgo to the\n end of\n the hallway.", pls2);
    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_TURN_LEFT_AT_INTERSECTION) {
      sprintf(guidance, "\"%sTURN LEFT AT THE INTERSECTION\"", pls);
      guidance_msg.text = carmen_new_string("%sturn left\n at the\n intersection.", pls2);
    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_TURN_RIGHT_AT_INTERSECTION) {
      sprintf(guidance, "\"%sTURN RIGHT AT THE INTERSECTION\"", pls);
      guidance_msg.text = carmen_new_string("%sturn right\n at the\n intersection.", pls2);
    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_TURN_NEXT_LEFT) {

      if(rand() % 10 < 5) {
	altdir = 0;
	sprintf(guidance, "\"%sTURN AT THE NEXT LEFT\"", pls);
	guidance_msg.text = carmen_new_string("%sturn\n at the\n next left.", pls2);
      } else {
	altdir = 1;
	sprintf(guidance, "\"%sTAKE THE NEXT LEFT\"", pls);
	guidance_msg.text = carmen_new_string("%stake\n the\n next left.", pls2);
      }

    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_TURN_NEXT_RIGHT) {

      if(rand() % 10 < 5) {
	altdir = 0;
	sprintf(guidance, "\"%sTURN AT THE NEXT RIGHT\"", pls);
	guidance_msg.text = carmen_new_string("%sturn\n at the\n next right.", pls2);
      } else {
	altdir = 1;
	sprintf(guidance, "\"%sTAKE THE NEXT RIGHT\"", pls);
	guidance_msg.text = carmen_new_string("%stake\n the\n next right.", pls2);
      }

    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_TURN_RIGHT_AT_DOORWAY) {
      sprintf(guidance, "\"%sTURN RIGHT AT THE DOORWAY\"", pls);
      guidance_msg.text = carmen_new_string("%sturn right\n at the\n doorway.", pls2);

    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_TURN_LEFT_AT_DOORWAY) {
      sprintf(guidance, "\"%sTURN LEFT AT THE DOORWAY\"", pls);
      guidance_msg.text = carmen_new_string("%sturn left\n at the\n doorway.", pls2);


    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_ENTER_HALLWAY_TURN_RIGHT) {
      sprintf(guidance, "\"%sENTER THE HALLWAY AND TURN RIGHT\"", pls);
      guidance_msg.text = carmen_new_string("%senter hallway\n and\n turn right.", pls2);
    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_ENTER_HALLWAY_TURN_LEFT) {
      sprintf(guidance, "\"%sENTER THE HALLWAY AND TURN LEFT\"", pls);
      guidance_msg.text = carmen_new_string("%senter hallway\n and\n turn left.", pls2);

    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_CONTINUE_DOWN_HALLWAY) {
      sprintf(guidance, "\"%sCONTINUE DOWN THE HALLWAY\"", pls);
      guidance_msg.text = carmen_new_string("%scontinue\n down \nthe hallway.", pls2);
      
    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_TAKE_LEFT_HALLWAY) {
      sprintf(guidance, "\"%sTAKE THE LEFT HALLWAY\"", pls);
      guidance_msg.text = carmen_new_string("%take the\n hallway on\n your left.", pls2);      
    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_TAKE_RIGHT_HALLWAY) {
      sprintf(guidance, "\"%sTAKE THE RIGHT HALLWAY\"", pls);
      guidance_msg.text = carmen_new_string("%stake the\n hallway on\n your right.", pls2);
    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_KEEP_GOING) {
      sprintf(guidance, "\"KEEP GOING\"");
      guidance_msg.text = carmen_new_string("Keep\n going.");
    
    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_KEEP_GOING2) {
      sprintf(guidance, "\"KEEP GOING\"");
      guidance_msg.text = carmen_new_string("Keep\n going.");

    } else if(dir == CARMEN_ROOMNAV_GUIDANCE_DESTINATION_REACHED) {
      sprintf(guidance, "\"YOU HAVE ARRIVED\"");
      guidance_msg.text = carmen_new_string("You\n have\n arrived");
    }

    if (strlen(pls) == 0) {
      guidance[0] = toupper(guidance[0]);
      guidance_msg.text[0] = toupper(guidance_msg.text[0]);
    }

    printf("%s\n", guidance);
    last_cmd_pose = globalpos_msg.globalpos;
    last_dir = dir;
    last_time = cur_time;

    publish_guidance_msg();

    if(config_audio)
      fork_speech(guidance, dir, altdir, please);

    return;
  }

 END:
  publish_guidance_msg();
}



/////////////////////////////////////
// UTIL FUNCTIONS
/////////////////////////////////////
void *Signal(int signum, void *handler) {
  struct sigaction action, old_action;
  
  action.sa_handler = handler;
  sigemptyset(&action.sa_mask);
  action.sa_flags = SA_RESTART;

  if(sigaction(signum, &action, &old_action) < 0) 
    printf("Signal error\n");
  return(old_action.sa_handler);
}


void sigchld_handler() {
  int status;
  waitpid(-1, &status, WNOHANG);

}


void shutdown_module(int sig)
{
  if(sig == SIGINT) {
    //carmen_logger_fclose(outfile);
    close_ipc();
    fprintf(stderr, "\nDisconnecting.\n");
    exit(0);
  }
}


double max(double d1, double d2) {
  return (d1 > d2 ? d1 : d2);
}

double min(double d1, double d2) {
  return (d1 < d2 ? d1 : d2);
}

double dist(double x1, double y1, double x2, double y2) {
  return sqrt((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
}


double projected_dist_to_door(carmen_point_p robot_pose, carmen_door_p door) {
 
  double tmp_x, tmp_y;
  
  tmp_x = robot_pose->x;
  tmp_y = robot_pose->y;
  tmp_x -= (door->pose).x;
  tmp_y -= (door->pose).y;

  carmen_rotate_2d(&tmp_x, &tmp_y, -1 * (door->pose).theta); 
  return max(fabs(tmp_y) - (door->width /2.0), 0);
  
}



double normalized_dist_to_door(carmen_point_p robot_pose, carmen_door_p door) {

  double tmp_x, tmp_y;

  if(projected_dist_to_door(robot_pose, door) == 0) { //within
    tmp_x = robot_pose->x;
    tmp_y = robot_pose->y;
    tmp_x -= (door->pose).x;
    tmp_y -= (door->pose).y;
    carmen_rotate_2d(&tmp_x, &tmp_y, -1 * (door->pose).theta);
    return fabs(tmp_x);
  }

  return min(dist(robot_pose->x, robot_pose->y, 
		  door->points.places[0].x, door->points.places[0].y), 
	     dist(robot_pose->x, robot_pose->y, 
		  door->points.places[1].x, door->points.places[1].y));    
}





/** 
    Return whether roughly perpendicular or not.
*/
int perpendicular(double theta) {

  //printf("PERPENDICULAR: theta is: %f\n", theta);
  if(((theta > PI_HALF - EPSILON) && (theta < PI_HALF + EPSILON)) 
     || ((theta < -1*PI_HALF + EPSILON) && (theta > -1*PI_HALF - EPSILON))) {
    return 1;
  }
  return 0;

}


/** Return whether roughly parallel or not.
    Assumes theta is normalized. */
int parallel(double theta) {

  //printf("PARALLEL: theta is: %f\n", theta);
  if(((theta > -1 * EPSILON) && (theta < EPSILON)) 
     || (theta > M_PI - EPSILON)
     || (theta < -1*M_PI + EPSILON)) {
    return 1;
  }
  return 0;

}


/** Return whether roughly parallel or not.
    Assumes theta is normalized. */
int parallel4(double theta) {

  //printf("PARALLEL: theta is: %f\n", theta);
  if(((theta > -1 * EPSILON4) && (theta < EPSILON4)) 
     || (theta > M_PI - EPSILON4)
     || (theta < -1*M_PI + EPSILON4)) {
    return 1;
  }
  return 0;

}



/* Compare by x value */
int compare_doors_by_x(carmen_door_t *d1, carmen_door_t *d2) {
  double tmp = (d1->pose).x - (d2->pose).y;
  if(tmp > 0) return 1;
  else if(tmp < 0) return -1;
  else return 0;
}




/**
   To call at the end of both cases:
   HALLWAY->ROOM and HALLWAY->HALLWAY
   @param door_struct The next door in the robot's path to goal
   @param robot_pose  Robot's current pose
   @param dir         Direction that the robot should be turning next to face door
   @return decision   Command decision
*/
int get_state_decision_general_hallway(carmen_point_p robot_pose,
				       carmen_point_p cur_room_end, 
				       carmen_point_p next_door_pose, 
				       double theta_robot_door_loc)
{
  //for "go to end of hallway"
  //if the door we are going to next is within a certain distance
  //from the endpoint of the room closer to the door,
  //then say "go to the end of hallway"
  if(((carmen_distance(cur_room_end, next_door_pose)) * cos(theta_robot_door_loc) < NEAR_DOOR)
     && ((carmen_distance(robot_pose, cur_room_end)) * cos(theta_robot_door_loc) 
	 > END_OF_HALLWAY_THRESHOLD)) {
    return CARMEN_ROOMNAV_GUIDANCE_GO_TO_END_OF_HALLWAY;
  }

  //printf("next_door_pose: %f, %f\n", next_door_pose->x, next_door_pose->y);
  //printf("cur_room_end: %f, %f\n", cur_room_end->x, cur_room_end->y);
  
  return -1;

}




/**
   To call to find out whether we should return "go straight". 
*/
int get_state_decision_straight_through(carmen_door_p following_door,
					carmen_room_p cur_room, 
					int dir)

{
  double tmp_x, tmp_y;
  tmp_x = (following_door->pose).x;
  tmp_y = (following_door->pose).y;
  tmp_x -= cur_room->ux;
  tmp_y -= cur_room->uy;
   
  carmen_rotate_2d(&tmp_x, &tmp_y, -1 * cur_room->theta); 

  if((tmp_y < LARGE_EPSILON) && (tmp_y > -LARGE_EPSILON)) { 
    //return CARMEN_ROOMNAV_GUIDANCE_FORWARD;
    return dir;
  } 
  
  return -1;

}


/**
 * robot_to_door --> where it should go
 * tmp_robot_to_door --> an option
 *
 */
int get_room_guidance_dir(int decision, int next_room_type,
			  double robot_to_door, double tmp_robot_to_door) {
  
  int relative_dir;
  //new robot to door
  double new_robot_to_door;

  //new_robot_to_door = 0; // = robot_to_door - robot_to_door;
  new_robot_to_door = carmen_normalize_theta(robot_to_door - tmp_robot_to_door);
  //new_tmp_robot_to_door = carmen_normalize_theta(new_tmp_robot_to_door);

  //printf("robot_to_door: %f\n", robot_to_door);
  //printf("tmp_robot_to_door: %f\n", tmp_robot_to_door);
  //printf("new_tmp_robot_to_door: %f\n", new_tmp_robot_to_door);

  if(new_robot_to_door > 0) {
    relative_dir = CARMEN_ROOMNAV_GUIDANCE_LEFT;
  } else { // < 0
    relative_dir = CARMEN_ROOMNAV_GUIDANCE_RIGHT;
  }

  //printf("---decision: %d\n", decision);
  
  if(next_room_type == CARMEN_ROOM_TYPE_HALLWAY) {
    switch(relative_dir) {
      //case CARMEN_ROOMNAV_GUIDANCE_FORWARD: 
      //return CARMEN_ROOMNAV_GUIDANCE_CONTINUE_DOWN_HALLWAY;
    case CARMEN_ROOMNAV_GUIDANCE_LEFT: 
      return CARMEN_ROOMNAV_GUIDANCE_TAKE_LEFT_HALLWAY;
    case CARMEN_ROOMNAV_GUIDANCE_RIGHT: 
      return CARMEN_ROOMNAV_GUIDANCE_TAKE_RIGHT_HALLWAY;
    default:
      break;
    }
  } else if(next_room_type == CARMEN_ROOM_TYPE_ROOM) {
    switch(relative_dir) {
      //case CARMEN_ROOMNAV_GUIDANCE_FORWARD: 
      //return CARMEN_ROOMNAV_GUIDANCE_ENTER_ROOM_IN_FRONT;
    case CARMEN_ROOMNAV_GUIDANCE_LEFT: 
      return CARMEN_ROOMNAV_GUIDANCE_ENTER_LEFT_ROOM;
    case CARMEN_ROOMNAV_GUIDANCE_RIGHT: 
      return CARMEN_ROOMNAV_GUIDANCE_ENTER_RIGHT_ROOM;
    default:
      break;
    }
  }
  


  return decision;
}



/**
 * Calculate angle between robot and door.
 */
double get_robot_to_door(carmen_point_p robot_pose, carmen_point_p door_pose) {

  //calculate angle between robot & door
  double robot_to_door = 0.0;
  carmen_door_p following_door;
  
  double threshold = 0.0;

  if (topology->rooms[room_msg.room].type == CARMEN_ROOM_TYPE_HALLWAY)
    threshold = 2.0;
  else
    threshold = 0.5;

  if (carmen_distance(robot_pose, door_pose) < threshold) { //TODO: MAKE DEPEND ON DOOR'S WIDTH

    if (room_msg.room == topology->doors[path_msg.path[0]].room1) {
      // if next room is a hallway
      if((topology->rooms[topology->doors[path_msg.path[0]].room2].type
	  == CARMEN_ROOM_TYPE_HALLWAY) && (path_msg.pathlen > 1)) {

	following_door = &topology->doors[path_msg.path[1]];
	robot_to_door = atan2(following_door->pose.y - robot_pose->y, 
			      following_door->pose.x - robot_pose->x);
	
      } else {
	robot_to_door = door_pose->theta;
      }
    } else {
      // if next room is a hallway
      if((topology->rooms[topology->doors[path_msg.path[0]].room1].type
	  == CARMEN_ROOM_TYPE_HALLWAY) && (path_msg.pathlen > 1)) {

	following_door = &topology->doors[path_msg.path[1]];
	robot_to_door = atan2(following_door->pose.y - robot_pose->y, 
			      following_door->pose.x - robot_pose->x);
	
      } else {
	robot_to_door = carmen_normalize_theta(door_pose->theta + M_PI);
      }
    }

  } else {
    //this could give negative angles
    robot_to_door = atan2(door_pose->y - robot_pose->y, door_pose->x - robot_pose->x);
  }

  //printf("returning rtd: %f\n", robot_to_door);

  return robot_to_door;

}

double get_normalized_robot_to_door(carmen_point_p robot_pose, carmen_door_p door) {

  //calculate angle between robot & door
  double robot_to_door = 0.0;
  carmen_door_p following_door;
  carmen_point_p door_pose = &door->pose;
  
  double threshold = 0.0;

  if (topology->rooms[room_msg.room].type == CARMEN_ROOM_TYPE_HALLWAY)
    threshold = 2.0;
  else
    threshold = 1.0;

  if (normalized_dist_to_door(robot_pose, door) < threshold) { //TODO: MAKE DEPEND ON DOOR'S WIDTH

    if (room_msg.room == topology->doors[path_msg.path[0]].room1) {
      // if next room is a hallway
      if((topology->rooms[topology->doors[path_msg.path[0]].room2].type
	  == CARMEN_ROOM_TYPE_HALLWAY) && (path_msg.pathlen > 1)) {

	following_door = &topology->doors[path_msg.path[1]];
	robot_to_door = atan2(following_door->pose.y - robot_pose->y, 
			      following_door->pose.x - robot_pose->x);
	
      } else {
	robot_to_door = door_pose->theta;
      }
    } else {
      // if next room is a hallway
      if((topology->rooms[topology->doors[path_msg.path[0]].room1].type
	  == CARMEN_ROOM_TYPE_HALLWAY) && (path_msg.pathlen > 1)) {

	following_door = &topology->doors[path_msg.path[1]];
	robot_to_door = atan2(following_door->pose.y - robot_pose->y, 
			      following_door->pose.x - robot_pose->x);
	
      } else {
	robot_to_door = carmen_normalize_theta(door_pose->theta + M_PI);
      }
    }

  } else {
    //this could give negative angles
    robot_to_door = atan2(door_pose->y - robot_pose->y, door_pose->x - robot_pose->x);
  }

  //printf("returning rtd: %f\n", robot_to_door);

  return robot_to_door;

}



/**
 * Get "basic dir" to next door (forward, right, left, backward)
 *
 */
int get_dir_to_door(carmen_point_p robot_pose, carmen_door_p door) {

  double robot_to_door;
  double theta_diff;
  int dir;

  robot_to_door = get_normalized_robot_to_door(robot_pose, door);
  
  //calculate difference between robot's angle and the door's angle
  theta_diff = carmen_normalize_theta(robot_pose->theta - robot_to_door);    
  dir = round_dir(theta_diff);
    
  return dir;
}



/**
   Check if this guidance is easily confused with any other option.
   If it is, then give more specific guidance.
*/
int check_guidance(carmen_point_t robot_pose, int decision) {
  
  //if decision is one of the basic decisions
  //  check for options
  //  if dir to any of the options matches decision
  //    then return a more specific guidance

  int i, tmp_i;
  int tmp_dir;
  double tmp_angle1, tmp_angle2;
  carmen_door_t next_door;
  carmen_point_t room_end;
  carmen_room_p cur_room = NULL;
  carmen_room_p next_room = NULL;
  double robot_to_door;
  double tmp_robot_to_door;
  

  robot_to_door = get_robot_to_door(&robot_pose, &(next_door.pose));
  
  next_door = (topology->doors[path_msg.path[0]]);
  
  if(decision == CARMEN_ROOMNAV_GUIDANCE_FORWARD) {
      
    //search for all other doors within 4metre radius
    //Detecting ROOM - HALLWAY combinations of next door  
    cur_room = &(topology->rooms[room_msg.room]);
    if (room_msg.room == next_door.room1) {
      next_room = &(topology->rooms[next_door.room2]);
    } else {
      next_room = &(topology->rooms[next_door.room1]);
    }
 
    //checking door options
    for(i=0; i<cur_room->num_doors; ++i) {
      tmp_i = cur_room->doors[i];
      
      
      if(tmp_i != next_door.num) {
	if(carmen_distance(&robot_pose, &(topology->doors[tmp_i].pose)) 
	   < OPTION_DISTANCE) {
	
	  tmp_dir = get_dir_to_door(&robot_pose, &(topology->doors[tmp_i]));
	  tmp_robot_to_door = get_robot_to_door(&robot_pose, 
						&(topology->doors[tmp_i].pose));
	  
	  if(tmp_dir == decision) { //ambiguous guidance, need to issue more specific
	    printf("changed guidance -- door option\n");
	    return get_room_guidance_dir(decision, next_room->type, 
					 robot_to_door, tmp_robot_to_door);
	  }
	}
      }
    } //end for
    

    //checking down hallway option
    //figure out distance till end of room
    tmp_angle1 = atan2(cur_room->e1.y - robot_pose.y, 
		       cur_room->e1.x - robot_pose.x);
    tmp_angle2 = atan2(cur_room->e2.y - robot_pose.y, 
		       cur_room->e2.x - robot_pose.x);

    //    if(fabs(carmen_normalize_theta(cur_room->theta - tmp_angle1)) 
    // < fabs(carmen_normalize_theta(cur_room->theta - tmp_angle2))) {

    if (fabs(carmen_normalize_theta(robot_pose.theta - tmp_angle1)) < 
	fabs(carmen_normalize_theta(robot_pose.theta - tmp_angle2))) {
      room_end = cur_room->e1;
    } else {
      room_end = cur_room->e2;
    }

    //if we're going down a hallway
    //double tmpf = carmen_distance(&robot_pose, &room_end); 
    //printf("distance: %f\n", tmpf);

    if(carmen_distance(&(next_door.pose), &room_end) > CONTINUE_DOWN_HALLWAY) {
      printf("changed guidance -- door option **2**\n");
      tmp_robot_to_door = get_robot_to_door(&robot_pose, &room_end);
      
      return get_room_guidance_dir(decision, next_room->type, 
				   robot_to_door, tmp_robot_to_door);
    }
    
  } // end if front/right/left

  return decision;
}







/**
   Retrieve the current state of things
   @param door_struct The next door in the robot's path to goal
   @param robot_pose  Robot's current pose
   @param dir         Direction that the robot should be turning next to face door
   @return decision   Command decision
*/
int get_state_decision(carmen_point_t robot_pose, int dir) 
{
  double cur_time;
  carmen_door_t next_door = (topology->doors[path_msg.path[0]]);
  carmen_door_t following_door;
  double room_angle_diff, door_angle_diff;
  double door_theta = -1;
  double distance_from_door = -1;
  double tmp;
  
  int tmp_int;
  double tmp_x, tmp_y;
  double tmp_x2, tmp_y2;

  //end-points of hallway to rotate, for hallway->hallway case
  carmen_point_t room_to_rotate_e1;
  carmen_point_t room_to_rotate_e2;  

  carmen_room_p cur_room = NULL;
  carmen_room_p next_room = NULL;
  //carmen_room_p following_room = NULL;
  int is_side_door = -1; //identify location of door wrt corner
  int corner = -1; //identify left or right corner
  carmen_point_t cur_room_end; //endpoint of cur_room closer to next_room
  //carmen_point_t cur_room_robotend; //endpoint of cur_room closer to robot
  int is_hallway_bend = 1; 
  int is_perpendicular_bend = 1;

  carmen_world_point_t world_point; 
  carmen_map_point_t map_point; 
  int room_actual;

  double theta_robot_door; //angle between robot's pose and next_door's pose
  double theta_robot_door_loc; /*angle between robot's location and next_door's location
				 for proj, dep on is_side_door. only relevant for hallways */
  double half_next_door; //half the width of next_door

  double tmp_angle;
  double normalized_angle; //used in hallway->room, and room->hallway

  double norm_dist_to_door;
  double proj_dist_to_door;


  half_next_door = next_door.width / 2.0;

  world_point.map = &map; 
  map_point.map = &map; 
    
  world_point.pose.x = globalpos_msg.globalpos.x;
  world_point.pose.y = globalpos_msg.globalpos.y;
  carmen_world_to_map(&world_point, &map_point); 
  
  cur_time = carmen_get_time_ms();

  distance_from_door = fabs(carmen_distance(&robot_pose, &(next_door.pose))); 
  norm_dist_to_door = normalized_dist_to_door(&robot_pose, &next_door);
  proj_dist_to_door = projected_dist_to_door(&robot_pose, &next_door);

  //printf("break 1 (%.3f): corrupt_command = %d\n", carmen_get_time_ms(), corrupt_command);
  //fflush(0);
  // wait for path if previous path still contains previous door
  room_actual = room_msg.room; //carmen_roomnav_get_room_from_point(map_point);
  //printf("break 2 (%.3f): corrupt_command = %d\n", carmen_get_time_ms(), corrupt_command);
  //fflush(0);

  if (room_actual != room_msg.room) { //haven't got right message yet
    //but didn't just receive a new path msg
    if (!(corrupt_command & PATH_CORRUPT)) {
      wait_for_path = 1;
      printf("set wait for path 1\n");
      return -1;
    }
  }

  tmp_x = robot_pose.x;
  tmp_y = robot_pose.y;
  tmp_x -= next_door.pose.x;
  tmp_y -= next_door.pose.y; 
  
  normalized_angle = next_door.pose.theta;
  
  //tmp_angle = carmen_normalize_theta(next_door.pose.theta - robot_pose.theta);
  //if(fabs(tmp_angle) > (M_PI/2.0)) {
  //  normalized_angle = carmen_normalize_theta(next_door.pose.theta + M_PI);
  //}
  carmen_rotate_2d(&tmp_x, &tmp_y, -normalized_angle);

  //NORMALIZED
  if(norm_dist_to_door < NEAR_DOOR) {
    if((tmp_x >= 0 && last_room_state <= 0) || (tmp_x <= 0 && last_room_state >= 0)) { 
      if(last_room_state != 100 && !(corrupt_command & PATH_CORRUPT)) {
	//printf("corrupt_command = %d\n", corrupt_command);
	last_room_state = tmp_x;
	printf("returning due to border case\n");
	wait_for_path = 1;
	printf("set wait for path 2\n");
	return -1;
      }
    }
  }

  last_room_state = tmp_x;

  if (corrupt_command) {
    printf("returned -- corrupt command\n");
    return -1;
  }



  //debug -- removeme
  //printf("next_door is: %d\n", next_door.num);


  //Detecting ROOM - HALLWAY combinations of next door  
  cur_room = &(topology->rooms[room_msg.room]);
  if (room_msg.room == next_door.room1) {
    next_room = &(topology->rooms[next_door.room2]);
  } else {
    next_room = &(topology->rooms[next_door.room1]);
  }
 
  //check if side door or right in front -- is_side_door
  door_angle_diff = fabs(carmen_normalize_theta(next_door.pose.theta - cur_room->theta));
  if (door_angle_diff > M_PI/2.0)
    door_angle_diff = M_PI - door_angle_diff;
  //is_side_door = (door_angle_diff > M_PI/4.0);
  //is_side_door = (door_angle_diff > M_PI/3.5); --> kills Y intersection
  is_side_door = (door_angle_diff > M_PI/3.2);

  //printf("is_side_door: %d\n", is_side_door);
  //printf("door_angle_diff: %f\n", door_angle_diff);
  //printf("check: %f\n", M_PI/3.8);

  //prep for calculating projections
  if(is_side_door) {
    theta_robot_door_loc = atan2(next_door.pose.x - robot_pose.x, 
				 next_door.pose.y - robot_pose.y);
  } else {
    theta_robot_door_loc = atan2(next_door.pose.y - robot_pose.y, 
				 next_door.pose.x - robot_pose.x);
  }


  theta_robot_door = carmen_normalize_theta(robot_pose.theta - next_door.pose.theta);
  //printf("theta_robot_door: %f\n", theta_robot_door);
 
 
  //case-by-case
  switch(cur_room->type) {
  case CARMEN_ROOM_TYPE_ROOM:

    tmp_int = -1;

    // note this doesn't account for the case where the door leads directly into the length
    // of the hallway
    if(next_door.is_real_door) {
      if(path_msg.pathlen == 1) { //not doing anything bout this
	//printf("****breaking because pathlen 1, ROOM****\n");
	break; 
      }
      
      following_door = (topology->doors[path_msg.path[1]]);
             
      tmp_x = following_door.pose.x;
      tmp_y = following_door.pose.y;
      tmp_x -= next_door.pose.x;
      tmp_y -= next_door.pose.y;
      
      tmp_angle = carmen_normalize_theta(next_door.pose.theta - robot_pose.theta);
      normalized_angle = next_door.pose.theta;
      if(fabs(tmp_angle) > (M_PI/2.0)) {
	normalized_angle = carmen_normalize_theta(next_door.pose.theta + M_PI);
      }
      carmen_rotate_2d(&tmp_x, &tmp_y, -normalized_angle);      

      if(tmp_y > 0) {
	tmp_int = CARMEN_ROOMNAV_GUIDANCE_LEFT;
      } else {
	tmp_int = CARMEN_ROOMNAV_GUIDANCE_RIGHT;
      }
    }

    switch(next_room->type) {
    case CARMEN_ROOM_TYPE_ROOM: //ROOM->ROOM
      
      //CASE: if you are 2 metres from a real door and facing it
      if(next_door.is_real_door) {
	//NORMA
	if ((norm_dist_to_door < 3.0) && (dir == CARMEN_ROOMNAV_GUIDANCE_FORWARD)) {
	  //return GO_THROUGH_DOORWAY; 

	  if(tmp_int == CARMEN_ROOMNAV_GUIDANCE_LEFT) {
	    return CARMEN_ROOMNAV_GUIDANCE_TURN_LEFT_AT_DOORWAY;
	  } else {
	    return CARMEN_ROOMNAV_GUIDANCE_TURN_RIGHT_AT_DOORWAY;
	  }
	}
      }
      break;

    case CARMEN_ROOM_TYPE_HALLWAY: //ROOM->HALLWAY
      

      //if you are 3 metres from a real door and facing it
      if(next_door.is_real_door) {
	//NORMA
	if ((norm_dist_to_door < 3.0) && (dir == CARMEN_ROOMNAV_GUIDANCE_FORWARD)) {
	 
	  //return ENTER_HALLWAY; 
	  if(tmp_int == CARMEN_ROOMNAV_GUIDANCE_RIGHT) {
	    //printf("turn right at hallway\n");
	    return CARMEN_ROOMNAV_GUIDANCE_ENTER_HALLWAY_TURN_RIGHT;
	  } else {
	    //printf("turn left at hallway\n");
	    return CARMEN_ROOMNAV_GUIDANCE_ENTER_HALLWAY_TURN_LEFT;
	  }	  
	}
      }
      break;
      
    }
    break;
    

  case CARMEN_ROOM_TYPE_HALLWAY:

    //figure out which of the current_room's endpoints is the one facing the (real) next room
    //know by figuring out which one is nearer to next_door
    if(carmen_distance(&(cur_room->e1), &(next_door.pose)) < 
       carmen_distance(&(cur_room->e2), &(next_door.pose))) {
      cur_room_end = cur_room->e1;
    } else {
      cur_room_end = cur_room->e2;
    }

    //NORMA
    if (norm_dist_to_door > 5.0) 
      is_hallway_bend = 0;
    //return -1;
    
    room_angle_diff = fabs(carmen_normalize_theta(next_room->theta - cur_room->theta));
    
    if (room_angle_diff > M_PI/2.0)
      room_angle_diff = M_PI - room_angle_diff;

    if(!perpendicular(room_angle_diff)) {
      is_perpendicular_bend = 0;
    }
    
    if(room_angle_diff < M_PI/5.0) {
      is_hallway_bend = 0;
    }

    //printf("is_hallway_bend: %d, is_perpendicular_bend: %d\n", is_hallway_bend, is_perpendicular_bend);

    //printf("perpendicular(room_angle_diff): %d\n", perpendicular(room_angle_diff));

    // otherwise, classify as T-intersection, corner, or X-intersection, any kind of turn
    if(is_hallway_bend) { 
  
      door_theta = next_door.pose.theta;
      
      if (is_side_door) {
	room_to_rotate_e1 = cur_room->e1;
	room_to_rotate_e2 = cur_room->e2;	
	if (room_msg.room == next_door.room1)
	  door_theta = carmen_normalize_theta(door_theta + M_PI);
      } else {
	room_to_rotate_e1 = next_room->e1;
	room_to_rotate_e2 = next_room->e2;
	if (room_msg.room == next_door.room2)
	  door_theta = carmen_normalize_theta(door_theta + M_PI);
      }
      
      room_to_rotate_e1.x -= next_door.pose.x;
      room_to_rotate_e1.y -= next_door.pose.y;
      room_to_rotate_e2.x -= next_door.pose.x;
      room_to_rotate_e2.y -= next_door.pose.y;
      // rotate by -theta of next_door
      carmen_rotate_2d(&(room_to_rotate_e1.x), &(room_to_rotate_e1.y), -1 * door_theta);
      carmen_rotate_2d(&(room_to_rotate_e2.x), &(room_to_rotate_e2.y), -1 * door_theta);
      
      // this ensures room_to_rotate_e1.y < room_to_rotate_e2.y
      if (room_to_rotate_e2.y < room_to_rotate_e1.y) {
	tmp = room_to_rotate_e2.y;
	//tmp2 = room_to_rotate_e2.x
	room_to_rotate_e2.y = room_to_rotate_e1.y;
	//room_to_rotate_e2.x = room_to_rotate_e1.x;
	room_to_rotate_e1.y = tmp;
	//room_to_rotate_e1.y = tmp2;
      }
      corner = 0;

      if (room_to_rotate_e1.y > -CORNER_DIST_THRESHOLD) { 
	corner = 1;
      } else if (room_to_rotate_e2.y < CORNER_DIST_THRESHOLD) {
	corner = 2;
      } else { 
	//if not_side_door --> is an intersection (could be T or X). if is_side_door --> 
	// means door to turn is coming up, but on the right or left?
	corner = 3; 
      }

    } //end is_hallway_bend

         
    switch(next_room->type) {
    case CARMEN_ROOM_TYPE_ROOM: //HALLWAY->ROOM
      
      //PROJ
      if(proj_dist_to_door < DOOR_DIST_THRESHOLD) {
	if (is_side_door) {
	  if(next_door.is_real_door) { //CASE: if you are 2 metres from a real door and facing it
	    //NORMA
	    if ((norm_dist_to_door < 2.0) && (dir == CARMEN_ROOMNAV_GUIDANCE_FORWARD)
		&& (parallel(theta_robot_door))) {
	      return CARMEN_ROOMNAV_GUIDANCE_ENTER_ROOM_IN_FRONT; 
	    }
	  }
	  //if not real door
	  tmp_angle = carmen_normalize_theta(cur_room->theta - robot_pose.theta);

	  if(parallel(tmp_angle)) {
	    tmp_x = next_door.pose.x;
	    tmp_y = next_door.pose.y;
	    tmp_x -= cur_room->ux;
	    tmp_y -= cur_room->uy;
	    
	    tmp_x2 = robot_pose.x;
	    tmp_y2 = robot_pose.y;
	    tmp_x2 -= cur_room->ux;
	    tmp_y2 -= cur_room->uy;
	    
	    normalized_angle = cur_room->theta;
	    if(fabs(tmp_angle) > (M_PI/2.0)) {
	      normalized_angle = carmen_normalize_theta(cur_room->theta + M_PI);
	    }
	    carmen_rotate_2d(&tmp_x, &tmp_y, -normalized_angle);
	    carmen_rotate_2d(&tmp_x2, &tmp_y2, -normalized_angle);
	    
	    if(tmp_x2 < tmp_x) {
	      if(tmp_y > 0) {
		return CARMEN_ROOMNAV_GUIDANCE_TURN_NEXT_LEFT;
	      } else {
		return CARMEN_ROOMNAV_GUIDANCE_TURN_NEXT_RIGHT;
	      }
	    }
	    
	  }
	} //end is_side_door
      } 

      //perhaps to end of hallway
      if(dir == CARMEN_ROOMNAV_GUIDANCE_FORWARD) {
	return get_state_decision_general_hallway(&robot_pose, &cur_room_end, &(next_door.pose),
						  theta_robot_door_loc);
      }


      break;
      
    case CARMEN_ROOM_TYPE_HALLWAY: //HALLWAY->HALLWAY

      //NORMA
      if(norm_dist_to_door < END_OF_HALLWAY_THRESHOLD) {

	if(path_msg.pathlen != 1) {
	  following_door = (topology->doors[path_msg.path[1]]);
	  tmp_int = get_state_decision_straight_through(&following_door, cur_room, dir);
	  if(tmp_int != -1) {
	    return tmp_int; //return decision to go straight through
	  }
	}

	//is_hallway_bend
	if(is_hallway_bend) {
	  if (is_side_door) {
	    //if in region of cur_room that is facing next_room, give F,B,R,L 
	    //guidance instead of with reference to corner.
	    if((carmen_distance(&robot_pose, &cur_room_end)) < next_door.width) {
	      break;
	    }
	    
	    //printf("corner: %d\n", corner);
	    //printf("dir: %d\n", dir);
	   
	    if(perpendicular(theta_robot_door) && (dir == CARMEN_ROOMNAV_GUIDANCE_FORWARD)) {
	      if (corner == 1) {
		return CARMEN_ROOMNAV_GUIDANCE_RIGHT_CORNER;
	      } else if (corner == 2) {
		return CARMEN_ROOMNAV_GUIDANCE_LEFT_CORNER;
	      } 
	    } 
	    
	    //T-intersection or X-intersection where door to turn is on the right or left
	    if(corner == 3) {

	      tmp_angle = carmen_normalize_theta(cur_room->theta - robot_pose.theta);
	      //printf("tmp_angle: %f, * 4/pi: %f\n", tmp_angle, tmp_angle * 4.0/M_PI);
	      

	      if(parallel4(tmp_angle)) {
		tmp_x = next_room->ux;
		tmp_y = next_room->uy;
		tmp_x -= cur_room->ux;
		tmp_y -= cur_room->uy;
		
		tmp_x2 = robot_pose.x;
		tmp_y2 = robot_pose.y;
		tmp_x2 -= cur_room->ux;
		tmp_y2 -= cur_room->uy;
		
		normalized_angle = cur_room->theta;
		if(fabs(tmp_angle) > (M_PI/2.0)) {
		  normalized_angle = carmen_normalize_theta(cur_room->theta + M_PI);
		}
		
		carmen_rotate_2d(&tmp_x, &tmp_y, -normalized_angle);
		carmen_rotate_2d(&tmp_x2, &tmp_y2, -normalized_angle);
		tmp_int = round_dir(carmen_normalize_theta(robot_pose.theta - cur_room->theta));
		
		//printf("h->h: tmp_x: %f, tmp_y: %f\n", tmp_x, tmp_y);
		//printf("door_theta: %f\n", door_theta);
		
		//printf("tmp_x2: %f, tmp_x: %f\n", tmp_x2, tmp_x);
		
		if(tmp_x2 < tmp_x) {
		  if(tmp_y > 0) {
		    return CARMEN_ROOMNAV_GUIDANCE_TURN_NEXT_LEFT;
		  } else {
		    return CARMEN_ROOMNAV_GUIDANCE_TURN_NEXT_RIGHT;
		  }
		}
	      } //end parallel



	    } //end corner == 3
	  } else { //not side door



	    //if(parallel(theta_robot_door)) {
	      if (corner == 1) {
		if(is_perpendicular_bend) {
		  if (dir == CARMEN_ROOMNAV_GUIDANCE_FORWARD)
		    return CARMEN_ROOMNAV_GUIDANCE_LEFT_CORNER;
		}
	      } else if (corner == 2) {
		if(is_perpendicular_bend) {
		  if (dir == CARMEN_ROOMNAV_GUIDANCE_FORWARD)
		    return CARMEN_ROOMNAV_GUIDANCE_RIGHT_CORNER;	    
		}
	      } else if(corner == 3) {
		if(path_msg.pathlen == 1) { //not doing anything bout this
		  //printf("****breaking because pathlen 1, HALLWAY****\n");
		  break; 
		}

		following_door = (topology->doors[path_msg.path[1]]);
		
		tmp_angle = carmen_normalize_theta(cur_room->theta - robot_pose.theta);
		
		
		//printf("cur_room theta: %f\n", cur_room->theta);
		//printf("robot pose theta: %f\n", robot_pose.theta);
		//printf("1\n");
		//printf("tmp_angle: %f\n", tmp_angle);
		if(parallel(tmp_angle) && dir != CARMEN_ROOMNAV_GUIDANCE_BACKWARD) {
		  
		  //printf("2\n");

		  tmp_x = following_door.pose.x;
		  tmp_y = following_door.pose.y;
		  tmp_x -= cur_room->ux;
		  tmp_y -= cur_room->uy;

		  normalized_angle = cur_room->theta;
		  if(fabs(tmp_angle) > (M_PI/2.0)) {
		    normalized_angle = carmen_normalize_theta(cur_room->theta + M_PI);
		  }		  
		  
		  carmen_rotate_2d(&tmp_x, &tmp_y, -normalized_angle);
		  
		  //printf("tmp_x: %f, tmp_y: %f\n", tmp_x, tmp_y);
		  //NORMA
		  if (norm_dist_to_door < END_OF_HALLWAY_THRESHOLD) {
		    if(tmp_y > 0) { 
		      return CARMEN_ROOMNAV_GUIDANCE_TURN_LEFT_AT_INTERSECTION;
		    } else {	
		      return CARMEN_ROOMNAV_GUIDANCE_TURN_RIGHT_AT_INTERSECTION;
		    }
		  }
		}
	      } //end corner == 3
	      //} // end parallel
	  } //end not side door
	} //end is_hallway_bend
      } //end_of_hallway_threshold
      
      if(dir == CARMEN_ROOMNAV_GUIDANCE_FORWARD) {
	return get_state_decision_general_hallway(&robot_pose, &cur_room_end, &(next_door.pose), 
						  theta_robot_door_loc);
      }
      
      break;

     
    } //end nextroom->type switch for hallway
    break;
   
  default:
    printf("cur_room is not a room or hallway\n");
    break;
  }
  
  return -1;
}










////////////////////////////////////
// EVENT HANDLERS
////////////////////////////////////

static void print_path(int *p, int plen) {

  int i, r;

  r = room_msg.room;

  printf("path = %d", r);
  for (i = 0; i < plen; i++) {
    r = (topology->doors[p[i]].room1 == r ? topology->doors[p[i]].room2 : topology->doors[p[i]].room1);
    printf(" %d", r);
  }
  printf("\n");
}



/*
  typedef struct {
  int *path;  // array of door #'s
  int pathlen;
  double timestamp;
  char host[10];
  } carmen_roomnav_path_msg;
  
*/
void path_msg_handler()
{
  //double cur_time;

  printf(" --> got path msg\n");
  print_path(path_msg.path, path_msg.pathlen);

  wait_for_path = 0;
  last_room_state = 100;

  if (computing_command) {
    //cur_time = carmen_get_time_ms();
    //if(cur_time < last_goal_time + NEW_GOAL) {
    //   corrupt_command = 0;
    //} else {
    printf("corrupt command, path_msg_handler!\n");
    corrupt_command |= PATH_CORRUPT;
    printf("corrupt_command = %d\n", corrupt_command);
    //}

  }


}



void goal_msg_handler()
{
  //have goal
  goal_set = 1;
  last_goal_time = carmen_get_time_ms();
  
  printf("in goal_msg_handler\n");
 
  //so that robot won't say keep_going on new dest, if roomnav_guidance was left on
  keep_going_mode = 0; 

  new_goal = 1;
  last_dir = CARMEN_ROOMNAV_GUIDANCE_NONE;
}




void room_msg_handler()
{
  //have set room at least once
  room_set = 1;
  printf(" --> got room message\n");
  if (computing_command) {
    printf("corrupt command, room_msg_handler!\n");
    corrupt_command |= ROOM_CORRUPT;
  }

}




void globalpos_msg_handler()
{
  carmen_point_t door_pose, robot_pose;
  double theta_diff, hallway_theta_diff;
  int dir;
  int decision;

  theta_diff = 0.0;

  computing_command = 1;
  corrupt_command = 0;

  //printf("in globalpos_msg_handler\n");
  
  //if goal is not set, quit!
  if(goal_set == 0) {
    //printf("Goal is not set, quitting\n");
    fflush(NULL);
    goto END;
  }
  //if room is not set, quit!
  if(room_set == 0) {
    //printf("Room is not set, quitting\n");
    fflush(NULL);
    goto END;
  }
  
  //printf("goal_msg.goal: %d\n", goal_msg.goal);
  //printf("room_msg.room: %d\n", room_msg.room);
  
  //if we have reached the goal,quit!
  //else get pose of the next door (there should still be at least one)
  if(goal_msg.goal == room_msg.room) {
    goal_set = 0; //unset
    last_dir = CARMEN_ROOMNAV_GUIDANCE_NONE;
    dir = CARMEN_ROOMNAV_GUIDANCE_DESTINATION_REACHED;
    printf("goal == room\n");
  } else {
    //printf("goal != room\n");
    if(path_msg.pathlen == 0) {
      printf("pathlen was 0, returned\n");
      goto END;
    }
    
    door_pose = (topology->doors[path_msg.path[0]]).pose;

    //get robot pose. robot_pose.theta is the direction the robot's facing
    robot_pose = globalpos_msg.globalpos;
    
    dir = get_dir_to_door(&robot_pose, &(topology->doors[path_msg.path[0]]));

    //figure out what state of affairs we're in
    //and apply based on that
    decision = get_state_decision(robot_pose, dir);

    //printf("decision: %d\n", decision);
    
    if(decision != CARMEN_ROOMNAV_GUIDANCE_DESTINATION_REACHED) {
      if (corrupt_command) {
	printf("caught corrupt command...returning\n");
	goto END;
      }
      
      if (wait_for_path) {
	printf(" --> waiting for path, globalpos_msg_handler\n");
	fflush(0);
	goto END;
      }
      
      if(decision != -1) {
	dir = decision;
      }
    } //end decision != "you have arrived"

    //dir = check_guidance(robot_pose, dir);

    theta_diff = get_normalized_robot_to_door(&robot_pose, &topology->doors[path_msg.path[0]]) - robot_pose.theta;
    guidance_msg.dist = carmen_distance(&robot_pose, &door_pose);
    if (topology->rooms[room_msg.room].type == CARMEN_ROOM_TYPE_HALLWAY) {
      //if (guidance_msg.dist < 4.0)
      if (projected_dist_to_door(&robot_pose, &(topology->doors[path_msg.path[0]])) > 3.0) {
	hallway_theta_diff = carmen_normalize_theta(topology->rooms[room_msg.room].theta -
						    atan2(door_pose.y - robot_pose.y, door_pose.x - robot_pose.x));
	if (fabs(hallway_theta_diff) < M_PI/2.0)
	  theta_diff = topology->rooms[room_msg.room].theta - robot_pose.theta;
	else
	  theta_diff = topology->rooms[room_msg.room].theta + M_PI - robot_pose.theta;
      }
    }
    guidance_msg.theta = carmen_normalize_theta(theta_diff);

  } //end else

  // make sure we've moved before we issue a new command
  if(dir != CARMEN_ROOMNAV_GUIDANCE_DESTINATION_REACHED && !new_goal) {
    if (carmen_distance(&globalpos_msg.globalpos, &last_cmd_pose) < 0.5 &&
	fabs(carmen_normalize_theta(globalpos_msg.globalpos.theta - last_cmd_pose.theta)) < M_PI/16.0) {
      publish_guidance_msg();
      goto END;
    }
  }
  
  print_dir_for_user(dir);

  new_goal = 0;

 END:
  computing_command = 0;
}


static void config_handler(MSG_INSTANCE msgRef, BYTE_ARRAY callData,
			   void *clientData __attribute__ ((unused))) {

  carmen_roomnav_guidance_config_msg msg;
  FORMATTER_PTR formatter;
  IPC_RETURN_TYPE err = IPC_OK;

  formatter = IPC_msgInstanceFormatter(msgRef);
  err = IPC_unmarshallData(formatter, callData, &msg,
			   sizeof(carmen_roomnav_guidance_config_msg));
  IPC_freeByteArray(callData);
  carmen_test_ipc_return(err, "Could not unmarshall", IPC_msgInstanceName(msgRef));

  carmen_verbose("config_audio = %d\n", msg.audio);

  config_audio = msg.audio;
  config_text = msg.text;
}


void initialize_printstate_ipc()
{
  IPC_RETURN_TYPE err;

  topology = carmen_roomnav_get_rooms_topology();
  while (topology == NULL) {
    sleep(5);
    topology = carmen_roomnav_get_rooms_topology();
  }
  
  //FOR DEBUG ONLY
  printf("------Printing some facts about the topology------\n");
  printf("number of doors: %d\n", topology->num_doors);
  printf("number of rooms: %d\n", topology->num_rooms);
  printf("----End printing some facts about the topology----\n");
  //END FOR DEBUG ONLY
  
  room_msg.room = carmen_roomnav_get_room();
  if (room_msg.room >= 0)
    room_set = 1;
  goal_msg.goal = carmen_roomnav_get_goal();
  if (goal_msg.goal >= 0)
    goal_set = 1;
  if (goal_set)
    path_msg.pathlen = carmen_roomnav_get_path(&path_msg.path);
  
  carmen_roomnav_subscribe_goal_changed_message
    (&goal_msg, (carmen_handler_t)goal_msg_handler, CARMEN_SUBSCRIBE_LATEST);
  
  carmen_roomnav_subscribe_path_message
    (&path_msg, (carmen_handler_t)path_msg_handler, CARMEN_SUBSCRIBE_LATEST);
  
  carmen_roomnav_subscribe_room_message
    (&room_msg, (carmen_handler_t)room_msg_handler, CARMEN_SUBSCRIBE_LATEST);
  
  carmen_localize_subscribe_globalpos_message(&globalpos_msg, (carmen_handler_t)globalpos_msg_handler,
					      CARMEN_SUBSCRIBE_LATEST);    

  guidance_msg.text = NULL;

  err = IPC_defineMsg(CARMEN_ROOMNAV_GUIDANCE_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROOMNAV_GUIDANCE_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_ROOMNAV_GUIDANCE_MSG_NAME);  

  err = IPC_defineMsg(CARMEN_ROOMNAV_GUIDANCE_CONFIG_MSG_NAME, IPC_VARIABLE_LENGTH, 
		      CARMEN_ROOMNAV_GUIDANCE_CONFIG_MSG_FMT);
  carmen_test_ipc_exit(err, "Could not define", CARMEN_ROOMNAV_GUIDANCE_CONFIG_MSG_NAME);
  
  err = IPC_subscribe(CARMEN_ROOMNAV_GUIDANCE_CONFIG_MSG_NAME, 
		      config_handler, NULL);
  carmen_test_ipc_exit(err, "Could not subcribe", 
		       CARMEN_ROOMNAV_GUIDANCE_CONFIG_MSG_NAME);
  IPC_setMsgQueueLength(CARMEN_ROOMNAV_GUIDANCE_CONFIG_MSG_NAME, 100);
}


int main(int argc, char **argv)
{
  //printf("PI_QUAT: %f\n", PI_QUARTER);
  //printf("PI_3QUAT: %f\n", PI_3QUARTER);
  //printf("- PI_QUAT: %f\n", -1 * PI_QUARTER); 

  if(argc > 1) {
    if(!strcmp(argv[1], "0")) {
      printf("Audio off.\n");
      config_audio = 0;
    } 
  }


  carmen_initialize_ipc(argv[0]);
  carmen_param_check_version(argv[0]);

  carmen_randomize();
  
  if (carmen_map_get_gridmap(&map) < 0) {
    fprintf(stderr, "error getting map from mapserver\n");
    exit(0);
  } 
  
  theta_init(NULL);
  initialize_printstate_ipc();
  
  signal(SIGINT, shutdown_module); 
  Signal(SIGCHLD, sigchld_handler);
  
  IPC_dispatch();
  
  return 0;
}
