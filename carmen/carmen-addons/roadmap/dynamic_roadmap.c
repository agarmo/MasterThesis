#include <carmen/carmen.h>
#include <limits.h>
#include <assert.h>
#include "roadmap.h"
#include "dynamics.h"

#define MAX_NUM_VERTICES 1500

struct state_struct {
  int id;
  int parent_id;
  double cost;
  double utility;
};

typedef struct state_struct state;
typedef state *state_ptr;

typedef struct {
  state_ptr *data_array;
  int num_elements;
  int queue_size;
} queue_struct, *queue;

static inline queue make_queue(void) 
{
  queue new_queue;
  
  new_queue=(queue)calloc(1, sizeof(queue_struct));
  carmen_test_alloc(new_queue);

  return new_queue;
}

static inline void swap_entries(int x1, int x2, queue the_queue)
{
  state_ptr tmp;
  
  tmp = the_queue->data_array[x1-1];
  the_queue->data_array[x1-1] = the_queue->data_array[x2-1];
  the_queue->data_array[x2-1] = tmp;
}

static inline void fix_priority_queue(queue the_queue)
{
  int left, right;
  int index;
  int smallest;

  index = 1;

  while (index < the_queue->num_elements) 
    {      
      left = 2*index;
      right = 2*index+1;
      
      if (left <= the_queue->num_elements && 
	  the_queue->data_array[left-1]->utility <
	  the_queue->data_array[index-1]->utility)
	smallest = left;
      else
	smallest = index;
      if (right <= the_queue->num_elements && 
	  the_queue->data_array[right-1]->utility <
	  the_queue->data_array[smallest - 1]->utility)
	smallest = right;
      
      if (smallest != index)
	{
	  swap_entries(smallest, index, the_queue);
	  index = smallest;
	}
      else
	break;
    }
}

static inline state_ptr pop_queue(queue the_queue) 
{
  state_ptr return_state;
  
  if (the_queue->num_elements == 0)
    return NULL;
  
  return_state = the_queue->data_array[0];
  
  the_queue->data_array[0] = the_queue->data_array[the_queue->num_elements-1];
  the_queue->num_elements--;
  
  fix_priority_queue(the_queue);
  
  return(return_state);
}

static inline void delete_queue(queue *queue_pointer) 
{
  queue the_queue;
  state_ptr cur_state;
  
  the_queue = *queue_pointer;

  if (the_queue == NULL)
    return;

  while (the_queue->num_elements > 0) 
    {
      cur_state = pop_queue(the_queue);
      free(cur_state);
    }
  
  if (the_queue->queue_size > 0)
    free(the_queue->data_array);
  
  free(the_queue);
  queue_pointer = NULL;
}

static void resize_queue(queue the_queue)
{
  if (the_queue->queue_size == 0) 
    {
      the_queue->data_array=(state_ptr *)calloc(256, sizeof(state_ptr));
      carmen_test_alloc(the_queue->data_array);
      the_queue->queue_size = 256;
      the_queue->num_elements = 0;
      return;
    }

  /* If the queue is full, we had better grow it some. */

  if (the_queue->queue_size < the_queue->num_elements) 
    return ;

  /* Realloc twice as much space */
      
  the_queue->data_array=(state_ptr *)
    realloc(the_queue->data_array, sizeof(state_ptr)*the_queue->queue_size*2);
  carmen_test_alloc(the_queue->data_array);
  
  the_queue->queue_size *= 2;
  memset(the_queue->data_array+the_queue->num_elements, 0, 
	 (the_queue->queue_size - the_queue->num_elements)*sizeof(state_ptr));
}

static inline double get_parent_value(queue the_queue, int index)
{
  int parent_index;

  parent_index = carmen_trunc(index/2)-1;
  return the_queue->data_array[parent_index]->utility;
}

