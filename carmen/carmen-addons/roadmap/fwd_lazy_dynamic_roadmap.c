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

static void empty_queue(queue the_state_queue) 
{
  state_ptr state;

  while ((state = pop_queue(the_state_queue)) != NULL) { 
    free(state);
  }
}


static void add_node(carmen_list_t *node_list, int x, int y)
{
  carmen_roadmap_vertex_t vertex;

  vertex.id = node_list->length;
  vertex.x = x;
  vertex.y = y;
  vertex.label = -1;
  vertex.utility = 0;
  vertex.bad = 0;
  vertex.edges = carmen_list_create(sizeof(carmen_roadmap_edge_t), 10);
  carmen_list_add(node_list, &vertex);
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
	c_space->map[x][y] = 1;
      }
      if (grid[x][y] < 6) {
	sample_grid[x][y] = total;
	if (grid[x][y] < 4) 
	  c_space->map[x][y] = 1e6;
	continue;
      }
      for (i = 0; i < 8; i+=2) {
	if (grid[x+x_offset[i]][y+y_offset[i]] >= grid[x][y])
	  break;
      }
      if (i >= 8) {
	total += 100;
	sample_grid[x][y] = total;
	continue;
      }
      for (i = 0; i < 8; i+=2) {
	if (grid[x+x_offset[i]][y+y_offset[i]] >= grid[x][y]+1)
	  break;
      }
      if (i == 8) {
	total += 50;
	sample_grid[x][y] = total;
	continue;
      }
      total++;
      sample_grid[x][y] = total;
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
  roadmap->avoid_people = 1;

  return roadmap;
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

static void add_edge(carmen_roadmap_vertex_t *node, 
		     carmen_roadmap_vertex_t *parent_node, double cost)
{
  int i, neighbour_id;
  carmen_roadmap_edge_t edge;
  carmen_roadmap_edge_t *edges;

  edges = (carmen_roadmap_edge_t*)(parent_node->edges->list);
  for (i = 0; i < parent_node->edges->length; i++) {
    neighbour_id = edges[i].id;
    if (neighbour_id == node->id) 
      return;
  }

  edge.id = node->id;
  edge.cost = cost;
  edge.blocked = 0;
  assert (edge.cost > 0);
  carmen_list_add(parent_node->edges, &edge);
}

int carmen_roadmap_is_visible(carmen_roadmap_vertex_t *node, 
			      carmen_world_point_t *position,
			      carmen_roadmap_t *roadmap)
{
  carmen_bresenham_param_t params;
  int x, y;
  carmen_map_point_t map_pt;
  carmen_map_p c_space;

  c_space = roadmap->c_space;
  carmen_world_to_map(position, &map_pt);

  carmen_get_bresenham_parameters(node->x, node->y, map_pt.x, 
				  map_pt.y, &params);
  if (node->x < 0 || node->x >= c_space->config.x_size || 
      map_pt.y < 0 || map_pt.y >= c_space->config.y_size)
    return 0;

  do {
    carmen_get_current_point(&params, &x, &y);
    if (c_space->map[x][y] > 1e5 || c_space->map[x][y] < 0) 
      return 0;
  } while (carmen_get_next_point(&params));

  return 1;
}

static double compute_cost(carmen_roadmap_vertex_t *node, 
			   carmen_roadmap_vertex_t *parent_node,
			   carmen_map_p c_space)
{
  carmen_bresenham_param_t params;
  int x, y;
  double cost;

  carmen_get_bresenham_parameters(node->x, node->y, parent_node->x, 
				  parent_node->y, &params);
  if (node->x < 0 || node->x >= c_space->config.x_size || 
      parent_node->y < 0 || parent_node->y >= c_space->config.y_size)
    return 1e6;

  cost = 0;
  do {
    carmen_get_current_point(&params, &x, &y);
    if (c_space->map[x][y] > 1e5 || c_space->map[x][y] < 0) {
      return 1e6;
      break;
    }
    cost += 1;
  } while (carmen_get_next_point(&params));

  return hypot(node->x-parent_node->x, node->y-parent_node->y);
}

