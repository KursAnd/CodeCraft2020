#include "MyStrategy.hpp"
#include <exception>
#include "strategy/strategy_all.h"

MyStrategy::MyStrategy() {}

Action MyStrategy::getAction(const PlayerView& playerView, DebugInterface* debugInterface)
{
    Action result = Action(std::unordered_map<int, EntityAction>());
    strategy_0 (playerView, debugInterface, result);
    return result;
}

void MyStrategy::debugUpdate(const PlayerView& playerView, DebugInterface& debugInterface)
{
    debugInterface.send(DebugCommand::Clear());
    debugInterface.getState();
}