static inline void insert_into_queue(state_ptr new_state, queue the_queue) 
{
  int index;

  if (!the_queue->queue_size || 
      the_queue->queue_size == the_queue->num_elements) 
    resize_queue(the_queue);

  the_queue->data_array[the_queue->num_elements] = new_state;
  the_queue->num_elements++;

  /* Fix up priority queue */

  index = the_queue->num_elements;
  
  while (index > 1 && get_parent_value(the_queue, index) > new_state->utility) 
    {
      swap_entries(carmen_trunc(index/2), index, the_queue);
      index = carmen_trunc(index/2);
    }
}

static void add_node(carmen_list_t *node_list, int x, int y)
{
  carmen_roadmap_vertex_t vertex;

  vertex.id = node_list->length;
  vertex.x = x;
  vertex.y = y;
  vertex.label = -1;
  vertex.utility = FLT_MAX;
  vertex.bad = 0;
  vertex.edges = carmen_list_create(sizeof(carmen_roadmap_edge_t), 10);
  carmen_list_add(node_list, &vertex);
}

static void add_edge(carmen_roadmap_vertex_t *node, 
		     carmen_roadmap_vertex_t *parent_node, double cost)
{
  int i, neighbour_id;
  carmen_roadmap_edge_t edge;
  carmen_roadmap_edge_t *edges;

  assert (cost > 0);

  edges = (carmen_roadmap_edge_t*)(parent_node->edges->list);
  for (i = 0; i < parent_node->edges->length; i++) {
    neighbour_id = edges[i].id;
    if (neighbour_id == node->id) 
      return;
  }

  edge.id = node->id;
  edge.cost = cost;
  edge.blocked = 0;
  carmen_list_add(parent_node->edges, &edge);

  edge.id = parent_node->id;
  carmen_list_add(node->edges, &edge);
}

int carmen_roadmap_is_visible(carmen_roadmap_vertex_t *node, 
			      carmen_world_point_t *position,
			      carmen_map_t *c_space)
{
  carmen_bresenham_param_t params;
  int x, y;
  carmen_map_point_t map_pt;

  carmen_world_to_map(position, &map_pt);

  carmen_get_bresenham_parameters(node->x, node->y, map_pt.x, 
				  map_pt.y, &params);
  if (node->x < 0 || node->x >= c_space->config.x_size || 
      map_pt.y < 0 || map_pt.y >= c_space->config.y_size)
    return 0;

  do {
    carmen_get_current_point(&params, &x, &y);
    if (c_space->map[x][y] > 9e5 || c_space->map[x][y] < 0) 
      return 0;
  } while (carmen_get_next_point(&params));

  return 1;
}

int carmen_roadmap_points_are_visible(carmen_traj_point_t *p1, 
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
    if (c_space->map[x][y] > 9e5 || c_space->map[x][y] < 0) 
      return 0;
  } while (carmen_get_next_point(&params));

  return 1;
}

static double compute_cost(int n1x, int n1y, int n2x, int n2y, 
			   carmen_map_p c_space)
{
  carmen_bresenham_param_t params;
  int x, y;
  double cost;
  double length;

  carmen_get_bresenham_parameters(n1x, n1y, n2x, n2y, &params);
  if (n1x < 0 || n1x >= c_space->config.x_size || 
      n2y < 0 || n2y >= c_space->config.y_size)
    return 1e6;

  cost = 0;
  do {
    carmen_get_current_point(&params, &x, &y);
    if (c_space->map[x][y] > 9e5 || c_space->map[x][y] < 0) {
      return 1e6;
      break;
    }
    cost += c_space->map[x][y];
  } while (carmen_get_next_point(&params));

  //  return hypot(n1x-n2x, n1y-n2y);
  length =  hypot(n1x-n2x, n1y-n2y);
  return cost;
}

double carmen_roadmap_get_cost(carmen_world_point_t *point, 
			       carmen_roadmap_vertex_t *node,
			       carmen_roadmap_t *roadmap)
{
  carmen_map_point_t pt;
  double cost;

  carmen_world_to_map(point, &pt);
  cost = compute_cost(pt.x, pt.y, node->x, node->y, roadmap->c_space);

  if (cost > 9e5)
    return cost;

  if (carmen_dynamics_test_point_for_block(node, point, roadmap->avoid_people))
    return 1e6;

  return cost;
}

