
#include <carmen/carmen.h>


typedef struct {
  int room;
} schedule_hmm_obs_t;

typedef struct {
  int activity;
  int time;
} schedule_hmm_state_t;

typedef struct {
  int num_states;
  int num_observations;
  // starting state probabilities are assumed to be given
  float *transition_probs;  // p(a2|a,t)
  float *observation_probs;  // p(o|a,t)
} schedule_hmm_t;

typedef struct {
  int room;
} trajectory_hmm_obs_t;

typedef struct {
  int activity;
  int room;
} trajectory_hmm_state_t;

typedef struct {
  int num_states;
  int num_observations;
  // starting state probabilities are assumed to be given
  float *transition_probs;  // p(r2|a,r)
  float *observation_probs;  // p(o|a,r) - unused
} trajectory_hmm_t;  // really just a markov model for now

typedef struct {
  char *name;
  double timestamp;
} rlog_activity_entry_t;

typedef struct {
  char *name;
  double timestamp;
} rlog_room_entry_t;

typedef struct {
  rlog_activity_entry_t *activities;
  int num_activities;
  rlog_room_entry_t *rooms;
  int num_rooms;
} rlog_t;


#define MAX_NUM_ROOMS 1000
#define MAX_NUM_ACTIVITIES 100
#define MAX_NUM_RLOGS 100

static char *room_names[MAX_NUM_ROOMS];
static int num_rooms = 0;
static char *activity_names[MAX_NUM_ACTIVITIES];
static int num_activities = 0;
static rlog_t rlogs[MAX_NUM_RLOGS];
static int num_rlogs = 0;

#define DEFAULT_TIME_RESOLUTION (1.0/60.0)  // time-slices per second

static double time_resolution = DEFAULT_TIME_RESOLUTION;
static struct tm schedule_start_time;
static struct tm schedule_end_time;

static schedule_hmm_t schedule_hmm;
static trajectory_hmm_t trajectory_hmm;

/*
static int *schedule;
static float *activity_pdfs;
static float *activity_cdfs;
*/

float **activity_trans_probs;
float ***room_trans_probs;
float *activity_tod_means;
float *activity_tod_variances;


static int schedule_hmm_state_index(schedule_hmm_state_t state) {

  return state.activity + state.time*num_activities;
}

/*
static int schedule_hmm_obs_index(schedule_hmm_obs_t obs) {

  return obs.room;
}
*/

/*
 * given: s2.time == s1.time+1
 */
static int schedule_hmm_trans_prob_index(schedule_hmm_state_t s1, schedule_hmm_state_t s2) {
  
  return s1.activity + (s2.activity + s1.time*num_activities)*num_activities;
}

static int schedule_hmm_obs_prob_index(schedule_hmm_state_t s, schedule_hmm_obs_t o) {

  return s.activity + (o.room + s.time*schedule_hmm.num_observations)*num_activities;
}

/*
static float schedule_hmm_trans_prob(schedule_hmm_state_t s1, schedule_hmm_state_t s2) {
  
  return schedule_hmm.transition_probs[schedule_hmm_trans_prob_index(s1, s2)];
}

static float schedule_hmm_obs_prob(schedule_hmm_state_t s, schedule_hmm_obs_t o) {

  return schedule_hmm.observation_probs[schedule_hmm_obs_prob_index(s, o)];
}
*/

static int trajectory_hmm_state_index(trajectory_hmm_state_t state) {

  return state.room + state.activity*num_rooms;
}

/*
 * given: s1.activity == s2.activity
 */
static int trajectory_hmm_trans_prob_index(trajectory_hmm_state_t s1, trajectory_hmm_state_t s2) {
  
  return s2.room + (s1.room + s1.activity*num_rooms)*num_rooms;
}

static int trajectory_hmm_obs_prob_index(trajectory_hmm_state_t s, trajectory_hmm_obs_t o) {

  return o.room + (s.room + s.activity*num_rooms)*num_rooms;
}

static int get_room_num(char *room_name) {

  int i;
  
  for (i = 0; i < num_rooms; i++)
    if (!strcmp(room_names[i], room_name))
      return i;

  return -1;
}