static inline double get_cost(carmen_roadmap_vertex_t *node, 
			      carmen_roadmap_vertex_t *parent_node,
			      carmen_roadmap_t *roadmap)
{
  carmen_roadmap_edge_t *edges;
  int i, j;
  int length;
  double cost = 0;
  carmen_map_p c_space;

  c_space = roadmap->c_space;

  edges = (carmen_roadmap_edge_t *)(parent_node->edges->list);
  length = parent_node->edges->length;
  for (i = 0; i < length; i++) {
    if (edges[i].id == node->id) {
      cost = edges[i].cost;
      break;
    }
  }  

  if (0)
    carmen_warn("Cost from %d %d to %d %d is %f\n",
		parent_node->x, parent_node->y, node->x, node->y, cost);

  if (edges[i].blocked)
    return 1e6;

  if (cost > 1e5)
    return edges[i].cost;

  if (carmen_dynamics_test_for_block(node, parent_node, 
				     roadmap->avoid_people)) {
    edges[i].blocked = 1;
    length = parent_node->edges->length;
    edges = (carmen_roadmap_edge_t *)(parent_node->edges->list);
    for (j = 0; j < length; j++) {
      if (edges[j].id == node->id)
	break;
    }
    if (j < length) {
      edges[j].blocked = 1;
      carmen_dynamics_mark_blocked(node->id, i, parent_node->id, j);
    }
    return 1e6;
  }

  return edges[i].cost;
}

static void construct_edges(carmen_roadmap_t *roadmap, int id) 
{
  carmen_roadmap_vertex_t *node_list;
  carmen_roadmap_edge_t *edges;
  int i, j;
  int length;
  double cost = 0;

  node_list = (carmen_roadmap_vertex_t *)(roadmap->nodes->list);

  assert(node_list[id].edges->length == 0);

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
      if (j == length) 
	cost = compute_cost(node_list+i, node_list+id, roadmap->c_space);
      if (cost < 1e5)
	add_edge(node_list+i, node_list+id, cost);
    }    
  }
    
  assert(node_list[id].edges->length > 0);
}

static void add_neighbours_to_queue(carmen_roadmap_t *roadmap, int id, 
				    int goal_id, double *fwd_utilities,
				    double *open_list, queue state_queue)
{
  int i;
  double new_util;
  double parent_utility;
  double cost, heuristic;
  carmen_roadmap_vertex_t *node_list;
  carmen_roadmap_edge_t *edges;
  int neighbour_id;
  double best_known_answer;
  int best_known_edge;

  node_list = (carmen_roadmap_vertex_t *)(roadmap->nodes->list);
  parent_utility = fwd_utilities[id];
  assert (parent_utility < 1e5);
  if (node_list[id].edges->length == 0) 
    construct_edges(roadmap, id);

  edges = (carmen_roadmap_edge_t *)node_list[id].edges->list;
  assert(node_list[id].edges->length > 0);

  best_known_answer = FLT_MAX;
  best_known_edge = -1;
  for (i = 0; i < node_list[id].edges->length; i++) {   
    neighbour_id = edges[i].id;
    if (node_list[neighbour_id].utility > FLT_MAX/2)
      continue;
    cost = get_cost(node_list+neighbour_id, node_list+id, roadmap);
    new_util = parent_utility+cost+node_list[neighbour_id].utility;
    if (best_known_answer > new_util) {
      best_known_answer = new_util;
      best_known_edge = i;
    }
  }

  for (i = 0; i < node_list[id].edges->length; i++) {   
    neighbour_id = edges[i].id;
    if (open_list[neighbour_id] == -2)
      continue;
    cost = get_cost(node_list+neighbour_id, node_list+id, roadmap);
    if (node_list[neighbour_id].utility < FLT_MAX/2) {
      if (i != best_known_edge)
	continue;
      new_util = parent_utility+cost+node_list[neighbour_id].utility;
    } else {
      heuristic = hypot(node_list[goal_id].x - node_list[neighbour_id].x,
			node_list[goal_id].y - node_list[neighbour_id].y);
      new_util = parent_utility + cost + heuristic;
    }

    if (0 && id == 576)
      carmen_warn("considering %d : %f : %f %f %f : %f \n", neighbour_id,
		  cost, fwd_utilities[neighbour_id], parent_utility, cost,
		  node_list[id].utility);

    if (cost > 1e5)
      continue;
    assert(new_util < 1e5);

    if (open_list[neighbour_id] >= 0) {
      //      assert (open_list[neighbour_id] <= new_util);
      if (open_list[neighbour_id] > new_util) {
	//	carmen_warn("Lowering cost\n");
	lower_cost(neighbour_id, id, cost, new_util, state_queue);
	open_list[neighbour_id] = new_util;
      }
      continue;
    }

    open_list[neighbour_id] = new_util;
    if (0)
      carmen_warn("pushed %d parent %d : %f %f %f\n", neighbour_id, id,
		  cost, fwd_utilities[id], parent_utility+cost);
    push_state(neighbour_id, id, cost, new_util, state_queue);
  } /* End of for (i = 0...) */
}


