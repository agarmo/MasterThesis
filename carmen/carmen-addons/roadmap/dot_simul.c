#include <carmen/carmen.h>
#include <carmen/dot.h>
#include <carmen/dot_messages.h>

static carmen_list_t *person_list = NULL;
static carmen_list_t *trash_list = NULL;

static carmen_simulator_truepos_message truepose;

static carmen_map_t *map;

static int points_are_visible(carmen_traj_point_t *p1, 
			      carmen_traj_point_t *p2,
			      carmen_map_t *c_space)
{
  carmen_bresenham_param_t params;
  int x, y;
  carmen_map_point_t mp1, mp2;

  carmen_trajectory_to_map(p1, &mp1, c_space);
  carmen_trajectory_to_map(p2, &mp2, c_space);

  carmen_get_bresenham_parameters(mp1.x, mp1.y, mp2.x, mp2.y, &params);
  if (mp1.x < 0 || mp1.x >= c_space->config.x_size || 
      mp1.y < 0 || mp1.y >= c_space->config.y_size)
    return 0;

  if (mp2.x < 0 || mp2.x >= c_space->config.x_size || 
      mp2.y < 0 || mp2.y >= c_space->config.y_size)
    return 0;

  do {
    carmen_get_current_point(&params, &x, &y);
    if (c_space->map[x][y] > .01 || c_space->map[x][y] < 0) 
      return 0;
  } while (carmen_get_next_point(&params));

  return 1;
}

static void simulator_objects_handler(carmen_simulator_objects_message 
				      *objects_msg)
{
  carmen_dot_person_t person;
  carmen_dot_trash_t trash;
  carmen_traj_point_t p1;
  carmen_dot_all_people_msg people_msg;
  carmen_dot_all_trash_msg trash_msg;
  int i;
  IPC_RETURN_TYPE err;

  people_msg.timestamp = carmen_get_time_ms();
  strcpy(people_msg.host, carmen_get_tenchar_host_name());

  trash_msg.timestamp = carmen_get_time_ms();
  strcpy(trash_msg.host, carmen_get_tenchar_host_name());

  if (person_list == NULL) 
    person_list = carmen_list_create(sizeof(carmen_dot_person_t), 10);

  if (trash_list == NULL) 
    trash_list = carmen_list_create(sizeof(carmen_dot_trash_t), 10);

  person_list->length = 0;
  trash_list->length = 0;
  p1.x = truepose.truepose.x;
  p1.y = truepose.truepose.y;

  for (i = 0; i < objects_msg->num_objects; i++) {
    if (!points_are_visible(&p1, objects_msg->objects_list+i, map))
      continue;
    if (objects_msg->objects_list[i].t_vel < .2) {
      trash.x = objects_msg->objects_list[i].x;
      trash.y = objects_msg->objects_list[i].y;
      //dbug: need to add convex hull code or won't work!
      trash.vx = 0.375;
      trash.vy = 0.375;
      trash.vxy = 0.125;
      trash.id = i;

      carmen_list_add(trash_list, &trash);

    } else {
      person.x = objects_msg->objects_list[i].x;
      person.y = objects_msg->objects_list[i].y;
      person.vx = 0.375;
      person.vy = 0.375;
      person.vxy = 0.125;
      person.id = i;

      carmen_list_add(person_list, &person);
    }
  }

  if (person_list->length > 0) {
    people_msg.num_people = person_list->length;
    people_msg.people = (carmen_dot_person_t *)person_list->list;
  } else {
    people_msg.num_people = 0;
    people_msg.people = NULL;
  }

  err = IPC_publishData(CARMEN_DOT_ALL_PEOPLE_MSG_NAME, &people_msg);
  carmen_test_ipc_exit(err, "Could not publish", 
		       CARMEN_DOT_ALL_PEOPLE_MSG_NAME);

  if (trash_list->length > 0) {
    trash_msg.num_trash = trash_list->length;
    trash_msg.trash = (carmen_dot_trash_t *)trash_list->list;
  } else {
    trash_msg.num_trash = 0;
    trash_msg.trash = NULL;
  }

  err = IPC_publishData(CARMEN_DOT_ALL_TRASH_MSG_NAME, &trash_msg);
  carmen_test_ipc_exit(err, "Could not publish", 
		       CARMEN_DOT_ALL_TRASH_MSG_NAME);
}

int main(int argc, char *argv[]) 
{
  unsigned int seed;
  
  carmen_initialize_ipc(argv[0]);

  carmen_param_check_version(argv[0]);
  IPC_setVerbosity(IPC_Print_Warnings);  
  
  seed = carmen_randomize(&argc, &argv);
  carmen_warn("Seed: %u\n", seed);
  carmen_simulator_subscribe_truepos_message
    (&truepose, NULL, CARMEN_SUBSCRIBE_LATEST);

  carmen_simulator_subscribe_objects_message
    (NULL, (carmen_handler_t)simulator_objects_handler, 
     CARMEN_SUBSCRIBE_LATEST);

  map = (carmen_map_t *)calloc(1, sizeof(carmen_map_t));
  carmen_test_alloc(map);
  carmen_map_get_gridmap(map);

  IPC_dispatch();

  return 0;
}