static int get_activity_num(char *activity_name) {

  int i;
  
  for (i = 0; i < num_activities; i++)
    if (!strcmp(activity_names[i], activity_name))
      return i;

  return -1;
}

static void hash_room_names() {

  int i, j, r;
  char *name;

  for (i = 0; i < num_rlogs; i++) {
    for (j = 0; j < rlogs[i].num_rooms; j++) {
      name = rlogs[i].rooms[j].name;
      for (r = 0; r < num_rooms; r++)
	if (!strcmp(room_names[r], name))
	  break;
      if (r == num_rooms) {
	room_names[num_rooms] = calloc(strlen(name)+1, sizeof(char));
	carmen_test_alloc(room_names[num_rooms]);
	strcpy(room_names[num_rooms], name);
	num_rooms++;
      }
    }
  }
}

static void hash_activity_names() {

  int i, j, a;
  char *name;

  for (i = 0; i < num_rlogs; i++) {
    for (j = 0; j < rlogs[i].num_activities; j++) {
      name = rlogs[i].activities[j].name;
      for (a = 0; a < num_activities; a++)
	if (!strcmp(activity_names[a], name))
	  break;
      if (a == num_activities) {
	activity_names[num_activities] = calloc(strlen(name)+1, sizeof(char));
	carmen_test_alloc(activity_names[num_activities]);
	strcpy(activity_names[num_activities], name);
	num_activities++;
      }
    }
  }
}

/*
 *  returns t2 - t1 (in seconds), ignoring anything above hours
 */
static time_t tm_diff_day(struct tm t1, struct tm t2) {

  return 3600*(t2.tm_hour - t1.tm_hour) + 60*(t2.tm_min - t1.tm_min) + (t2.tm_sec - t1.tm_sec);
}

static void print_time_of_day(double timestamp) {

  time_t ts;
  struct tm *t;

  ts = (time_t) timestamp;
  t = localtime(&ts);

  printf("%.2d:%.2d:%.2d", t->tm_hour, t->tm_min, t->tm_sec);
}

static int schedule_time_index(double timestamp) {

  time_t ts;
  struct tm *t;

  ts = (time_t) timestamp;
  t = localtime(&ts);
  
  return tm_diff_day(schedule_start_time, *t) * time_resolution;
}

static float schedule_time_slice_percent(double timestamp) {

  time_t ts;
  struct tm *t;

  ts = (time_t) timestamp;
  t = localtime(&ts);
  
  return (tm_diff_day(schedule_start_time, *t) % (int)(1.0 / time_resolution)) * time_resolution;
}

/*
 * assumes daily schedule
 */
static void schedule_get_time_info() {

  time_t ts;
  struct tm *t;
  int i;

  memset(&schedule_start_time, 0, sizeof(struct tm));
  memset(&schedule_end_time, 0, sizeof(struct tm));

  ts = (time_t) rlogs[0].activities[0].timestamp;
  t = localtime(&ts);
  schedule_start_time.tm_hour = t->tm_hour;
  schedule_start_time.tm_min = t->tm_min;
  schedule_start_time.tm_sec = t->tm_sec;

  ts = (time_t) rlogs[0].activities[rlogs[0].num_activities-1].timestamp;
  t = localtime(&ts);
  schedule_end_time.tm_hour = t->tm_hour;
  schedule_end_time.tm_min = t->tm_min;
  schedule_end_time.tm_sec = t->tm_sec;

  for (i = 0; i < num_rlogs; i++) {
    ts = (time_t) rlogs[i].activities[0].timestamp;
    t = localtime(&ts);
    if (tm_diff_day(*t, schedule_start_time) > 0) {
      schedule_start_time.tm_hour = t->tm_hour;
      schedule_start_time.tm_min = t->tm_min;
      schedule_start_time.tm_sec = t->tm_sec;  
    }
    ts = (time_t) rlogs[i].activities[rlogs[i].num_activities-1].timestamp;
    t = localtime(&ts);
    if (tm_diff_day(*t, schedule_end_time) < 0) {
      schedule_end_time.tm_hour = t->tm_hour;
      schedule_end_time.tm_min = t->tm_min;
      schedule_end_time.tm_sec = t->tm_sec;
    }
  }

  printf("schedule start time: %.2d:%.2d:%.2d\n", schedule_start_time.tm_hour,
	 schedule_start_time.tm_min, schedule_start_time.tm_sec);
  printf("schedule end time: %.2d:%.2d:%.2d\n", schedule_end_time.tm_hour,
	 schedule_end_time.tm_min, schedule_end_time.tm_sec);
}