static int construct_edges(carmen_roadmap_t *roadmap, int id) 
{
  carmen_roadmap_vertex_t *node_list;
  carmen_roadmap_edge_t *edges;
  int i, j;
  int length;
  double cost = 0;

  node_list = (carmen_roadmap_vertex_t *)(roadmap->nodes->list);

  for (i = 0; i < roadmap->nodes->length; i++) {
    if (i == id)
      continue;
    if (hypot(node_list[i].x - node_list[id].x, 
	      node_list[i].y - node_list[id].y) < 50) {
      cost = 1e6;
      edges = (carmen_roadmap_edge_t *)(node_list[i].edges->list);
      length = node_list[i].edges->length;
      for (j = 0; j < length; j++) {
	if (edges[j].id == id) {
	  cost = edges[j].cost;
	  break;
	}
      }
      if (j == length) {
	cost = compute_cost(node_list[i].x, node_list[i].y, 
			    node_list[id].x, node_list[id].y, 
			    roadmap->c_space);
	if (cost < 9e5)
	  add_edge(node_list+i, node_list+id, cost);
      }
    }    
  }
  
  return (node_list[id].edges->length);
}

void carmen_roadmap_add_node(carmen_roadmap_t *roadmap, int x, int y)
{
  add_node(roadmap->nodes, x, y);
  construct_edges(roadmap, roadmap->nodes->length-1);
}