void carmen_roadmap_plan(carmen_roadmap_t *roadmap, carmen_world_point_t *goal)
{
  double dist, best_dist;
  carmen_map_point_t map_goal;
  int closest_node;
  int i;
  carmen_roadmap_vertex_t *node_list;
  carmen_roadmap_edge_t *edges;

  if (roadmap->nodes->length == 0)
    return;

  carmen_warn("Replan\n");

  carmen_world_to_map(goal, &map_goal);

  if (map_goal.x < 0 || map_goal.x >= goal->map->config.x_size ||
      map_goal.y < 0 || map_goal.y >= goal->map->config.y_size ||
      goal->map->map[map_goal.x][map_goal.y] > .001)
    return;
  
  node_list = (carmen_roadmap_vertex_t *)(roadmap->nodes->list);

  best_dist = FLT_MAX;
  closest_node = 0;
  for (i = 0; i < roadmap->nodes->length; i++) {
    node_list[i].utility = FLT_MAX;
    dist = hypot(node_list[i].x-map_goal.x,node_list[i].y-map_goal.y);
    if (dist < best_dist) {
      best_dist = dist;
      closest_node = i;
    }
  }
	
  if (best_dist >= 1) {
    add_node(roadmap->nodes, map_goal.x, map_goal.y);
    node_list = (carmen_roadmap_vertex_t *)(roadmap->nodes->list);
    roadmap->goal_id = roadmap->nodes->length-1;
    construct_edges(roadmap, roadmap->goal_id);
    edges = (carmen_roadmap_edge_t *)
      node_list[roadmap->goal_id].edges->list;
    for (i = 0; i < node_list[roadmap->goal_id].edges->length; i++) {
      if (node_list[edges[i].id].edges->length) {
	add_edge(node_list+roadmap->goal_id, node_list+edges[i].id,
		 edges[i].cost);
      }
    }
  } else 
    roadmap->goal_id = closest_node;

  for (i = 0; i < roadmap->nodes->length; i++) 
    node_list[i].utility = FLT_MAX;
}