static void init_schedule_hmm() {

  int num_time_slices;

  num_time_slices = tm_diff_day(schedule_start_time, schedule_end_time) * time_resolution + 1;
  schedule_hmm.num_states = num_activities * num_time_slices;
  schedule_hmm.num_observations = num_rooms;
  schedule_hmm.transition_probs = (float *)
    malloc(num_activities * num_activities * num_time_slices);
  carmen_test_alloc(schedule_hmm.transition_probs);
  schedule_hmm.observation_probs = (float *)
    malloc(schedule_hmm.num_observations * schedule_hmm.num_states);
  carmen_test_alloc(schedule_hmm.observation_probs);
}

/*
static void learn_schedule_hmm() {

  int *state_cnt;
  float *trans_probs, *obs_probs;
  int (*si)(schedule_hmm_state_t);
  int (*tpi)(schedule_hmm_state_t, schedule_hmm_state_t);
  int (*opi)(schedule_hmm_state_t, schedule_hmm_obs_t);
  int (*ti)(double);
  schedule_hmm_state_t s, s1, s2;
  schedule_hmm_obs_t o, o1;
  int i, j, k, a1, a2, r, t, t1, t2;
  float timeslice;

  si = schedule_hmm_state_index;
  tpi = schedule_hmm_trans_prob_index;
  opi = schedule_hmm_obs_prob_index;
  ti = schedule_time_index;

  trans_probs = schedule_hmm.transition_probs;
  obs_probs = schedule_hmm.observation_probs;

  state_cnt = (int *) calloc(schedule_hmm.num_states, sizeof(int));
  carmen_test_alloc(state_cnt);
  
  for (i = 0; i < num_rlogs; i++) {
    r = 0;
    for (j = 0; j < rlogs[i].num_activities; j++) {
      // update transition probs
      a1 = get_activity_num(rlogs[i].activities[j].name);
      t1 = ti(rlogs[i].activities[j].timestamp);
      a2 = t2 = 0;
      if (j < rlogs[i].num_activities - 1) {
	a2 = get_activity_num(rlogs[i].activities[j+1].name);
	t2 = ti(rlogs[i].activities[j+1].timestamp);
      }
      else {
	a2 = a1;
	t2 = schedule_hmm.num_states / num_activities - 1;
      }
      s1.activity = a1;
      for (t = t1; t < t2; t++) {
	s1.time = t;
	s2.time = t+1;
	s2.activity = (t < t2-1 ? a1 : a2);
	trans_probs[tpi(s1,s2)] =
	  (trans_probs[tpi(s1,s2)] * state_cnt[si(s1)] + 1.0) / (float)(state_cnt[si(s1)] + 1);
	s.time = t+1;
	for (k = 0; k < num_activities; k++) {
	  if (k != s2.activity) {
	    s.activity = k;
	    trans_probs[tpi(s1,s)] *= state_cnt[si(s1)] / (float)(state_cnt[si(s1)] + 1);
	  }
	}
      }      
      // update observation probs
      while (ti(rlogs[i].rooms[r+1].timestamp) <= t1)
	r++;
      for (t = t1; t < t2; t++) {
	s1.time = t;
	for (; ti(rlogs[i].rooms[r].timestamp) <= t; r++) {
	  o1.room = get_room_num(rlogs[i].rooms[r].name);
	  timeslice = 1.0;
	  if (ti(rlogs[i].rooms[r+1].timestamp) == t) {
	    if (ti(rlogs[i].rooms[r].timestamp) == t)
	      timeslice = schedule_time_slice_percent(rlogs[i].rooms[r+1].timestamp) -
		schedule_time_slice_percent(rlogs[i].rooms[r].timestamp);
	    else
	      timeslice = schedule_time_slice_percent(rlogs[i].rooms[r+1].timestamp);
	  }
	  else if (ti(rlogs[i].rooms[r].timestamp) == t)
	    timeslice = 1.0 - schedule_time_slice_percent(rlogs[i].rooms[r].timestamp);
	  obs_probs[opi(s1,o1)] = (obs_probs[opi(s1,o1)] * state_cnt[si(s1)] + timeslice) /
	    (float)(state_cnt[si(s1)] + timeslice);
	  for (k = 0; k < schedule_hmm.num_observations; k++) {
	    if (k != o1.room) {
	      o.room = k;
	      obs_probs[opi(s1,o)] *= state_cnt[si(s1)] / (float)(state_cnt[si(s1)] + timeslice);
	    }
	  }
	}
	if (ti(rlogs[i].rooms[r].timestamp) > t + 1)
	  r--;
      }
      state_cnt[si(s1)]++;
    }
  }

  // initialize probs for states we haven't seen
  for (i = 0; i < num_activities; i++) {
    s1.activity = i;
    for (j = 0; j < schedule_hmm.num_states / num_activities; j++) {
      s1.time = j;
      if (state_cnt[si(s1)] == 0) {
	s2.time = s1.time + 1;
	for (k = 0; k < num_activities; k++) {
	  s2.activity = k;
	  trans_probs[tpi(s1,s2)] = 1.0 / (float)num_activities;
	}
	for (k = 0; k < num_rooms; k++) {
	  o.room = k;
	  obs_probs[opi(s1,o)] = 1.0 / (float)num_rooms;
	}
      }
    }
  }
}
*/



