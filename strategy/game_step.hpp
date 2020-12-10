#pragma once

#include "../MyStrategy.hpp"

class game_step
  {
  private:
    const PlayerView *playerView;
    DebugInterface *debugInterface;
    Action *result;

    int m_id = 0;
    int m_resource = 0;
    std::unordered_map<EntityType, std::vector<const Entity *>> m_entity;

  public:
    game_step (const PlayerView &_playerView, DebugInterface *_debugInterface, Action &_result);
  };