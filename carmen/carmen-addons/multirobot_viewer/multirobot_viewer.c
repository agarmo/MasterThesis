#include <carmen/carmen.h>
#include <GL/glut.h>

#define MAX_ROBOTS     10

#define WINDOW_WIDTH   600
#define WINDOW_HEIGHT  400
#define GRAPHICS_FPS   15.0

int window_width = WINDOW_WIDTH, window_height = WINDOW_HEIGHT;
int graphics_initialized = 0;

typedef struct {
  GLuint map_texture;
  unsigned char *texture_data;
  int map_width, map_height;
  int texture_width, texture_height;
  float u_limit, v_limit;
} map_texture_t, *map_texture_p;

typedef struct {
  char host[100];
  int port;
  IPC_CONTEXT_PTR context;
  float x, y, theta;
  float goal_x, goal_y;
  int goal_set, autonomous;
  int plan_length;
  carmen_traj_point_t *plan;
} robot_info_t, *robot_info_p;

char map_filename[200];
map_texture_t map;
int num_robots = 0, current_robot = 0;
robot_info_t robot_info[MAX_ROBOTS];

carmen_map_t carmen_map;

void take_screenshot(char *basefilename)
{
  static unsigned char *buf = NULL;
  static int image_num = 0;
  char filename[100];
  int i;
  FILE *fp;

  if(buf == NULL) {
    buf = (char *)calloc(window_width * window_height * 3, 1);
    carmen_test_alloc(buf);
  }
  glReadPixels(0, 0, window_width, window_height, GL_RGB, GL_UNSIGNED_BYTE,
	       buf);
  
  sprintf(filename, "%s%05d.ppm", basefilename, image_num);
  fp = fopen(filename, "w");
  if(fp == NULL)
    return;
  fprintf(fp, "P6\n%d %d\n255\n", window_width, window_height);

  for(i = 0; i < window_height; i++)
    fwrite(buf + (window_height - i - 1) * window_width * 3,
	   window_width * 3, 1, fp);
  fclose(fp);
  image_num++;
}