/*
static void get_schedule_from_hmm() {

  float *activity_probs;
  int max_prob, max_prob_activity;
  schedule_hmm_state_t s1, s2;
  int i, j;

  activity_probs = calloc(num_activities
}
*/

static void learn_activity_distributions() {

  int i, j, a, *state_cnt;
  time_t ts;
  struct tm t0, *t;
  float *mean, *variance, x;

  memset(&t0, 0, sizeof(struct tm));

  state_cnt = (int *) calloc(num_activities, sizeof(int));
  carmen_test_alloc(state_cnt);
  mean = (float *) calloc(num_activities, sizeof(float));
  carmen_test_alloc(mean);
  variance = (float *) calloc(num_activities, sizeof(float));
  carmen_test_alloc(variance);
  
  for (i = 0; i < num_rlogs; i++) {
    for (j = 0; j < rlogs[i].num_activities; j++) {
      a = get_activity_num(rlogs[i].activities[j].name);
      state_cnt[a]++;
      ts = (time_t) rlogs[i].activities[j].timestamp;
      t = localtime(&ts);
      mean[a] += (float) tm_diff_day(t0, *t);
    }
  }

  for (a = 0; a < num_activities; a++) {
    mean[a] /= (float) state_cnt[a];
    printf("mean[%d] = %f sec = %f min = %f hr\n", a,
	   mean[a], mean[a] / 60.0, mean[a] / 3600.0);
  }

  for (i = 0; i < num_rlogs; i++) {
    for (j = 0; j < rlogs[i].num_activities; j++) {
      a = get_activity_num(rlogs[i].activities[j].name);
      ts = (time_t) rlogs[i].activities[j].timestamp;
      t = localtime(&ts);
      x = (float) tm_diff_day(t0, *t) - mean[a];
      variance[a] += x*x;
    }
  }
  
  for (a = 0; a < num_activities; a++) {
    variance[a] /= (float) state_cnt[a];
    printf("variance[%d] = %f sec^2 = %f min^2 = %f hr^2\n", a,
	   variance[a], variance[a] / (60.0*60.0), variance[a] / (3600.0*3600.0));
  }

 activity_tod_means = mean;
 activity_tod_variances = variance;
}

