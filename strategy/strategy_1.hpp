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

  gs.train_unit (BUILDER_BASE, result);
  gs.train_unit (RANGED_BASE , result);
  gs.train_unit (MELEE_BASE  , result);

  gs.try_build (BUILDER_BASE, result);
  gs.try_build (MELEE_BASE  , result);
  gs.try_build (RANGED_BASE , result);
  gs.try_build (TURRET      , result);
  gs.try_build (HOUSE       , result);
  gs.try_build (WALL        , result);



  gs.move_builders (result);
  gs.move_army (RANGED_UNIT, result);
  gs.move_army (MELEE_UNIT , result);

  gs.turn_on_turrets (result);
}
