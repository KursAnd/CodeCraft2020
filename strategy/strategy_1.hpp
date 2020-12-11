#pragma once
#include "game_step.hpp"
#include <iostream>

void strategy_1 (const PlayerView& playerView, DebugInterface* debugInterface, Action& result)
{
  game_step_t gs (playerView, debugInterface, result);


  gs.check_repair (BUILDER_BASE, result);
  gs.check_repair (MELEE_BASE  , result);
  gs.check_repair (RANGED_BASE , result);
  gs.check_repair (TURRET      , result);
  gs.check_repair (HOUSE       , result);
  gs.check_repair (WALL        , result);

  gs.try_train_unit (BUILDER_BASE, result);

  gs.try_build (BUILDER_BASE, result);
  gs.try_build (MELEE_BASE  , result);
  gs.try_build (RANGED_BASE , result);
  gs.try_build (TURRET      , result);
  gs.try_build (HOUSE       , result);
  gs.try_build (WALL        , result);

  gs.try_train_unit (RANGED_BASE , result);
  gs.try_train_unit (MELEE_BASE  , result);

  for (const Entity *entity : gs.get_vector (BUILDER_UNIT))
    {
      if (gs.is_busy (entity))
        continue;

      const EntityProperties &properties = playerView.entityProperties.at (entity->entityType);

      std::shared_ptr<MoveAction>   moveAction   = std::shared_ptr<MoveAction> (new MoveAction (gs.get_res_pos (), true, true));
      std::shared_ptr<BuildAction>  buildAction  = nullptr;
      std::shared_ptr<AttackAction> atackAction  = std::shared_ptr<AttackAction> (new AttackAction (nullptr, std::shared_ptr<AutoAttack> (new AutoAttack (properties.sightRange, {RESOURCE}))));
      std::shared_ptr<RepairAction> repairAction = nullptr;

      result.entityActions[entity->id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
    }

  for (const Entity *entity : gs.get_vector (RANGED_UNIT))
    {
      const EntityProperties &properties = playerView.entityProperties.at (entity->entityType);

      std::shared_ptr<MoveAction>   moveAction   = nullptr;
      std::shared_ptr<BuildAction>  buildAction  = nullptr;
      std::shared_ptr<AttackAction> atackAction  = nullptr;
      std::shared_ptr<RepairAction> repairAction = nullptr;

      //if (gs.m_entity[RANGED_UNIT].size () + gs.m_entity[MELEE_UNIT].size () >= 7)
        //moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (playerView.mapSize - 1, playerView.mapSize - 1), true, true));
      //else
        moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (10 + gs.get_count (RANGED_UNIT) * 2, 10 + gs.get_count (RANGED_UNIT) * 2), true, false));

      atackAction = std::shared_ptr<AttackAction> (new AttackAction (nullptr, std::shared_ptr<AutoAttack> (new AutoAttack (properties.sightRange, {}))));

      result.entityActions[entity->id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
    }

  for (const Entity *entity : gs.get_vector (MELEE_UNIT))
    {
      const EntityProperties &properties = playerView.entityProperties.at (entity->entityType);

      std::shared_ptr<MoveAction>     moveAction   = nullptr;
      std::shared_ptr<BuildAction>    buildAction  = nullptr;
      std::shared_ptr<AttackAction>   atackAction  = nullptr;
      std::shared_ptr<RepairAction> repairAction = nullptr;

      //if (gs.m_entity[RANGED_UNIT].size () + gs.m_entity[MELEE_UNIT].size () >= 7)
        //moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (playerView.mapSize - 1, playerView.mapSize - 1), true, true));
      //else
        moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (10 + gs.get_count (MELEE_UNIT) * 2, 10 + gs.get_count (MELEE_UNIT) * 2), true, false));

      atackAction = std::shared_ptr<AttackAction> (new AttackAction (nullptr, std::shared_ptr<AutoAttack> (new AutoAttack (properties.sightRange, {}))));

      result.entityActions[entity->id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
    }
  
}
