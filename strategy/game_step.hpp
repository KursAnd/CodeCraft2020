#pragma once

#include "../MyStrategy.hpp"
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <functional>

class game_step_t
{
private:
  const PlayerView *playerView;
  Action *result;

  
  static std::unordered_set<int> destroyed_pos;
  static std::unordered_map<int, Vec2Int> attack_move_tasks;
  std::vector<Vec2Int> attack_pos;


  bool builders_is_attakers = false;

  const int MIN_BUILDER_UNITS = 15;
  const int MY_BASE_ZONE = 30;
  std::vector<int> enemy_near_base_ids;

  int m_id = 0;
  int m_resource = 0;
  int m_population_max = 0;
  int m_population_max_future = 0; // include inactive houses
  int m_population_use = 0;

  std::unordered_set<int> tree_was;
  Vec2Int m_res_pos;  // purpouse for BUILDERs who collect thr resources
  const Vec2Int INCORRECT_VEC2INT = Vec2Int(-1, -1);

  std::unordered_map<int, Entity> m_entity_by_id;
  std::unordered_map<EntityType, std::vector<Entity>> m_entity;
  std::unordered_set<int> ids_was;
  std::vector<std::vector<int>> map;
  std::vector<std::vector<int>> map_id;
  std::vector<std::vector<int>> map_damage;
  std::vector<std::vector<int>> map_run;

  std::unordered_map<EntityType, std::vector<Entity>> m_enemy;


public:
  game_step_t (const PlayerView &_playerView, Action &_result);


  Vec2Int get_place_for (const EntityType type) const;
  bool need_build (const EntityType type) const;
  bool can_build (const EntityType type) const;
  int entity_price (const EntityType type, const int cnt = 1) const;
  void set_map_damage (const Entity &entity, bool add = true);

  int get_count (const EntityType type) const;
  int get_army_count () const;
  int get_base_count () const;
  Vec2Int get_res_pos () const;
  std::vector<Entity> &get_vector (const EntityType type);
  const std::vector<Entity> &get_vector (const EntityType type) const;
  std::vector<Entity> &get_enemy_vector (const EntityType type);
  const std::vector<Entity> &get_enemy_vector (const EntityType type) const;
  
  bool is_busy (const int id) const;
  bool is_busy (const Entity &entity) const;
  bool is_empty_space_for_type (const Vec2Int pos, const EntityType type) const;
  bool is_place_free_or_my (const int x, const int y, const int id) const;
  bool is_place_free (const Vec2Int pos) const;
  bool is_place_free (const int x, const int y) const;
  bool is_place_contain (const int x, const int y, const EntityType type) const;
  bool is_place_contain_enemy (const int x, const int y, const EntityType type) const;
  bool is_correct (const Vec2Int pos) const;
  bool is_correct_xy (const int x, const int y) const;

  int get_max_distance () const;
  void get_nearest_worker_and_best_pos (const Vec2Int build_pos, const EntityType buildType, const Entity *&entity, Vec2Int &best_pos) const;
  static int get_distance (const Vec2Int pos_a, const Vec2Int pos_b);
  int get_build_distance (const int id /*build*/, const Vec2Int pos) const;
  int get_build_to_build_distance (const Entity &a, const Entity &b) const;
  int get_distance (const Entity &a, const Entity &b) const;
  int get_distance_for_base (const int id_a, const Vec2Int &pos_b, const EntityType type_b, Vec2Int &best_pos) const;
  std::vector<Vec2Int> get_nearest_free_places_for_me (const int id_a, const int id_b) const;
  std::vector<Vec2Int> get_nearest_free_places_for_me (const int id_a, const Vec2Int pos, const EntityType type) const;
  bool find_escape_way_for_workers (const EntityType type, const Vec2Int worker_pos, Vec2Int &safe_pos) const;

  int count_workers_to_repair (const EntityType type) const;
  

  bool buy_entity (const EntityType type, const int cnt = 1);
  void make_busy (const Entity &entity);
  void make_busy (const int id);
  void try_build (const EntityType buildType);
  void train_unit (const EntityType factoryType);
  void check_repair (const EntityType repairType);
  void move_builders ();
  void attack_in_zone ();
  void attack_step_back ();
  void attack_out_zone ();
  void attack_others ();
  void move_army (const EntityType type);
  void turn_on_turrets ();
  void save_builders ();
  void heal_nearest ();


  void run_tasks ();
  void make_atack_groups ();
  void redirect_all_atack_move_tasks (const Vec2Int old_pos, const Vec2Int new_pos);


  int get_attack_pos_id_by_vec (const Vec2Int pos) const;
  int choose_atack_pos (const Vec2Int old_pos = Vec2Int (-1, -1));
};