static void learn_schedule() {

  int *state_cnt, i, j, a1, a2;
  float **trans_prob, c = 0.01;

  schedule_get_time_info();
  //init_schedule_hmm();
  //learn_schedule_hmm();
  //get_schedule_from_hmm();

  printf("\n");
  for (i = 0; i < num_activities; i++)
    printf("activity %d:  %s\n", i, activity_names[i]);

  state_cnt = (int *) calloc(num_activities, sizeof(int));
  carmen_test_alloc(state_cnt);

  trans_prob = (float **) calloc(num_activities, sizeof(float *));
  carmen_test_alloc(trans_prob);
  for (i = 0; i < num_activities; i++) {
    trans_prob[i] = (float *) calloc(num_activities, sizeof(float));
    carmen_test_alloc(trans_prob[i]);
  }

  for (i = 0; i < num_rlogs; i++) {
    for (j = 0; j < rlogs[i].num_activities - 1; j++) {
      a1 = get_activity_num(rlogs[i].activities[j].name);
      //printf("log %d, activity %d = %s\n", i, j, activity_names[a1]);
      state_cnt[a1]++;
      a2 = get_activity_num(rlogs[i].activities[j+1].name);
      trans_prob[a1][a2] += 1.0;
    }
  }

  printf("\n");
  printf("{ ");
  for (a1 = 0; a1 < num_activities; a1++) {
    if (a1 == 0)
      printf("{ ");
    else
      printf("  { ");
    for (a2 = 0; a2 < num_activities; a2++) {
      trans_prob[a1][a2] += c;
      trans_prob[a1][a2] /= (float) state_cnt[a1] + c*num_activities;
      if (a2 == 0)
	printf("%.3f", trans_prob[a1][a2]);
      else
	printf(", %.3f", trans_prob[a1][a2]);
    }
    if (a1 < num_activities - 1)
      printf(" },\n");
    else
      printf("}");
  }
  printf(" }\n");

  activity_trans_probs = trans_prob;
}

static void init_trajectory_hmm() {

  trajectory_hmm.num_states = num_activities * num_rooms;
  trajectory_hmm.num_observations = num_rooms;
  trajectory_hmm.transition_probs = (float *)
    malloc(num_activities * num_rooms * num_rooms);
  carmen_test_alloc(trajectory_hmm.transition_probs);
  trajectory_hmm.observation_probs = (float *)
    malloc(trajectory_hmm.num_observations * trajectory_hmm.num_states);
  carmen_test_alloc(trajectory_hmm.observation_probs);
}

static void learn_trajectory_hmm() {

  int *state_cnt;
  float *trans_probs, *obs_probs;
  int (*si)(trajectory_hmm_state_t);
  int (*tpi)(trajectory_hmm_state_t, trajectory_hmm_state_t);
  int (*opi)(trajectory_hmm_state_t, trajectory_hmm_obs_t);
  trajectory_hmm_state_t s, s1, s2;
  //trajectory_hmm_obs_t o, o1;
  int i, j, k, a, r;
  double t1, t2, max_double;

  si = trajectory_hmm_state_index;
  tpi = trajectory_hmm_trans_prob_index;
  opi = trajectory_hmm_obs_prob_index;

  trans_probs = trajectory_hmm.transition_probs;
  obs_probs = trajectory_hmm.observation_probs;

  state_cnt = (int *) calloc(trajectory_hmm.num_states, sizeof(int));
  carmen_test_alloc(state_cnt);

  max_double = 100000000000.0;  //dbug

  for (i = 0; i < num_rlogs; i++) {
    r = 0;
    for (j = 0; j < rlogs[i].num_activities; j++) {
      // update transition probs
      a = get_activity_num(rlogs[i].activities[j].name);
      printf("log %d, activity %d = %s\n", i, j, activity_names[a]);
      t1 = rlogs[i].activities[j].timestamp;
      t2 = (j < rlogs[i].num_activities - 1 ? rlogs[i].activities[j+1].timestamp : max_double);
      s1.activity = s2.activity = a;
      while (rlogs[i].rooms[r+1].timestamp <= t1)
	r++;
      for (; r+1 < rlogs[i].num_rooms && rlogs[i].rooms[r+1].timestamp < t2; r++) {
	s1.room = get_room_num(rlogs[i].rooms[r].name);
	s2.room = get_room_num(rlogs[i].rooms[r+1].name);
	printf("r = %s, r' = %s\n", room_names[s1.room], room_names[s2.room]);
	trans_probs[tpi(s1,s2)] =
	  (trans_probs[tpi(s1,s2)] * state_cnt[si(s1)] + 1.0) / (float)(state_cnt[si(s1)] + 1);
	s.activity = a;
	for (k = 0; k < num_rooms; k++) {
	  if (k != s2.room) {
	    s.room = k;
	    trans_probs[tpi(s1,s)] *= state_cnt[si(s1)] / (float)(state_cnt[si(s1)] + 1);
	  }
	}
	state_cnt[si(s1)]++;
      }
    }
  }

  // initialize probs for states we haven't seen
  for (i = 0; i < num_activities; i++) {
    s1.activity = i;
    for (j = 0; j < num_rooms; j++) {
      s1.room = j;
      if (state_cnt[si(s1)] == 0) {
	s2.activity = s1.activity;
	for (k = 0; k < num_rooms; k++) {
	  s2.room = k;
	  trans_probs[tpi(s1,s2)] = 1.0 / (float)num_rooms;
	}
	/*
	for (k = 0; k < num_rooms; k++) {
	  o.room = k;
	  obs_probs[opi(s1,o)] = 1.0 / (float)num_rooms;
	}
	*/
      }
    }
  }
}