static void search(carmen_roadmap_vertex_t *start_node, 
		   carmen_roadmap_t *roadmap)
{
  int num_expanded;
  state_ptr current_state;
  int i, j;
  double *fwd_utilities, *open_list;
  int *parent_ptrs;
  carmen_roadmap_vertex_t *node_list;
  carmen_roadmap_edge_t *edges;
  double min_neighbour_utility;
  int min_neighbour, neighbour_id;
  int min_edge;
  double best_utility, utility;
  int best_neighbour;
  queue the_state_queue = NULL;

  the_state_queue = make_queue();

  node_list = (carmen_roadmap_vertex_t *)(roadmap->nodes->list);

  fwd_utilities = (double *)calloc(roadmap->nodes->length, sizeof(double));
  carmen_test_alloc(fwd_utilities);
  for (i = 0; i < roadmap->nodes->length; i++)
    fwd_utilities[i] = FLT_MAX;

  open_list = (double *)calloc(roadmap->nodes->length, sizeof(double));
  carmen_test_alloc(open_list);
  for (i = 0; i < roadmap->nodes->length; i++)
    open_list[i] = -1;

  parent_ptrs = (int *)calloc(roadmap->nodes->length, sizeof(int));
  carmen_test_alloc(parent_ptrs);
  for (i = 0; i < roadmap->nodes->length; i++)
    parent_ptrs[i] = -1;

  push_state(start_node->id, start_node->id, 0, 0, the_state_queue);
  fwd_utilities[start_node->id] = 0;

  node_list[roadmap->goal_id].utility = 0;
  num_expanded = 0;

  while ((current_state = pop_queue(the_state_queue)) != NULL) {
    num_expanded++;
    if (0)
    carmen_warn("Popped %d\n", current_state->id);
    fwd_utilities[current_state->id] = 
      fwd_utilities[current_state->parent_id] + current_state->cost;
    parent_ptrs[current_state->id] = current_state->parent_id;
    open_list[current_state->id] = -2;
    assert (fwd_utilities[current_state->id] < 1e5);
    if (current_state->id == roadmap->goal_id) {
      if (node_list[roadmap->goal_id].edges->length == 0)
	construct_edges(roadmap, roadmap->goal_id);
      empty_queue(the_state_queue);
    } else {
      add_neighbours_to_queue(roadmap, current_state->id,
			      roadmap->goal_id, fwd_utilities, open_list,
			      the_state_queue);
    }

    free(current_state);
  }

  carmen_warn("Num Expanded %d\n", num_expanded);

  if (fwd_utilities[roadmap->goal_id] > FLT_MAX/2)
    carmen_warn("PROBLEM\n");
  else if (0) {
    node_list[roadmap->goal_id].utility = 0;
    i = roadmap->goal_id;
    while (i != start_node->id) {
      edges = (carmen_roadmap_edge_t *)(node_list[i].edges->list);
      assert (node_list[i].edges->length > 0);
      min_neighbour_utility = FLT_MAX;      
      min_neighbour = -1;
      min_edge = -1;
      for (j = 0; j < node_list[i].edges->length; j++) {
	if (edges[j].cost > 1e5 || edges[j].blocked)
	  continue;
	neighbour_id = edges[j].id;
	assert (fwd_utilities[neighbour_id] >= 0);
	if (fwd_utilities[neighbour_id] < min_neighbour_utility) {
	  min_neighbour_utility = fwd_utilities[neighbour_id];
	  min_neighbour = neighbour_id;
	  min_edge = j;
	}
      }
      assert (min_neighbour >= 0);
      assert (min_neighbour_utility < fwd_utilities[i]);
      assert(node_list[min_neighbour].utility > FLT_MAX/2 ||
	     node_list[min_neighbour].utility ==
	     node_list[i].utility + edges[min_edge].cost);      

      if (node_list[min_neighbour].utility > FLT_MAX/2) {
	node_list[min_neighbour].utility = node_list[i].utility + 
	  edges[min_edge].cost;

	assert(node_list[i].utility < FLT_MAX/2);
	assert(fwd_utilities[i] < FLT_MAX/2);
	assert(fwd_utilities[min_neighbour] < FLT_MAX/2);
	assert(node_list[min_neighbour].utility < 1e5);
      }
      assert (node_list[i].utility < node_list[min_neighbour].utility);
      //      carmen_warn("%d %d %f %f\n", node_list[i].x, node_list[i].y, node_list[i].utility, FLT_MAX/2);

      i = min_neighbour;
      assert(node_list[i].utility < FLT_MAX/2);
    }
  } else {
    node_list[roadmap->goal_id].utility = 0;
    i = roadmap->goal_id;
    while (i != start_node->id) {
      min_neighbour = parent_ptrs[i];
      assert (min_neighbour >= 0);

      min_neighbour_utility = fwd_utilities[min_neighbour];
      assert (min_neighbour_utility < fwd_utilities[i]);

      edges = (carmen_roadmap_edge_t *)(node_list[i].edges->list);
      assert (node_list[i].edges->length > 0);
      for (min_edge = 0; min_edge < node_list[i].edges->length; min_edge++) {
	if (edges[min_edge].id == min_neighbour)
	  break;
      }
      assert (min_edge < node_list[i].edges->length);
      assert(node_list[min_neighbour].utility > FLT_MAX/2 ||
	     fabs(node_list[min_neighbour].utility  - 
		  (node_list[i].utility + edges[min_edge].cost)) < 1e6);      

      if (node_list[min_neighbour].utility > FLT_MAX/2) {
	carmen_roadmap_edge_t *edges2;
	edges2 = (carmen_roadmap_edge_t *)node_list[min_neighbour].edges->list;
	node_list[min_neighbour].utility = node_list[i].utility + 
	  edges[min_edge].cost;
	best_neighbour = -1;
	best_utility = FLT_MAX;
	for (j = 0; j < node_list[min_neighbour].edges->length; j++) {
	  if (edges2[j].cost > 1e5 || edges2[j].blocked)
	    continue;      
	  if (node_list[edges2[j].id].utility > FLT_MAX/2)
	    continue;
	  utility = node_list[edges2[j].id].utility;
	  if (utility < best_utility) {
	    best_utility = utility;
	    best_neighbour = edges2[j].id;
	  }
	}
	
	if (best_neighbour < 0)
	  continue;
	assert (best_utility < FLT_MAX/2);
	assert (node_list[best_neighbour].utility < 
		node_list[min_neighbour].utility);

	assert(node_list[i].utility < FLT_MAX/2);
	assert(fwd_utilities[i] < FLT_MAX/2);
	assert(fwd_utilities[min_neighbour] < FLT_MAX/2);
	assert(node_list[min_neighbour].utility < 1e5);
      }
      assert (node_list[i].utility < node_list[min_neighbour].utility);
      //      carmen_warn("%d %d %f %f\n", node_list[i].x, node_list[i].y, node_list[i].utility, FLT_MAX/2);

      i = min_neighbour;
      assert(node_list[i].utility < FLT_MAX/2);
    }
  }

  /* Check to make sure that no node is a local minimum. The only node that's
     allowed to be a local minimum is the goal.
   */
  for (i = 0; i < roadmap->nodes->length; i++) {
    if (i == roadmap->goal_id)
      continue;
    edges = (carmen_roadmap_edge_t *)(node_list[i].edges->list);

    if (node_list[i].utility > FLT_MAX/2)
      continue;

    best_neighbour = -1;
    best_utility = FLT_MAX;
    for (j = 0; j < node_list[i].edges->length; j++) {
      if (edges[j].cost > 1e5 || edges[j].blocked)
	continue;      
      if (node_list[edges[j].id].utility > FLT_MAX/2)
	continue;
      utility = node_list[edges[j].id].utility;
      if (utility < best_utility) {
	best_utility = utility;
	best_neighbour = edges[j].id;
      }
    }

    if (best_neighbour < 0)
      continue;
    assert (best_utility < FLT_MAX/2);
    assert (node_list[best_neighbour].utility < node_list[i].utility);
  }

  free(fwd_utilities);
  delete_queue(&the_state_queue);
}

