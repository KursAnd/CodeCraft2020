#pragma once
#include "game_step.hpp"
#include <iostream>

void strategy_1 (const PlayerView& playerView, DebugInterface* debugInterface, Action& result)
{
  game_step_t gs (playerView, debugInterface, result);

  for (int i = 0; i < 10; ++i)
    {
      std::cout << i << "\t";
      for (const Entity *entity : gs.get_vector (static_cast<EntityType> (i)))
        std::cout << entity->id << "(" << entity->health << ") ";
      std::cout << "\n";
    }
  std::cout << "\n\n";

  for (const Entity *house_entity : gs.get_vector (HOUSE))
    {
      const EntityProperties &properties = playerView.entityProperties.at (house_entity->entityType);

      if (house_entity->health < properties.maxHealth)
        {
          // TO-DO: let rebuild all workers near building
          const Entity *entity = nullptr;
          for (const Entity *_entity : gs.get_vector (BUILDER_UNIT))
            if (!gs.is_busy (_entity) && (!entity || gs.get_distance (house_entity, _entity) < gs.get_distance (house_entity, entity)))
              entity = _entity;

          if (entity)
            {
              const EntityProperties &properties = playerView.entityProperties.at (entity->entityType);
              EntityType buildEntityType = properties.build->options[0];

              std::shared_ptr<MoveAction>   moveAction   = std::shared_ptr<MoveAction> (new MoveAction (house_entity->position, true, true));
              std::shared_ptr<BuildAction>  buildAction  = nullptr;
              std::shared_ptr<AttackAction> atackAction  = nullptr;
              std::shared_ptr<RepairAction> repairAction = std::shared_ptr<RepairAction> (new RepairAction (house_entity->id));

              result.entityActions[entity->id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
              gs.make_busy (entity);
            }
        }
    }

  gs.try_build (HOUSE , result);
  gs.try_build (TURRET, result);

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


  for (const Entity *entity : gs.get_vector (BUILDER_BASE))
    {
      const EntityProperties &properties = playerView.entityProperties.at (entity->entityType);
      EntityType buildEntityType = properties.build->options[0];

      std::shared_ptr<MoveAction>   moveAction   = nullptr;
      std::shared_ptr<BuildAction>  buildAction  = nullptr;
      std::shared_ptr<AttackAction> atackAction  = nullptr;
      std::shared_ptr<RepairAction> repairAction = nullptr;

      if (gs.need_build (buildEntityType))
        {
          buildAction = std::shared_ptr<BuildAction> (new BuildAction (buildEntityType, Vec2Int (entity->position.x + properties.size, entity->position.y + properties.size - 1)));
          gs.buy_entity (buildEntityType);
        }

      result.entityActions[entity->id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
    }

  for (const Entity *entity : gs.get_vector (RANGED_BASE))
    {
      const EntityProperties &properties = playerView.entityProperties.at (entity->entityType);
      EntityType buildEntityType = properties.build->options[0];

      std::shared_ptr<MoveAction>   moveAction = nullptr;
      std::shared_ptr<BuildAction>  buildAction = nullptr;
      std::shared_ptr<AttackAction> atackAction = nullptr;
      std::shared_ptr<RepairAction> repairAction = nullptr;

      if (gs.need_build (buildEntityType))
        {
          buildAction = std::shared_ptr<BuildAction> (new BuildAction (buildEntityType, Vec2Int (entity->position.x + properties.size, entity->position.y + properties.size - 1)));
          gs.buy_entity (buildEntityType);
        }

      result.entityActions[entity->id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
    }

  for (const Entity *entity : gs.get_vector (MELEE_BASE))
    {
      const EntityProperties &properties = playerView.entityProperties.at (entity->entityType);
      EntityType buildEntityType = properties.build->options[0];

      std::shared_ptr<MoveAction>   moveAction = nullptr;
      std::shared_ptr<BuildAction>  buildAction = nullptr;
      std::shared_ptr<AttackAction> atackAction = nullptr;
      std::shared_ptr<RepairAction> repairAction = nullptr;

      if (gs.need_build (buildEntityType))
        {
          buildAction = std::shared_ptr<BuildAction> (new BuildAction (buildEntityType, Vec2Int (entity->position.x + properties.size, entity->position.y + properties.size - 1)));
          gs.buy_entity (buildEntityType);
        }

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
