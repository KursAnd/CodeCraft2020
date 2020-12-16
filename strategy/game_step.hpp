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


  static std::vector<int> cleaner_lvs;
  std::vector<std::pair<EntityType, int>> cleaner_aims;

  const int MIN_BUILDER_UNITS = 13;

  int m_id = 0;
  int m_resource = 0;
  int m_population_max = 0;
  int m_population_max_future = 0; // include inactive houses
  int m_population_use = 0;

  Vec2Int m_res_pos;  // purpouse for BUILDERs who collect thr resources
  const Vec2Int INCORRECT_VEC2INT = Vec2Int(-1, -1);

  std::unordered_map<int, Entity> m_entity_by_id;
  std::unordered_map<EntityType, std::vector<Entity>> m_entity;
  std::unordered_set<int> ids_was;
  std::vector<std::vector<int>> map;
  std::vector<std::vector<int>> map_id;

  std::unordered_map<EntityType, std::vector<Entity>> m_enemy;


public:
  game_step_t (const PlayerView &_playerView, Action &_result);


  Vec2Int get_place_for (const EntityType type) const;
  bool need_build (const EntityType type) const;
  bool can_build (const EntityType type) const;
  int entity_price (const EntityType type, const int cnt = 1) const;

  int get_count (const EntityType type) const;
  int get_base_count () const;
  Vec2Int get_res_pos () const;
  std::vector<Entity> &get_vector (const EntityType type);
  const std::vector<Entity> &get_vector (const EntityType type) const;
  bool is_busy (const Entity &entity) const;
  bool is_empty_space_for_type (const Vec2Int pos, const EntityType type) const;
  bool is_place_free_or_my (const int x, const int y, const int id) const;
  bool is_place_free (const int x, const int y) const;
  bool is_place_contain (const int x, const int y, const EntityType type) const;
  bool is_place_contain_enemy (const int x, const int y, const EntityType type) const;
  bool is_correct (const Vec2Int pos) const;
  bool is_correct_xy (const int x, const int y) const;

  Vec2Int get_cleaner_aim (const int cleaner_id) const;
  int get_max_distance () const;
  void get_nearest_worker_and_best_pos (const Vec2Int build_pos, const EntityType buildType, const Entity *&entity, Vec2Int &best_pos) const;
  static int get_distance (const Vec2Int pos_a, const Vec2Int pos_b);
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
  void move_army (const EntityType type);
  void turn_on_turrets ();
  void save_builders ();
  void heal_nearest ();
  void send_cleaners ();
};
