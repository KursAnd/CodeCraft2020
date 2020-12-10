#include "game_step.hpp"

game_step::game_step (const PlayerView &_playerView, DebugInterface *_debugInterface, Action &_result)
  : playerView (&_playerView), debugInterface (_debugInterface), result (&_result)
{
  m_id = playerView->myId;
  for (auto &player : playerView->players)
    if (player.id == m_id)
      {
      m_resource = player.resource;
      }
  for (const Entity &entity : playerView->entities)
    if (entity.playerId != nullptr && *entity.playerId != m_id && entity.active == true)
      m_entity[entity.entityType].push_back (&entity);
}