carmen_roadmap_t *carmen_roadmap_initialize(carmen_map_p new_map)
{
  int x, y, i;
  int **grid, *grid_ptr, **sample_grid;
  int progress;
  int size;
  int growth_counter;
  int x_offset[8] = {1, 1, 0, -1, -1, -1, 0, 1};
  int y_offset[8] = {0, 1, 1, 1, 0, -1, -1, -1};  
  int total;
  int sample;
  int random_value;
  int max_label;
  int num_edges;

  carmen_roadmap_t *roadmap;
  carmen_map_p c_space;
  carmen_list_t *node_list;

  c_space = carmen_map_copy(new_map);

  node_list = carmen_list_create(sizeof(carmen_roadmap_vertex_t), 
				 MAX_NUM_VERTICES);

  size = c_space->config.x_size*c_space->config.y_size;
  grid = (int **)calloc(c_space->config.x_size, sizeof(int *));
  carmen_test_alloc(grid);
  grid[0] = (int *)calloc(size, sizeof(int));
  carmen_test_alloc(grid[0]);
  for (x = 1; x < c_space->config.x_size; x++)
    grid[x] = grid[0] + x*c_space->config.y_size;

  sample_grid = (int **)calloc(c_space->config.x_size, sizeof(int *));
  carmen_test_alloc(sample_grid);
  sample_grid[0] = (int *)calloc(size, sizeof(int));
  carmen_test_alloc(sample_grid);
  for (x = 1; x < c_space->config.x_size; x++)
    sample_grid[x] = sample_grid[0] + x*c_space->config.y_size;

  grid_ptr = grid[0];
  for (i = 0; i < size; i++)
    *(grid_ptr++) = 0;
  for (x = 0; x < c_space->config.x_size; x++)
    for (y = 0; y < c_space->config.y_size; y++) {
      if (x == 0 || y == 0 || x == c_space->config.x_size-1 ||
	  y == c_space->config.y_size-1 || 
	  c_space->map[x][y] > 0.001 || c_space->map[x][y] < 0) {
	grid[x][y] = 1;
	c_space->map[x][y] = 1;
      }
    }
  growth_counter = 1;

  do {
    progress = 0;    
    for (x = 1; x < c_space->config.x_size-1; x++)
      for (y = 1; y < c_space->config.y_size-1; y++) {	
	if (grid[x][y] == growth_counter) {
	  for (i = 0; i < 8; i+=2) {
	    if (grid[x+x_offset[i]][y+y_offset[i]] == 0) {
	      grid[x+x_offset[i]][y+y_offset[i]] = growth_counter+1;
	      c_space->map[x][y] = growth_counter+1;
	    }
	    progress = 1;
	  }
	}
      }
    growth_counter++;
  } while (progress);
  
  total = 0;
  for (x = 0; x < c_space->config.x_size; x++)
    for (y = 0; y < c_space->config.y_size; y++) {      
      if (x == 0 || x == c_space->config.x_size-1 || 
	  y == 0 || y == c_space->config.y_size-1) {
	sample_grid[x][y] = total;
	c_space->map[x][y] = 1e6;
	continue;
      }

      if (grid[x][y] < 6) {
	if (grid[x][y] < 3) 
	  c_space->map[x][y] = 1e6;
	else
	  c_space->map[x][y] = (6-grid[x][y])*100000;
      } else
	c_space->map[x][y] = 1;

      if (grid[x][y] < 6) {
	sample_grid[x][y] = total;
	assert (c_space->map[x][y] > 1);
	continue;
      }
      /* Check to see if this is a local maximum in the Voronoi grid */
      for (i = 0; i < 8; i+=2) {
	if (grid[x+x_offset[i]][y+y_offset[i]] >= grid[x][y])
	  break;
      }
      /* if it is a local maximum in the Voronoi grid, increase its weight */
      if (i >= 8) {
	total += 100;
	sample_grid[x][y] = total;
	continue;
      }
      /* Check to see if this is almost a local maximum in the Voronoi grid */
      for (i = 0; i < 8; i+=2) {
	if (grid[x+x_offset[i]][y+y_offset[i]] >= grid[x][y]+1)
	  break;
      }
      /* If it is almost a local maximum in the Voronoi grid, 
	 increase its weight a little less than before */
      if (i == 8) {
	total += 50;
	sample_grid[x][y] = total;
	continue;
      }
      total++;
      sample_grid[x][y] = total;
      //      if (grid[x][y] < 9) 
      //	c_space->map[x][y] = (9-grid[x][y])*1e3;
      //      else
    }  

  max_label = 0;
  x = 0;
  y = 0;
  for (sample = 0; sample < MAX_NUM_VERTICES; sample++) {
    random_value = (int)carmen_uniform_random(0, total);
    if (random_value < sample_grid[0][size/2]) {
      grid_ptr = sample_grid[0];
      for (i = 0; i < size-1; i++) {
	if (random_value < *(grid_ptr++))
	  break;
      }
    } else {
      grid_ptr = sample_grid[0]+size/2;
      for (i = size/2; i < size-1; i++) {
	if (random_value < *(grid_ptr++))
	  break;
      }
    }
    if (grid[0][i] > 0) {
      x = i / c_space->config.y_size;
      y = i % c_space->config.y_size;
      
      add_node(node_list, x, y);
      grid[0][i] = 0;
    }
  }
  
  add_node(node_list, 450, 140);
  add_node(node_list, 550, 140);

  add_node(node_list, 600, 141);
  add_node(node_list, 550, 141);

  free(grid[0]);
  free(grid);

  free(sample_grid[0]);
  free(sample_grid);

  roadmap = (carmen_roadmap_t *)calloc(1, sizeof(carmen_roadmap_t));
  carmen_test_alloc(roadmap);

  roadmap->nodes = node_list;
  roadmap->goal_id = -1;
  roadmap->c_space = c_space;
  roadmap->path = NULL;
  roadmap->avoid_people = 1;

  carmen_param_set_module("robot");
  if (carmen_param_get_double("max_t_vel",&(roadmap->max_t_vel)) < 0) {
    roadmap->max_t_vel = .8;
    carmen_warn("Could not set robot speed. Setting to default %f\n",
		roadmap->max_t_vel);
  }

  if (carmen_param_get_double("max_r_vel",&(roadmap->max_r_vel)) < 0) {
    roadmap->max_r_vel = .8;
    carmen_warn("Could not set robot speed. Setting to default %f\n",
		roadmap->max_r_vel);
  }

  for (i = 0; i < node_list->length; i++) 
    num_edges = construct_edges(roadmap, i);

  return roadmap;
}

