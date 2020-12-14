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

  std::unordered_map<EntityType, std::vector<Vec2Int>> priority_places_for_building;
  std::vector<Vec2Int> attack_pos;

  static std::unordered_set<int> destroyed_pos;
  int choose_atack_pos (const Vec2Int old_pos = Vec2Int (-1, -1));
  int get_id_pos_by_vec (const Vec2Int pos);

  const int MIN_BUILDER_UNITS = 13;

  int m_id = 0;
  int m_resource = 0;
  int m_population_max = 0;
  int m_population_max_future = 0; // include inactive houses
  int m_population_use = 0;

  Vec2Int m_res_pos;  // purpouse for BUILDERs who collect thr resources

  std::unordered_map<int, EntityType> m_entity_type_by_id;
  std::unordered_map<int, Entity> m_entity_by_id;
  std::unordered_map<EntityType, std::unordered_set<int>> m_entity_set;
  std::unordered_map<EntityType, std::vector<Entity>> m_entity;
  std::unordered_set<int> ids_was;
  std::vector<std::vector<int>> map;
  
  std::unordered_map<int, int> repair_ids;
  static std::unordered_map<int, int> repair_tasks;
  static std::unordered_map<int, Vec2Int> attack_move_tasks;

public:
  game_step_t (const PlayerView &_playerView, Action &_result);


  Vec2Int get_place_for (const EntityType type) const;
  bool need_build (const EntityType type) const;
  bool can_build (const EntityType type) const;
  int entity_price (const EntityType type, const int cnt = 1) const;

  int get_count (const EntityType type) const;
  int get_army_count () const;
  int get_base_count () const;
  Vec2Int get_res_pos () const;
  std::vector<Entity> &get_vector (const EntityType type);
  const std::vector<Entity> &get_vector (const EntityType type) const;
  bool is_busy (const Entity &entity) const;
  bool is_empty_space_for_type (const Vec2Int pos, const EntityType type) const;
  bool is_first_building_attaked (const EntityType type) const;
  bool is_place_free_or_worker (const int x, const int y) const;
  bool is_place_free (const int x, const int y) const;
  bool is_place_contain (const int x, const int y, const EntityType type) const;

  int get_max_distance () const;
  static int get_distance (const Vec2Int pos_a, const Vec2Int pos_b);
  int get_distance_for_base (const Vec2Int pos_a, const Vec2Int &pos_b, const EntityType type_b, Vec2Int &best_pos) const;
  bool get_pos_for_safe_operation (const EntityType type, Vec2Int &pos) const;
  std::vector<Vec2Int> get_nearest_free_places (const int id) const;
  std::vector<Vec2Int> get_nearest_free_places (const Vec2Int pos, const EntityType type) const;

  int count_workers_to_repair (const EntityType type) const;
  

  bool buy_entity (const EntityType type, const int cnt = 1);
  void make_busy (const Entity &entity);
  void make_busy (const int id);
  void try_build (const EntityType buildType);
  void train_unit (const EntityType factoryType);
  void check_repair (const EntityType repairType);
  void move_builders ();
  void move_army (const EntityType type);
  void turn_on_turrets ();
  void make_atack_groups ();
  void move_solder (const Entity &entity, const Vec2Int &pos, bool need_add_task = true);

  void run_tasks ();
  void add_repair_task (const int id, const int id_rep);
  void add_move_task (const int id, const Vec2Int id_rep);
  void redirect_all_atack_move_tasks (const Vec2Int old_pos, const Vec2Int new_pos);
};