void create_map_texture(carmen_map_p carmen_map, map_texture_p map_texture)
{
  int x, y, mark_in, mark_out;

  map_texture->texture_width = 1;
  while(map_texture->texture_width < carmen_map->config.x_size)
    map_texture->texture_width *= 2;
  map_texture->texture_height = 1;
  while(map_texture->texture_height < carmen_map->config.y_size)
    map_texture->texture_height *= 2;
  map_texture->map_width = carmen_map->config.x_size;
  map_texture->map_height = carmen_map->config.y_size;
  map_texture->texture_data = 
    (unsigned char *)calloc(map_texture->texture_width *
			    map_texture->texture_height * 3, 1);
  carmen_test_alloc(map_texture->texture_data);

  for(x = 0; x < carmen_map->config.x_size; x++)
    for(y = 0; y < carmen_map->config.y_size; y++) {
      mark_in = ((carmen_map->config.y_size - 1 - y) * 
		 carmen_map->config.x_size + x) * 3;
      mark_out = (y * map_texture->texture_width + x) * 3;
      if(carmen_map->map[x][y] != -1) {
	map_texture->texture_data[mark_out] = 
	  255 - carmen_map->map[x][y] * 255;
	map_texture->texture_data[mark_out + 1] = 
	  255 - carmen_map->map[x][y] * 255;
	map_texture->texture_data[mark_out + 2] = 
	  255 - carmen_map->map[x][y] * 255;
      }
      else {
	map_texture->texture_data[mark_out] = 0;
	map_texture->texture_data[mark_out + 1] = 0;
	map_texture->texture_data[mark_out + 2] = 255;
      }
    }
  map_texture->u_limit = carmen_map->config.x_size / 
    (float)map_texture->texture_width;
  map_texture->v_limit = carmen_map->config.y_size / 
    (float)map_texture->texture_height;

  glBindTexture(GL_TEXTURE_2D, map_texture->map_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, 3, map_texture->texture_width, 
	       map_texture->texture_height, 0, GL_RGB, GL_UNSIGNED_BYTE,
	       map_texture->texture_data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glutReshapeWindow(map_texture->map_width, map_texture->map_height);
}

void set_display_mode_2D(int w, int h)
{
  glDisable(GL_DEPTH_TEST);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.0, (GLfloat)w, 0.0, (GLfloat)h);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void reshape(int w, int h)
{
  window_width = w;
  window_height = h;
  glViewport(0, 0, (GLsizei) w, (GLsizei) h);
  set_display_mode_2D(w, h);
}

void keyboard(unsigned char key, int x __attribute__ ((unused)), 
	      int y __attribute__ ((unused)))
{
  int i;

  switch(key) {
  case 27: case 'q': case 'Q':
    exit(0);
    break;
  case '1':
    current_robot = 0;
    break;
  case '2':
    current_robot = 1;
    break;
  case '3':
    current_robot = 2;
    break;
  case '4':
    current_robot = 3;
    break;
  case '5':
    current_robot = 4;
    break;
  case '6':
    current_robot = 5;
    break;
  case '7':
    current_robot = 6;
    break;
  case '8':
    current_robot = 7;
    break;
  case '9':
    current_robot = 8;
    break;
  case 'g':
    IPC_setContext(robot_info[current_robot].context);
    carmen_navigator_go();
    break;
  case ' ':
    for(i = 0; i < num_robots; i++) {
      IPC_setContext(robot_info[i].context);
      carmen_navigator_go();
    }
    break;
  default:
    break;
  }
  if(current_robot > num_robots)
    current_robot = num_robots - 1;
}

void draw_circle(float x, float y, float r, int sides)
{
  float angle;
  int i;

  for(i = 0; i < sides; i++) {
    angle = i / (float)sides * 2 * M_PI;
    glVertex2f(x + r * cos(angle), y + r * sin(angle));
  }
}

void display(void)
{
  int i, j;
  float scale;

  if(!graphics_initialized)
    return;

  /* clear the background */
  glClearColor(0, 0, 1, 1);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  /* everything is in meters */
  glPushMatrix();

  scale = (float)window_width / (float)map.map_width / 
    carmen_map.config.resolution;
  glScalef(scale, scale, 1.0);

  /* draw the map */
  glColor3f(1, 1, 1);
  glEnable(GL_TEXTURE_2D);
  glBegin(GL_POLYGON);
  glTexCoord2f(0, 0);
  glVertex2f(0, 0);
  glTexCoord2f(map.u_limit, 0);
  glVertex2f(map.map_width * carmen_map.config.resolution, 0);
  glTexCoord2f(map.u_limit, map.v_limit);
  glVertex2f(map.map_width * carmen_map.config.resolution, 
	     map.map_height * carmen_map.config.resolution);
  glTexCoord2f(0, map.v_limit);
  glVertex2f(0, map.map_height * carmen_map.config.resolution);
  glEnd();
  glDisable(GL_TEXTURE_2D);
  
  for(i = 0; i < num_robots; i++) {
    if(robot_info[i].goal_set) {
      /* draw the plans */
      glColor3f(0, 0, 1);
      glBegin(GL_LINE_STRIP);
      for(j = 0; j < robot_info[i].plan_length; j++)
	glVertex2f(robot_info[i].plan[j].x,
		   robot_info[i].plan[j].y);
      glEnd();
    }
    
    if(robot_info[i].goal_set) {
      /* draw the navigator goals */
      glColor3f(1, 1, 0);
      glBegin(GL_POLYGON);
      draw_circle(robot_info[i].goal_x, robot_info[i].goal_y, 5 / scale, 20);
      glEnd();
      glColor3f(0, 0, 0);
      glBegin(GL_LINE_LOOP);
      draw_circle(robot_info[i].goal_x, robot_info[i].goal_y, 5 / scale, 20);
      glEnd();
    }

    /* draw the robots */
    glColor3f(1, 0, 0);
    glBegin(GL_POLYGON);
    draw_circle(robot_info[i].x, robot_info[i].y, 5 / scale, 20);
    glEnd();
    glColor3f(0, 0, 0);
    glBegin(GL_LINE_LOOP);
    draw_circle(robot_info[i].x, robot_info[i].y, 5 / scale, 20);
    glEnd();
    glBegin(GL_LINES);
    glVertex2f(robot_info[i].x, robot_info[i].y);
    glVertex2f(robot_info[i].x + 9 / scale * cos(robot_info[i].theta),
	       robot_info[i].y + 9 / scale * sin(robot_info[i].theta));
    glEnd();
  }

  glPopMatrix();
  glutSwapBuffers();
}

void mouse(int button, int state, int x, int y)
{
  float map_x, map_y;
  static float down_x = 0, down_y = 0;
  carmen_point_t truepose, std;

  map_x = x / (float)window_width * carmen_map.config.x_size * 
    carmen_map.config.resolution;
  map_y = (window_height - y - 1) / 
    (float)window_width * carmen_map.config.x_size * 
    carmen_map.config.resolution;

  if(state == GLUT_DOWN) {
    if(button == GLUT_LEFT_BUTTON) {
      IPC_setContext(robot_info[current_robot].context);
      carmen_navigator_set_goal(map_x, map_y);
    }
    else if(button == GLUT_RIGHT_BUTTON) {
      down_x = map_x;
      down_y = map_y;
    }
  }
  else if(state == GLUT_UP) {
    if(button == GLUT_RIGHT_BUTTON) {
      IPC_setContext(robot_info[current_robot].context);
      truepose.x = down_x;
      truepose.y = down_y;
      if(map_x != down_x || map_y != down_y) {
	truepose.theta = atan2(map_y - down_y, map_x - down_x);
	std.x = 0.2; std.y = 0.2; std.theta = 4.0 * M_PI / 180.0;
	carmen_localize_initialize_gaussian_command(truepose, std);
      }
    }
  }
}

void motion(int x, int y)
{
  x = x;
  y = y;
}

void idle(void)
{
  static double last_update = 0;
  double current_time;
  int i;

  for(i = 0; i < num_robots; i++) {
    IPC_setContext(robot_info[i].context);
    sleep_ipc(0.001);
  }
 
  current_time = carmen_get_time_ms();
  if(current_time - last_update > 1.0 / (float)GRAPHICS_FPS) {
    glutPostRedisplay();
    last_update = current_time;
  }
}

void initialize_glut(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
  glutInitWindowSize(window_width, window_height);
  glutInitWindowPosition(10, 10);
  glutCreateWindow(argv[0]);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  
  glShadeModel(GL_SMOOTH);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  graphics_initialized = 1;
}

void localize_handler(carmen_localize_globalpos_message *globalpos)
{
  IPC_CONTEXT_PTR context;
  int i;

  context = IPC_getContext();
  for(i = 0; i < num_robots; i++)
    if(context == robot_info[i].context) {
      robot_info[i].x = globalpos->globalpos.x;
      robot_info[i].y = globalpos->globalpos.y;
      robot_info[i].theta = globalpos->globalpos.theta;
    }
}

void navigator_status_handler(carmen_navigator_status_message *status)
{
  IPC_CONTEXT_PTR context;
  int i;

  context = IPC_getContext();
  for(i = 0; i < num_robots; i++)
    if(context == robot_info[i].context) {
      robot_info[i].goal_x = status->goal.x;
      robot_info[i].goal_y = status->goal.y;
      robot_info[i].goal_set = status->goal_set;
      robot_info[i].autonomous = status->autonomous;
    }
}

void navigator_plan_handler(carmen_navigator_plan_message *plan)
{
  IPC_CONTEXT_PTR context;
  int i;

  context = IPC_getContext();
  for(i = 0; i < num_robots; i++)
    if(context == robot_info[i].context) {
      robot_info[i].plan_length = plan->path_length;
      robot_info[i].plan = 
	(carmen_traj_point_t *)realloc(robot_info[i].plan, 
				       plan->path_length * 
				       sizeof(carmen_traj_point_t));
      memcpy(robot_info[i].plan, plan->path, plan->path_length *
	     sizeof(carmen_traj_point_t));
    }
}

void initialize_ipc_connections(char *progname, int simulation)
{
  int i,j;
  char servername[100];
  IPC_RETURN_TYPE err;

  for(i = 0; i < num_robots; i++) {
    sprintf(servername, "%s:%d", robot_info[i].host, robot_info[i].port);
    fprintf(stderr, "Connecting to central %s... ", servername);
    IPC_setVerbosity(IPC_Silent);
    err = IPC_connectModule(progname, servername);
    carmen_test_ipc_exit(err, "Could not connect to %s\n", servername);
    fprintf(stderr, "done.\n");
    robot_info[i].context = IPC_getContext();

    carmen_localize_subscribe_globalpos_message(NULL, (carmen_handler_t)
						localize_handler,
						CARMEN_SUBSCRIBE_LATEST);

    carmen_navigator_subscribe_status_message(NULL, (carmen_handler_t)
					      navigator_status_handler,
					      CARMEN_SUBSCRIBE_ALL);
    
    carmen_navigator_subscribe_plan_message(NULL, (carmen_handler_t)
					    navigator_plan_handler,
					    CARMEN_SUBSCRIBE_LATEST);

    robot_info[i].x = 0;
    robot_info[i].y = 0;
    robot_info[i].theta = 0;
    robot_info[i].goal_set = 0;
    robot_info[i].autonomous = 0;
    robot_info[i].plan_length = 0;
    robot_info[i].plan = NULL;
  }

  //added by curt for simulation purposes
  //this lets the simulated robots see each other
  //this also assumes that this program is called after the simulators
  //are running, otherwise it won't be able to connect them up
  if(simulation){
    for(i = 0; i < num_robots; i++) {
      IPC_setContext(robot_info[i].context);
      for(j = 0; j < num_robots; j++) {
	if( i != j){
	  sprintf(servername, "%s:%d", robot_info[j].host, robot_info[j].port);
	  carmen_simulator_connect_robots(servername);  
	}
      }
    }
  }
}

void get_map(void)
{
 /* Request map, and wait for it to arrive */
 carmen_warn("Requesting a map from the map server... ");
 if(carmen_map_get_gridmap(&carmen_map) < 0) 
   carmen_die("Could not get a map from the map server.\n");
 else
   carmen_warn("done.\n");
}


void initialize(int argc, char **argv, int simulation)
{
  initialize_glut(argc, argv);
  initialize_ipc_connections(argv[0],simulation);
  get_map();
  create_map_texture(&carmen_map, &map);
}

void read_config_file(char *filename)
{
  FILE *fp;
  int i;

  fp = fopen(filename, "r");
  if(fp == NULL)
    carmen_die("Error: could not open config file %s for reading.\n", filename);
  fscanf(fp, "%d\n", &num_robots);
  for(i = 0; i < num_robots; i++) {
    fscanf(fp, "%s %d\n", robot_info[i].host, &robot_info[i].port);
    fprintf(stderr, "Robot %d: %s:%d\n", i, robot_info[i].host,
	    robot_info[i].port);
  }
}

int main(int argc, char **argv)
{
  char filename[200];
  int simulation=0;
  
  strcpy(filename, "ipc.config");
  if(argc >= 2)
    strcpy(filename, argv[1]);
  if(argc >= 3)
    simulation =1;
  read_config_file(filename);
  initialize(argc, argv, simulation);
  glutMainLoop();
  return 0;
}
