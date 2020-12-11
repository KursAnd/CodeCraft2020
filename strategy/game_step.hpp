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

public:
  int m_id = 0;
  int m_resource = 0;
  int m_population_max = 0;
  int m_population_use = 0;

  Vec2Int m_res_pos;  // purpouse for BUILDERs who collect thr resources

  std::unordered_map<EntityType, std::unordered_set<int>> m_entity_set;
  std::unordered_map<EntityType, std::vector<const Entity *>> m_entity;

public:
  game_step_t (const PlayerView &_playerView, DebugInterface *_debugInterface, Action &_result);
  Vec2Int get_place_for_HOUSE () const;
  bool need_build (EntityType type) const;
};

class task_groups_t
{
public:
  static std::unordered_map<int, const Entity *> m_builder;

  static void update (const game_step_t &game_step);
};