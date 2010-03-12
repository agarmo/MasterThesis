#ifndef POMDP_ROADMAP_H
#define POMDP_ROADMAP_H

typedef struct {
  carmen_roadmap_t *roadmap;
  carmen_roadmap_t *roadmap_without_people;  
  carmen_roadmap_t *current_roadmap;
  carmen_map_t *c_space;
  carmen_list_t *path;
  carmen_list_t *nodes;
  double max_t_vel;
  double max_r_vel;
  int goal_id;
} carmen_roadmap_mdp_t;

carmen_roadmap_mdp_t *carmen_roadmap_mdp_initialize(carmen_map_t *map);

int carmen_roadmap_mdp_num_nodes(carmen_roadmap_mdp_t *roadmap);
carmen_roadmap_vertex_t *carmen_roadmap_mdp_get_nodes
(carmen_roadmap_mdp_t *roadmap);
carmen_list_t *carmen_roadmap_mdp_generate_path
(carmen_traj_point_t *robot, carmen_roadmap_mdp_t *graphs);
void carmen_roadmap_mdp_plan(carmen_roadmap_mdp_t *roadmap, 
			     carmen_world_point_t *goal);

carmen_roadmap_vertex_t *carmen_roadmap_mdp_nearest_node
(carmen_world_point_t *robot, carmen_roadmap_mdp_t *roadmap);
carmen_roadmap_vertex_t *carmen_roadmap_mdp_next_node
(carmen_roadmap_vertex_t *node, carmen_roadmap_mdp_t *roadmap);
void carmen_mdp_dynamics_clear_all_blocked(carmen_roadmap_mdp_t *roadmap);

#endif