static void learn_trajectories() {
  
  int **state_cnt, i, j, a, r, r1, r2;
  double t1, t2, max_double;
  float ***trans_prob, c = 0.01;

  max_double = 100000000000.0;  //dbug

  //init_trajectory_hmm();
  //learn_trajectory_hmm();

  printf("\n");
  for (i = 0; i < num_activities; i++)
    printf("activity %d:  %s\n", i, activity_names[i]);

  printf("\n");
  for (i = 0; i < num_rooms; i++)
    printf("room %d:  %s\n", i, room_names[i]);

  state_cnt = (int **) calloc(num_activities, sizeof(int *));
  carmen_test_alloc(state_cnt);
  for (i = 0; i < num_activities; i++) {
    state_cnt[i] = (int *) calloc(num_rooms, sizeof(int));
    carmen_test_alloc(state_cnt);
  }

  trans_prob = (float ***) calloc(num_activities, sizeof(float **));
  carmen_test_alloc(trans_prob);
  for (i = 0; i < num_activities; i++) {
    trans_prob[i] = (float **) calloc(num_rooms, sizeof(float *));
    carmen_test_alloc(trans_prob[i]);
    for (j = 0; j < num_rooms; j++) {
      trans_prob[i][j] = (float *) calloc(num_rooms, sizeof(float));
      carmen_test_alloc(trans_prob[i][j]);
    }
  }

  for (i = 0; i < num_rlogs; i++) {
    r = 0;
    for (j = 0; j < rlogs[i].num_activities; j++) {
      a = get_activity_num(rlogs[i].activities[j].name);
      printf("log %d, activity %d = %s\n", i, j, activity_names[a]);
      t1 = rlogs[i].activities[j].timestamp;
      t2 = (j < rlogs[i].num_activities - 1 ? rlogs[i].activities[j+1].timestamp : max_double);
      while (rlogs[i].rooms[r+1].timestamp < t1)
	r++;
      for (; r+1 < rlogs[i].num_rooms && rlogs[i].rooms[r+1].timestamp <= t2; r++) {
	r1 = get_room_num(rlogs[i].rooms[r].name);
	r2 = get_room_num(rlogs[i].rooms[r+1].name);
	printf("%s -> %s\n", room_names[r1], room_names[r2]);
	trans_prob[a][r1][r2] += 1.0;
	state_cnt[a][r1]++;
      }
    }
  }

  for (a = 0; a < num_activities; a++) {
    printf("\n");
    printf("ACTIVITY %s\n", activity_names[a]);
    printf("{ ");
    for (r1 = 0; r1 < num_rooms; r1++) {
      if (r1 == 0)
	printf("{ ");
      else
	printf("  { ");
      for (r2 = 0; r2 < num_rooms; r2++) {
	trans_prob[a][r1][r2] += c;
	trans_prob[a][r1][r2] /= (float) state_cnt[a][r1] + c*num_rooms;
	if (r2 == 0)
	  printf("%.3f", trans_prob[a][r1][r2]);
	else
	  printf(", %.3f", trans_prob[a][r1][r2]);
      }
      if (r1 < num_rooms - 1)
	printf(" },\n");
      else
	printf("}");
    }
    printf(" }\n");
  }

  room_trans_probs = trans_prob;
}

/*
 * mean = mu, variance = sigma^2
 */
