#pragma once

#include "../MyStrategy.hpp"
#include <unordered_set>
#include <unordered_map>
#include <set>
#include <functional>

enum class game_type_t
{
  FOURxONE,
  FOURxONEplus,
  TWOxONE
};

class game_step_t
{
private:
  const PlayerView *playerView;
  Action *result;

  
  static std::unordered_set<int> destroyed_pos;
  static std::unordered_map<int, Vec2Int> attack_move_tasks;
  std::vector<Vec2Int> attack_pos;


  bool builders_is_attakers = false;

  const int MIN_BUILDER_UNITS = 30;
  const int MIN_RANGED_UNITS = 20;
  const int MIN_BUILDER_UNITS_2 = 50;
  const int MAX_LINES_OF_HOUSES = 10;
  const int MAX_POSITION_OF_RANGED_BASE = 17;
  const int MIN_POSITION_RANGED_DO_STEP_BACK = 20;
  const int ATTACK_OTHERS_OUT_ZONE_DISTANCE = 20;
  const int BASE_PROTECT_DISTANCE = 20;

  std::vector<int> enemy_near_base_ids;
  int base_zone_x = 0, base_zone_y = 0;

  int m_id = 0;
  int m_resource = 0;
  int m_population_max = 0;
  int m_population_max_future = 0; // include inactive houses
  int m_population_use = 0;

  const Vec2Int INCORRECT_VEC2INT = Vec2Int(-1, -1);

  struct find_id_t
  {
    bool is_enemy = false;
    EntityType type;
    int it = 0;
    find_id_t () {}
    find_id_t (bool is_enemy, EntityType type, int it)
      : is_enemy (is_enemy), type (type), it (it)
    {}
  };
  std::unordered_map<int, find_id_t> m_entity_by_id;
  std::unordered_map<EntityType, std::vector<Entity>> m_entity;
  std::unordered_set<int> ids_was;
  std::vector<std::vector<int>> map;
  std::vector<std::vector<int>> map_id;
  std::vector<std::vector<int>> map_damage;
  std::vector<std::vector<int>> map_damage_melee; // TO-DO: think about all types
  std::vector<std::vector<int>> map_run;
  std::vector<std::vector<int>> map_run_melee;

  std::unordered_map<EntityType, std::vector<Entity>> m_enemy;


  void calculate_enemies_near_base ();
  void recalculate_map_run ();

public:
  game_step_t (const PlayerView &_playerView, Action &_result);


  Vec2Int get_place_for (const EntityType type) const;
  bool need_build (const EntityType type) const;
  bool can_build (const EntityType type) const;
  int entity_price (const EntityType type, const int cnt = 1) const;
  void set_map_damage (const Entity &entity, bool add = true);

  Entity &get_entity_by_id (const int id);
  const Entity &get_entity_by_id (const int id) const;
  int get_count (const EntityType type) const;
  int get_army_count () const;
  int get_base_count () const;
  Vec2Int get_step_back_for_entity (const Entity &entity, const int level) const;
  std::vector<Entity> &get_vector (const EntityType type);
  const std::vector<Entity> &get_vector (const EntityType type) const;
  std::vector<Entity> &get_enemy_vector (const EntityType type);
  const std::vector<Entity> &get_enemy_vector (const EntityType type) const;
  game_type_t get_game_type ();

  bool is_busy (const int id) const;
  bool is_busy (const Entity &entity) const;
  bool is_empty_space_for_type (const Vec2Int pos, const EntityType type) const;
  bool is_place_free_or_my (const int x, const int y, const int id) const;
  bool is_place_free (const Vec2Int pos) const;
  bool is_place_free (const int x, const int y) const;
  bool is_place_contain (const int x, const int y, const EntityType type) const;
  bool is_place_contain_enemy (const int x, const int y, const EntityType type) const;
  bool is_place_contain_us (const int x, const int y) const;
  bool is_place_contain_enemy (const int x, const int y) const;
  bool is_correct (const Vec2Int pos) const;
  bool is_correct_xy (const int x, const int y) const;
  bool is_building (const EntityType type) const;

  int get_max_distance () const;
  void get_nearest_worker_and_best_pos (const Vec2Int build_pos, const EntityType buildType, const Entity *&entity, Vec2Int &best_pos) const;
  static int get_distance (const Vec2Int pos_a, const Vec2Int pos_b);
  int get_build_distance (const int id /*build*/, const Vec2Int pos) const;
  int get_build_to_build_distance (const Entity &a, const Entity &b) const;
  std::vector<Vec2Int> get_poses_around (const Vec2Int pos) const;
  int get_distance (const Entity &a, const Entity &b) const;
  int get_distance_for_base (const int id_a, const Vec2Int &pos_b, const EntityType type_b, Vec2Int &best_pos) const;
  std::vector<Vec2Int> get_nearest_free_places_for_me (const int id_a, const int id_b) const;
  std::vector<Vec2Int> get_nearest_free_places_for_me (const int id_a, const Vec2Int pos, const EntityType type) const;
  std::vector<Vec2Int> get_all_poses_in_area_of_entity (const Entity &entity, const int zone_size) const;

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
  void attack_step_back_from_melee ();
  void attack_out_zone ();
  void attack_others_in_zone ();
  void attack_others_out_zone ();
  void attack_clear_place ();
  void attack_try_heal ();
  void builder_step_back ();
  void move_army ();
  void turn_on_turrets ();
  void heal_nearest ();
  void protect_base ();

  void run_tasks ();
  void make_atack_groups ();
  void redirect_all_atack_move_tasks (const Vec2Int old_pos, const Vec2Int new_pos);

  int get_attack_pos_id_by_vec (const Vec2Int pos) const;
  int choose_atack_pos (const Vec2Int old_pos = Vec2Int (-1, -1));
};


int my_sign (const int a);
