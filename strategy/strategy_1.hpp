#pragma once
#include "game_step.hpp"
#include <time.h>
#include <cstdlib>

void strategy_1 (const PlayerView& playerView, DebugInterface* debugInterface, Action& result)
{
  if (playerView.currentTick == 0)
    {
      srand(time(NULL));
    }

  game_step_t gs (playerView, result);

  gs.run_tasks ();
  gs.send_cleaner ();

  gs.check_repair (BUILDER_BASE);
  gs.check_repair (MELEE_BASE);
  gs.check_repair (RANGED_BASE);
  gs.check_repair (TURRET);
  gs.check_repair (HOUSE);
  gs.check_repair (WALL);
  
  gs.train_unit (BUILDER_BASE);
  gs.train_unit (RANGED_BASE);
  gs.train_unit (MELEE_BASE);
  
  gs.try_build (BUILDER_BASE);
  gs.try_build (MELEE_BASE);
  gs.try_build (RANGED_BASE);
  gs.try_build (HOUSE);
  gs.try_build (TURRET);
  gs.try_build (WALL);
  
  gs.make_atack_groups ();
  
  gs.move_builders ();
  gs.move_army (RANGED_UNIT);
  gs.move_army (MELEE_UNIT);

  gs.turn_on_turrets ();
}