carmen_roadmap_vertex_t *carmen_roadmap_nearest_node
(carmen_world_point_t *point, carmen_roadmap_t *roadmap)
{
  double best_dist = FLT_MAX;
  int closest_node = 0;
  double dist;
  int i;
  carmen_map_point_t pt;
  carmen_roadmap_vertex_t *node_list;

  carmen_world_to_map(point, &pt);
  node_list = (carmen_roadmap_vertex_t *)roadmap->nodes->list;
  for (i = 0; i < roadmap->nodes->length; i++) {
    dist = hypot(node_list[i].x-pt.x, node_list[i].y-pt.y);
    if (dist < best_dist) {
      best_dist = dist;
      closest_node = i;
    }    
  }  

  return node_list+closest_node;
}

carmen_roadmap_vertex_t *carmen_roadmap_next_node
(carmen_roadmap_vertex_t *node, carmen_roadmap_t *roadmap)
{
  double best_utility = FLT_MAX;
  int best_neighbour = 0;
  double utility;
  int i, j;
  carmen_roadmap_edge_t *edges, *neighbour_edges;
  carmen_roadmap_vertex_t *node_list;
  int length;

  if (node->edges->length == 0 || node->utility > FLT_MAX/2) 
    search(node, roadmap);
  if (node->edges->length == 0) 
    return NULL;

  node_list = (carmen_roadmap_vertex_t *)(roadmap->nodes->list);
  edges = (carmen_roadmap_edge_t *)(node->edges->list);

  best_neighbour = -1;
  best_utility = FLT_MAX;
  for (i = 0; i < node->edges->length; i++) {
    if (edges[i].cost > 1e5 || edges[i].blocked)
      continue;
    
    if (carmen_dynamics_test_for_block(node, node_list+edges[i].id,
				       roadmap->avoid_people)) {
      edges[i].blocked = 1;
      length = node_list[edges[i].id].edges->length;
      neighbour_edges = (carmen_roadmap_edge_t *)
	node_list[edges[i].id].edges->list;
      for (j = 0; j < length; j++) {
	if (neighbour_edges[j].id == node->id)
	  break;
      }
      assert (j < length);
      neighbour_edges[j].blocked = 1;
      carmen_dynamics_mark_blocked(node->id, i, edges[i].id, j);

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
  assert (edges[best_neighbour].cost < 1e5);

  if (best_utility > FLT_MAX/2)
    return NULL;
  
  if (node_list[edges[best_neighbour].id].utility >= node->utility) {
    carmen_warn("bad utility %d %d : %f -> %d %d %f\n", 
		node->x, node->y, node->utility,
		node_list[edges[best_neighbour].id].x,
		node_list[edges[best_neighbour].id].y,
		node_list[edges[best_neighbour].id].utility);
    return NULL;
  }

  assert (node_list[edges[best_neighbour].id].utility < node->utility);

  return node_list+edges[best_neighbour].id;
}