static float nprob(float x, float mean, float variance) {

  return (1.0/sqrt(2.0*M_PI*variance))*exp(-0.5*(x-mean)*(x-mean)/variance);
}

static double tod(double timestamp) {

  time_t ts;
  struct tm t0, *t;

  memset(&t0, 0, sizeof(struct tm));
  ts = (time_t) timestamp;
  t = localtime(&ts);
  return (double)tm_diff_day(t0, *t) + (timestamp - (double)ts);
}

static void track_activity(rlog_t *rlog) {

  //float **activity_trans_probs;
  //float ***room_trans_probs;
  //float *activity_tod_means;
  //float *activity_tod_variances;

  float *ap1, *ap2, n;
  int a, a2, r1, r2, ai, ri;
  double at, rt1, rt2, tmax;
  //double t;

  tmax = 100000000000.0;

  ap1 = malloc(num_activities * sizeof(float));
  carmen_test_alloc(ap1);
  for (a = 0; a < num_activities; a++)
    ap1[a] = 1.0 / (float) num_activities;
  ap2 = malloc(num_activities * sizeof(float));
  carmen_test_alloc(ap2);

  r1 = get_room_num(rlog->rooms[0].name);
  rt1 = rlog->rooms[0].timestamp;

  ai = 0;
  ri = 1;

  //t = rt1 - tod(rt1) + 12.0*3600.0;
  
  while (ai < rlog->num_activities || ri < rlog->num_rooms) {
    at = (ai < rlog->num_activities ? rlog->activities[ai].timestamp : tmax);
    rt2 = (ri < rlog->num_rooms ? rlog->rooms[ri].timestamp : tmax);
    if (at < rt2) {
      n = 0.0;
      for (a2 = 0; a2 < num_activities; a2++) {
	ap2[a2] = 0.0;
	for (a = 0; a < num_activities; a++)
	  ap2[a2] += activity_trans_probs[a][a2] * ap1[a];
	ap2[a2] *= nprob((float)tod(at), activity_tod_means[a2], activity_tod_variances[a2]);
	n += ap2[a2];
      }
      printf("\nACTIVITY CHANGE\n");
      printf("Actual Activity = %d\n", get_activity_num(rlog->activities[ai].name));
      // normalize and print
      printf("Activities Distribution = { ");
      for (a = 0; a < num_activities; a++) {
	ap1[a] = ap2[a] / n;
	if (a == 0)
	  printf("%f", ap1[a]);
	else
	  printf(", %f", ap1[a]);
      }
      printf(" }\n");
      ai++;
    }
    else {
      r2 = get_room_num(rlog->rooms[ri].name);
      n = 0.0;
      for (a = 0; a < num_activities; a++) {
	ap2[a] = ap1[a] * room_trans_probs[a][r1][r2];
	n += ap2[a];
      }
      printf("\nROOM CHANGE\n");
      printf("Actual Activity = %d\n",
	     get_activity_num(rlog->activities[ai-1].name));
      // normalize and print
      printf("Activities Distribution = { ");
      for (a = 0; a < num_activities; a++) {
	ap1[a] = ap2[a] / n;
	if (a == 0)
	  printf("%f", ap1[a]);
	else
	  printf(", %f", ap1[a]);
      }
      printf(" }\n");
      r1 = r2;
      rt1 = rt2;
      ri++;
    }
  }
}

static void rlog_add_room_entry(rlog_t *rlog, char *name, double timestamp) {

  rlog_room_entry_t *re;

  rlog->rooms = (rlog_room_entry_t *)
    realloc(rlog->rooms, (rlog->num_rooms+1) * sizeof(rlog_room_entry_t));
  carmen_test_alloc(rlog->rooms);

  re = &rlog->rooms[rlog->num_rooms];
  re->name = calloc(strlen(name)+1, sizeof(char));
  carmen_test_alloc(re->name);
  strcpy(re->name, name);
  re->timestamp = timestamp;

  rlog->num_rooms++;
}

