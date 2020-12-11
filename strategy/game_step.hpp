#pragma once

#include "../MyStrategy.hpp"
#include <unordered_set>
#include <unordered_map>
#include <set>

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
  int m_population_use = 0;

  Vec2Int m_res_pos;  // purpouse for BUILDERs who collect thr resources

  std::unordered_map<EntityType, std::unordered_set<int>> m_entity_set;
  std::unordered_map<EntityType, std::vector<const Entity *>> m_entity;

public:
  game_step_t (const PlayerView &_playerView, DebugInterface *_debugInterface, Action &_result);
  bool buy_entity (const EntityType type, const int cnt = 1);


  Vec2Int get_place_for (const EntityType type) const;
  bool need_build (const EntityType type) const;
  int entity_price (const EntityType type, const int cnt = 1) const;

  int get_count (const EntityType type) const;
  Vec2Int get_res_pos () const;
  const std::vector<const Entity*> &get_vector (const EntityType type) const;
};