carmen_roadmap_t *carmen_roadmap_copy(carmen_roadmap_t *roadmap)
{
  carmen_roadmap_t *new_roadmap;
  carmen_roadmap_vertex_t *node;
  int num_edges;
  int i;

  new_roadmap = (carmen_roadmap_t *)calloc(1, sizeof(carmen_roadmap_t));
  carmen_test_alloc(new_roadmap);

  *new_roadmap = *roadmap;
  new_roadmap->nodes = carmen_list_duplicate(roadmap->nodes);

  for (i = 0; i < new_roadmap->nodes->length; i++) {
    node = (carmen_roadmap_vertex_t *)carmen_list_get(new_roadmap->nodes, i);
    node->edges = carmen_list_create(sizeof(carmen_roadmap_edge_t), 10);
    num_edges = construct_edges(new_roadmap, i);
  }

  new_roadmap->path = NULL;

  return new_roadmap;
}

static inline state_ptr 
create_state(int id, int parent_id, double cost, double util) 
{
  state_ptr new_ptr = NULL;

  new_ptr = (state_ptr)calloc(1, sizeof(state));
  carmen_test_alloc(new_ptr);

  new_ptr->id = id;
  new_ptr->parent_id = parent_id;
  new_ptr->cost = cost;
  new_ptr->utility = util;

  return new_ptr;
}

static inline void push_state(int id, int parent_id, double cost, 
			      double new_utility, queue state_queue)
{
  state_ptr new_state = create_state(id, parent_id, cost, new_utility);
  insert_into_queue(new_state, state_queue);
}

static inline void lower_cost(int id, int parent_id, double cost, 
			      double new_utility, queue state_queue)
{
  int i;

  for (i = 0; i < state_queue->num_elements; i++) {
    if (state_queue->data_array[i]->id == id) {
      state_queue->data_array[i]->parent_id = parent_id;
      state_queue->data_array[i]->cost = cost;
      state_queue->data_array[i]->utility = new_utility;      
    }
  }

  /* Fix up priority queue */

  i = state_queue->num_elements;
  
  while (i > 1 && get_parent_value(state_queue, i) > new_utility) {
    swap_entries(carmen_trunc(i/2), i, state_queue);
    i = carmen_trunc(i/2);
  }
}

static inline double get_cost(carmen_roadmap_vertex_t *node, 
			      carmen_roadmap_vertex_t *parent_node,
			      carmen_roadmap_t *roadmap)
{
  carmen_roadmap_edge_t *edges;
  int i, neighbour_edge;
  int length;
  double cost = 0;
  double turning_angle, radius, speed, turning_cost;

  edges = (carmen_roadmap_edge_t *)(node->edges->list);
  length = node->edges->length;
  for (i = 0; i < length; i++) {
    if (edges[i].id == parent_node->id) {
      cost = edges[i].cost;
      break;
    }
  }  

  if (0)
    carmen_warn("Cost from %d %d to %d %d is %f\n",
		parent_node->x, parent_node->y, node->x, node->y, cost);

  if (edges[i].blocked)
    return 1e6;

  if (cost > 9e5)
    return edges[i].cost;

  if (carmen_dynamics_test_for_block(node, parent_node, 
				     roadmap->avoid_people)) {
    edges[i].blocked = 1;
    neighbour_edge = 
      carmen_dynamics_find_edge_and_block(parent_node, node->id);

    carmen_dynamics_mark_blocked(node->id, i, parent_node->id, neighbour_edge);

    return 1e6;
  }

  return edges[i].cost;

  if (node->theta > FLT_MAX/2)
    return edges[i].cost;

  turning_angle = atan2(node->y-parent_node->y, node->x-parent_node->x) - 
    node->theta;
  turning_angle = fabs(carmen_normalize_theta(turning_angle));
  if (turning_angle < M_PI/8)
    return edges[i].cost;
    
  radius = 2 * roadmap->c_space->config.resolution * 
    sin(turning_angle/2)/(1-sin(turning_angle/2));
  speed = roadmap->max_r_vel * radius;
  if (speed > roadmap->max_t_vel)
    return edges[i].cost;

  turning_cost = roadmap->max_t_vel / speed * radius*1.5;
  turning_cost /= roadmap->c_space->config.resolution;

  assert (turning_cost >= 0);
  assert (turning_cost < 2000);    

  return edges[i].cost + turning_cost;
}

