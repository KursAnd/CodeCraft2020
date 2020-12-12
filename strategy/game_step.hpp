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
  DebugInterface *debugInterface;
  Action *result;

  static std::unordered_map<EntityType, std::vector<Vec2Int>> init_places_for_building ();


  const int MIN_BUILDER_UNITS = 6;

  int m_id = 0;
  int m_resource = 0;
  int m_population_max = 0;
  int m_population_max_future = 0; // include inactive houses
  int m_population_use = 0;

  Vec2Int m_res_pos;  // purpouse for BUILDERs who collect thr resources
  bool collect_money = false;

  std::unordered_map<int, EntityType> m_entity_type_by_id;
  std::unordered_map<int, Entity> m_entity_by_id;
  std::unordered_map<EntityType, std::unordered_set<int>> m_entity_set;
  std::unordered_map<EntityType, std::vector<Entity>> m_entity;
  std::unordered_set<int> ids_was;
  
  std::unordered_map<int, int> repair_ids;
  static std::unordered_map<int, int> repair_tasks;
  static std::unordered_map<int, Vec2Int> move_tasks;

public:
  game_step_t (const PlayerView &_playerView, DebugInterface *_debugInterface, Action &_result);


  Vec2Int get_place_for (const EntityType type) const;
  bool need_build (const EntityType type) const;
  void check_have_build (const EntityType type);
  int entity_price (const EntityType type, const int cnt = 1) const;

  int get_count (const EntityType type) const;
  int get_army_count () const;
  Vec2Int get_res_pos () const;
  std::vector<Entity> &get_vector (const EntityType type);
  const std::vector<Entity> &get_vector (const EntityType type) const;
  bool is_busy (const Entity &entity) const;

  static int get_distance (const Entity &ent_a, const Entity &ent_b);
  int get_distance (const Vec2Int &pos, const Entity &ent_b, const EntityType type) const;


  bool buy_entity (const EntityType type, const int cnt = 1);
  void make_busy (const Entity &entity);
  void make_busy (const int id);
  void try_build      (const EntityType buildType, Action& result);
  void train_unit (const EntityType factoryType, Action& result);
  void check_repair   (const EntityType repairType, Action& result);
  void move_builders (Action& result);
  void move_army (const EntityType type, Action& result);
  void turn_on_turrets (Action& result);
  void make_atack_groups (Action &result);
  void move_solder (const Entity &entity, const Vec2Int &pos, Action& result);

  void run_tasks ();
  void add_repair_task (const int id, const int id_rep);
  void add_move_task (const int id, const Vec2Int id_rep);
};