static void rlog_add_activity_entry(rlog_t *rlog, char *name, double timestamp) {

  rlog_activity_entry_t *ae;

  rlog->activities = (rlog_activity_entry_t *)
    realloc(rlog->activities, (rlog->num_activities+1) * sizeof(rlog_activity_entry_t));
  carmen_test_alloc(rlog->activities);

  ae = &rlog->activities[rlog->num_activities];
  ae->name = calloc(strlen(name)+1, sizeof(char));
  carmen_test_alloc(ae->name);
  strcpy(ae->name, name);
  ae->timestamp = timestamp;

  printf("Adding activity entry:  name = %s, time = ", name);
  print_time_of_day(timestamp);
  printf("\n");

  rlog->num_activities++;
}

static char *next_word(char *str) {

  char *mark = str;

  mark += strcspn(mark, " \t");
  mark += strspn(mark, " \t");

  return mark;
}

static void load_rlog(char *progname, char *filename) {

  FILE *fp;
  char token[100], name[100];
  char line[2000], *mark;
  rlog_t *rlog;
  int n, len;
  double timestamp;

  if (strcmp(carmen_file_extension(filename), ".rlog"))
    carmen_die("usage: %s logfile1.rlog [ ... logfileN.rlog]", progname);

  if (!carmen_file_exists(filename))
    carmen_die("Could not find file %s\n", filename);  

  fp = fopen(filename, "r");

  rlog = &rlogs[num_rlogs];
  memset(rlog, 0, sizeof(rlog_t));

  fgets(line, 2000, fp);
  for (n = 1; !feof(fp); n++) {
    //printf("---> n = %d, line = %s\n", n, line); 
    if (sscanf(line, "%s", token) == 1) {
      if (!strcmp(token, "ROOM")) {
	mark = line + strcspn(line, "\"");
	mark++;
	len = strcspn(mark, "\"");
	strncpy(name, mark, len);
	name[len] = '\0';
	mark += len;
	mark = next_word(mark);
	sscanf(mark, "%lf", &timestamp);
	rlog_add_room_entry(rlog, name, timestamp);
      }
      else if (!strcmp(token, "ACTIVITY")) {
	mark = line + strcspn(line, "\"");
	mark++;
	len = strcspn(mark, "\"");
	strncpy(name, mark, len);
	name[len] = '\0';
	mark += len;
	mark = next_word(mark);
	sscanf(mark, "%lf", &timestamp);
	rlog_add_activity_entry(rlog, name, timestamp);
      }
    }

    fgets(line, 2000, fp);
  }

  num_rlogs++;

  fclose(fp);
}

static void output_sebastian_rlog() {

  int i, j, a, r;
  double t1, t2, max_double;
  time_t ts;
  struct tm t0, *t;

  max_double = 100000000000.0;  //dbug
  memset(&t0, 0, sizeof(struct tm));

  printf("\n------------- SEBASTIAN --------------\n");
  for (i = 0; i < num_rlogs; i++) {
    r = 0;
    for (j = 0; j < rlogs[i].num_activities; j++) {
      a = get_activity_num(rlogs[i].activities[j].name);
      t1 = rlogs[i].activities[j].timestamp;
      t2 = (j < rlogs[i].num_activities - 1 ? rlogs[i].activities[j+1].timestamp : max_double);
      while (rlogs[i].rooms[r+1].timestamp < t1)
	r++;
      for (; r < rlogs[i].num_rooms && rlogs[i].rooms[r].timestamp < t2; r++) {
	ts = (time_t) rlogs[i].rooms[r].timestamp;
	t = localtime(&ts);
	printf("%d %d %.2f %.2f\n", a, get_room_num(rlogs[i].rooms[r].name),
	       rlogs[i].rooms[r].timestamp,
	       (float)tm_diff_day(t0, *t) + (rlogs[i].rooms[r].timestamp - (float)ts));
      }
    }
  }
}

int main(int argc, char *argv[]) {

  int i;

  if (argc < 2)
    carmen_die("usage: %s logfile1.rlog [ ... logfileN.rlog]", argv[0]);

  for (i = 1; i < argc; i++)
    load_rlog(argv[0], argv[i]);

  num_rlogs--;

  hash_room_names();
  hash_activity_names();

  learn_schedule();
  learn_activity_distributions();
  learn_trajectories();
  track_activity(&rlogs[num_rlogs]);

  //output_sebastian_rlog();

  return 0;
}