static inline void 
add_neighbours_to_queue(int id, carmen_roadmap_t *roadmap, double *utility, 
			queue state_queue)
{
  int i, cur_neighbour_id;
  double cur_util, new_util;
  double parent_utility;
  carmen_roadmap_edge_t *edges;
  carmen_roadmap_vertex_t *node, *cur_neighbour;

  parent_utility = utility[id];

  node = (carmen_roadmap_vertex_t *)(roadmap->nodes->list)+id;
  edges = (carmen_roadmap_edge_t *)(node->edges->list);

  for (i = 0; i < node->edges->length; i++) {      
    cur_neighbour_id = edges[i].id;
    cur_neighbour = (carmen_roadmap_vertex_t *)(roadmap->nodes->list)+
      cur_neighbour_id;
    cur_util = utility[cur_neighbour_id];

    new_util = parent_utility + get_cost(node, cur_neighbour, roadmap);

    assert(new_util > 0);
    if (cur_util < 0 || cur_util > new_util) {
      cur_neighbour->theta = atan2(node->y-cur_neighbour->y, node->x-cur_neighbour->x);
      push_state(cur_neighbour_id, id, edges[i].cost, new_util, state_queue);
      utility[cur_neighbour_id] = new_util;
    } /* End of for (Index = 0...) */
  }
}

static void dynamic_program(carmen_roadmap_t *roadmap) 
{
  double *utility, *utility_ptr;
  int index;
  int num_expanded;
  carmen_roadmap_vertex_t *node_list;

  queue state_queue;
  state_ptr current_state;

  node_list = (carmen_roadmap_vertex_t *)(roadmap->nodes->list);

  utility = (double *)calloc(roadmap->nodes->length, sizeof(double));
  carmen_test_alloc(utility);

  utility_ptr = utility;
  for (index = 0; index < roadmap->nodes->length; index++) 
    *(utility_ptr++) = FLT_MAX;

  state_queue = make_queue();

  node_list[roadmap->goal_id].theta = FLT_MAX;

  push_state(roadmap->goal_id, roadmap->goal_id, 0, 0, state_queue);
  utility[roadmap->goal_id] = 0;
  num_expanded = 1;
  
  while ((current_state = pop_queue(state_queue)) != NULL) {
    if (current_state->utility < utility[current_state->id])
      continue;
    num_expanded++;
    add_neighbours_to_queue(current_state->id, roadmap, utility,
			       state_queue);
    free(current_state);
  }

  for (index = 0; index < roadmap->nodes->length; index++)
    node_list[index].utility = utility[index];

  free(utility);

  carmen_warn("Num Expanded %d\n", num_expanded);

  delete_queue(&state_queue);
}


void carmen_roadmap_plan(carmen_roadmap_t *roadmap, carmen_world_point_t *goal)
{
  carmen_map_point_t map_goal;
  carmen_roadmap_vertex_t *node_list;
  int i;

  if (roadmap->nodes->length == 0)
    return;

  carmen_warn("Replan\n");

  carmen_world_to_map(goal, &map_goal);

  if (map_goal.x < 0 || map_goal.x >= goal->map->config.x_size ||
      map_goal.y < 0 || map_goal.y >= goal->map->config.y_size ||
      roadmap->c_space->map[map_goal.x][map_goal.y] > 1)
    return;

  node_list = (carmen_roadmap_vertex_t *)(roadmap->nodes->list);
  for (i = 0; i < roadmap->nodes->length; i++) {
    if (map_goal.x == node_list[i].x && map_goal.y == node_list[i].y)
      break;
  }

  if (i == roadmap->nodes->length) {
    add_node(roadmap->nodes, map_goal.x, map_goal.y);
    roadmap->goal_id = roadmap->nodes->length-1;
    construct_edges(roadmap, roadmap->goal_id);
  } else 
    roadmap->goal_id = i;

  dynamic_program(roadmap);
}

