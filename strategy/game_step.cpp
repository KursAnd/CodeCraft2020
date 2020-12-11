#include "game_step.hpp"

game_step_t::game_step_t (const PlayerView &_playerView, DebugInterface *_debugInterface, Action &_result)
  : playerView (&_playerView), debugInterface (_debugInterface), result (&_result),
    m_res_pos (_playerView.mapSize - 1, _playerView.mapSize - 1)
{
  m_id = playerView->myId;
  for (const auto &player : playerView->players)
    if (player.id == m_id)
      {
        m_resource = player.resource;
      }

  for (const Entity &entity : playerView->entities)
    if (entity.playerId != nullptr &&*entity.playerId == m_id)
      {
        m_entity_set[entity.entityType].insert (entity.id);
        m_entity[entity.entityType].push_back (&entity);

        const EntityProperties &properties = playerView->entityProperties.at (entity.entityType);
        m_population_max += properties.populationProvide;
        m_population_use += properties.populationUse;
      }
    else if (entity.entityType == RESOURCE)
      {
        if (m_res_pos.x + m_res_pos.y > entity.position.x + entity.position.y)
          m_res_pos = entity.position;
      }

  if (m_res_pos.x == _playerView.mapSize - 1 && m_res_pos.y == _playerView.mapSize - 1)
    m_res_pos = Vec2Int (0, 0);
}

Vec2Int game_step_t::get_place_for_HOUSE () const
{
  static std::vector<Vec2Int> priority = { {0, 0}, {3, 0}, {0, 3}, {6, 0}, {0, 6}, {9, 0}, {0, 9}, {12, 0}, {0, 12}, {15, 0}, {0, 15} };
  std::unordered_set<int> exclude;
  for (const Entity *entity : m_entity.at (HOUSE))
    exclude.insert (entity->position.x * playerView->mapSize + entity->position.y);

  for (const Vec2Int &vec2 : priority)
    if (!exclude.count (vec2.x * playerView->mapSize + vec2.y))
      return vec2;
  return Vec2Int (-1, -1);
}

bool game_step_t::need_build (EntityType type) const
{
  switch (type)
    {
      case BUILDER_UNIT: return m_entity.at (BUILDER_UNIT).size () < std::max (6, m_population_max * 3 / 10);
      case RANGED_UNIT : return !need_build (BUILDER_UNIT);
      case MELEE_BASE  : return !need_build (BUILDER_UNIT);
      default: break;
    }
  return false;
}

std::unordered_map<int, const Entity *> task_groups_t::m_builder;

void task_groups_t::update (const game_step_t &game_step)
{
  std::unordered_map<int, const Entity *> ids = task_groups_t::m_builder;
  for (const std::pair<int, const Entity *> &el : ids)
    {
      if (!game_step.m_entity_set.at (BUILDER_UNIT).count (el.second->id))
        task_groups_t::m_builder.erase (el.second->id);
    }
}