carmen_roadmap_vertex_t *carmen_roadmap_nearest_node
(carmen_world_point_t *point, carmen_roadmap_t *roadmap)
{
  double best_dist = FLT_MAX;
  int closest_node = -1;
  double dist;
  int i;
  carmen_map_point_t pt;
  carmen_roadmap_vertex_t *node_list;

  carmen_world_to_map(point, &pt);

  node_list = (carmen_roadmap_vertex_t *)roadmap->nodes->list;
  for (i = 0; i < roadmap->nodes->length; i++) {
    dist = hypot(node_list[i].x-pt.x, node_list[i].y-pt.y);
    if (dist < best_dist && node_list[i].utility >= 0 &&
	node_list[i].utility < FLT_MAX/2 && 
	carmen_roadmap_is_visible(node_list+i, point, roadmap->c_space) &&
	!carmen_dynamics_test_point_for_block
	(node_list+i, point, roadmap->avoid_people)) {
      best_dist = dist;
      closest_node = i;
    }    
  }  
  
  if (closest_node < 0)
    return NULL;

  return node_list+closest_node;
}

carmen_roadmap_vertex_t *carmen_roadmap_best_node
(carmen_world_point_t *point, carmen_roadmap_t *roadmap)
{
  double best_utility = FLT_MAX;
  int best_node = -1;
  double utility, cost;
  int i;
  carmen_map_point_t pt;
  carmen_roadmap_vertex_t *node_list;

  carmen_world_to_map(point, &pt);
  node_list = (carmen_roadmap_vertex_t *)roadmap->nodes->list;
  for (i = 0; i < roadmap->nodes->length; i++) {
    if (hypot(pt.x-node_list[i].x, pt.y-node_list[i].y) > 50)
      continue;
    cost = carmen_roadmap_get_cost(point, node_list+i, roadmap);
    utility = node_list[i].utility + cost;
    if (utility < best_utility && 
	carmen_roadmap_is_visible(node_list+i, point, roadmap->c_space) &&
	!carmen_dynamics_test_point_for_block
	(node_list+i, point, roadmap->avoid_people)) {
      best_utility = utility;
      best_node = i;
    }    
  }  

  if (best_node < 0)
    return NULL;

  return node_list+best_node;
}

carmen_roadmap_vertex_t *carmen_roadmap_next_node
(carmen_roadmap_vertex_t *node, carmen_roadmap_t *roadmap)
{
  double best_utility = FLT_MAX;
  int best_neighbour = 0;
  double utility;
  int i, neighbour_edge;
  carmen_roadmap_edge_t *edges;
  carmen_roadmap_vertex_t *node_list;

  assert (node->edges->length > 0 && node->utility < FLT_MAX/2);
  if (node->edges->length == 0) 
    return NULL;

  node_list = (carmen_roadmap_vertex_t *)(roadmap->nodes->list);
  edges = (carmen_roadmap_edge_t *)(node->edges->list);

  best_neighbour = -1;
  best_utility = FLT_MAX;
  for (i = 0; i < node->edges->length; i++) {
    if (edges[i].cost > 9e5 || edges[i].blocked)
      continue;
    
    if (carmen_dynamics_test_for_block(node, node_list+edges[i].id,
				       roadmap->avoid_people)) {      
      edges[i].blocked = 1;
      neighbour_edge = 
	carmen_dynamics_find_edge_and_block(node_list+edges[i].id, node->id);

      carmen_dynamics_mark_blocked(node->id, i, edges[i].id, neighbour_edge);

      continue;
    }

    utility = edges[i].cost + node_list[edges[i].id].utility;
    if (utility < best_utility) {
      if (0)
      carmen_warn("%d %d -> %d %d : %f\n", node->x, node->y,
		  node_list[edges[i].id].x, node_list[edges[i].id].y,
		  utility);
      best_utility = utility;
      best_neighbour = i;
    }    
  }

  if (best_neighbour < 0)
    return NULL;
  assert (edges[best_neighbour].cost < 9e5);

  if (best_utility > FLT_MAX/2)
    return NULL;

  assert (!carmen_dynamics_test_for_block(node, node_list+edges[best_neighbour].id,
					  roadmap->avoid_people));
    
  
  if (node_list[edges[best_neighbour].id].utility >= node->utility) {
    carmen_warn("bad utility %d %d : %f -> %d %d %f\n", 
		node->x, node->y, node->utility,
		node_list[edges[best_neighbour].id].x,
		node_list[edges[best_neighbour].id].y,
		node_list[edges[best_neighbour].id].utility);
    return NULL;
  }

  assert (node_list[edges[best_neighbour].id].utility < node->utility);

  assert (edges[best_neighbour].id < roadmap->nodes->length);

  return node_list+edges[best_neighbour].id;
}

static int compute_path_segments(carmen_world_point_t *world_robot,
				 carmen_roadmap_t *road) 
{
  int length;
  carmen_roadmap_vertex_t *n1, *n2;
  carmen_roadmap_vertex_t *node;

  node = carmen_roadmap_best_node(world_robot, road);  
  if (!node || carmen_dynamics_test_node(node, road->avoid_people))
    return 0;

  length = 0;

  n1 = node;
  length = 2;
  while (n1 != NULL && n1->utility > 0) {
    n2 = carmen_roadmap_next_node(n1, road);
    n1 = n2;
    length++;
  }

  if (!n1 || n1->utility > 0) 
    return -1;

  return length;
}

int carmen_roadmap_check_path(carmen_traj_point_t *robot, 
			      carmen_roadmap_t *road, int allow_replan)
{
  carmen_roadmap_vertex_t *goal_node;
  int length;
  carmen_map_point_t map_pt;
  carmen_world_point_t world_robot, goal;

  world_robot.pose.x = robot->x;
  world_robot.pose.y = robot->y;
  world_robot.map = road->c_space;

  goal_node = (carmen_roadmap_vertex_t *)
    carmen_list_get(road->nodes, road->goal_id);

  if (carmen_dynamics_test_node(goal_node, road->avoid_people))
    return 0;

  map_pt.x = goal_node->x;
  map_pt.y = goal_node->y;
  map_pt.map = road->c_space;
  carmen_map_to_world(&map_pt, &goal);

  length = compute_path_segments(&world_robot, road);

  if (allow_replan && length < 0) {
    carmen_roadmap_refine_graph(&world_robot, road);
    length = compute_path_segments(&world_robot, road);
  }

  if (allow_replan && length < 0) {
    carmen_dynamics_clear_all_blocked(road);
    carmen_warn("Goal: %f %f\n", goal.pose.x, goal.pose.y);
    carmen_roadmap_plan(road, &goal);
    length = compute_path_segments(&world_robot, road);
    if (length < 0)
      length = 0;
  }

  return length;
}

void carmen_roadmap_generate_path(carmen_traj_point_t *robot,
				 carmen_roadmap_t *roadmap)
{
  carmen_map_point_t map_node;
  carmen_traj_point_t traj_point;
  carmen_world_point_t world_robot;
  carmen_roadmap_vertex_t *node;

  if (roadmap->path == NULL)
    roadmap->path = carmen_list_create(sizeof(carmen_traj_point_t), 10);  

  world_robot.pose.x = robot->x;
  world_robot.pose.y = robot->y;
  world_robot.map = roadmap->c_space;

  roadmap->path->length = 0;

  traj_point.x = robot->x;
  traj_point.y = robot->y;
  carmen_list_add(roadmap->path, &traj_point);

  node = NULL;
  map_node.map = roadmap->c_space;
  do {
    if (!node)
      node = carmen_roadmap_best_node(&world_robot, roadmap);  
    else
      node = carmen_roadmap_next_node(node, roadmap);
    map_node.x = node->x;
    map_node.y = node->y;
    carmen_map_to_trajectory(&map_node, &traj_point);
    carmen_list_add(roadmap->path, &traj_point);
  } while (node != NULL && node->utility > 0);

  assert(node);

  //  carmen_warn("Path length: %d\n", roadmap->path->length);
}
