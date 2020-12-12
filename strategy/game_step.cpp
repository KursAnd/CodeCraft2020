#include "game_step.hpp"


std::unordered_map<int, std::function<bool(int)>> game_step_t::tasks; 

std::unordered_map<EntityType, std::vector<Vec2Int>> game_step_t::init_places_for_building ()
{
  std::unordered_map<EntityType, std::vector<Vec2Int>> res;
  res[HOUSE]  = { {0, 0}, {3, 0}, {0, 3}, {6, 0}, {0, 6}, {9, 0}, {0, 9}, {12, 0}, {0, 12}, {15, 0}, {0, 15} };
  res[TURRET] = { {15, 15}, {20, 6}, {6, 20}, {19, 12}, {12, 19}, {2, 20}, {20, 2} };
  res[BUILDER_BASE] = { {5, 5} };
  res[MELEE_BASE] = { {5, 15} };
  res[RANGED_BASE] = { {15, 5} };
  res[WALL] = {};
  return res;
}

game_step_t::game_step_t (const PlayerView &_playerView, DebugInterface *_debugInterface, Action &_result)
  : playerView (&_playerView), debugInterface (_debugInterface), result (&_result),
    m_res_pos (_playerView.mapSize - 1, _playerView.mapSize - 1)
{
  // hack to avoid const wraper that doesn't work on platform ..
  for (const EntityType tupe : {WALL, HOUSE, BUILDER_BASE, BUILDER_UNIT, MELEE_BASE,  MELEE_UNIT, RANGED_BASE, RANGED_UNIT, RESOURCE, TURRET})
    m_entity[tupe] = std::vector <Entity> ();

  m_id = playerView->myId;
  for (const auto &player : playerView->players)
    if (player.id == m_id)
      {
        m_resource = player.resource;
      }

  for (const Entity &entity : playerView->entities)
    if (entity.playerId != nullptr && *entity.playerId == m_id)
      {
        m_entity_type_by_id[entity.id] = entity.entityType;
        m_entity_by_id[entity.id] = entity;
        m_entity_set[entity.entityType].insert (entity.id);
        m_entity[entity.entityType].push_back (entity);

        const EntityProperties &properties = playerView->entityProperties.at (entity.entityType);
        if (entity.active)
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

Vec2Int game_step_t::get_place_for (const EntityType type) const
{
  static std::unordered_map <EntityType, std::vector<Vec2Int>> priority = init_places_for_building ();

  if (priority[type].size () > get_count (type))
    {
      std::unordered_set<int> exclude;
      for (const Entity &entity : get_vector (type))
        exclude.insert (entity.position.x * playerView->mapSize + entity.position.y);

      for (const Vec2Int &vec2 : priority[type])
        if (!exclude.count (vec2.x * playerView->mapSize + vec2.y))
          return vec2;
    }
  return Vec2Int (-1, -1);
}

bool game_step_t::need_build (const EntityType type) const
{
  if (collect_money || m_resource < entity_price (type))
    return false;
  switch (type)
    {
      case BUILDER_UNIT: return get_count (BUILDER_UNIT) < std::max (MIN_BUILDER_UNITS, m_population_max * 3 / 10);
      case RANGED_UNIT : return !need_build (BUILDER_UNIT);
      case MELEE_UNIT  : return !need_build (BUILDER_UNIT) && get_count (MELEE_UNIT) < get_count (RANGED_UNIT) * 2;

      case HOUSE       :
        return    get_count (BUILDER_UNIT) >= MIN_BUILDER_UNITS
               && m_res_pos.x + m_res_pos.y >= 6
               && m_population_use + 5 >= m_population_max;
      case TURRET      :
        return    get_count (BUILDER_UNIT) >= MIN_BUILDER_UNITS
               && get_count (RANGED_UNIT) + get_count (MELEE_UNIT) > get_count (TURRET)
               && m_population_use >= 20;
      default: break;
    }
  return false;
}

void game_step_t::check_have_build (const EntityType type)
{
  switch (type)
    {
      case BUILDER_BASE:
      case RANGED_BASE :
      case MELEE_BASE  :
        if (get_count (type) < 1)
          collect_money = true;
      default: break;
    }
}

int game_step_t::entity_price (const EntityType type, const int cnt) const
{
  const EntityProperties &properties = playerView->entityProperties.at (type);
  int price = properties.initialCost * cnt;
  switch (type)
    {
      case BUILDER_UNIT:
      case RANGED_UNIT :
      case MELEE_BASE  :
        price += get_count (type) * cnt + cnt * (cnt - 1) / 2;
        break;
      default : break;
    }
  return price;
}

bool game_step_t::buy_entity (const EntityType type, const int cnt)
{
  int price = entity_price (type, cnt);
  if (m_resource < price)
    return false;
  m_resource -= price;
  return true;
}

void game_step_t::make_busy (const Entity &entity)
{
  ids_was.insert (entity.id);
}

void game_step_t::make_busy (const int id)
{
  ids_was.insert (id);
}

Vec2Int game_step_t::get_res_pos () const
{
  return m_res_pos;
}

std::vector<Entity> &game_step_t::get_vector (const EntityType type)
{
  return m_entity[type];
}

const std::vector<Entity> &game_step_t::get_vector (const EntityType type) const
{
  // safe bec�use of hack in game_step_t::game_step_t
  return m_entity.at (type);
}

int game_step_t::get_count (const EntityType type) const
{
  if (!m_entity.count (type))
    return 0;
  return m_entity.at (type).size ();
}

int game_step_t::get_army_count () const
{
  return get_count (RANGED_UNIT) + get_count (MELEE_UNIT);
}

bool game_step_t::is_busy (const Entity &entity) const
{
  return ids_was.count (entity.id);
}

int game_step_t::get_distance (const Entity &ent_a, const Entity &ent_b)
{
  return (ent_a.position.x - ent_b.position.x) * (ent_a.position.x - ent_b.position.x)
       + (ent_a.position.y - ent_b.position.y) * (ent_a.position.y - ent_b.position.y);
}

int game_step_t::get_distance (const Vec2Int &pos, const Entity &ent_b, const EntityType type) const
{
  const int offset = playerView->entityProperties.at (type).size - 1;
  return (pos.x + offset - ent_b.position.x) * (pos.x + offset - ent_b.position.x)
       + (pos.y + offset - ent_b.position.y) * (pos.y + offset - ent_b.position.y);
}

void game_step_t::try_build (const EntityType buildType, Action& result)
{
  if (!need_build (buildType))
    return check_have_build (buildType);

  Vec2Int pos = get_place_for (buildType);
  if (pos.x < 0)
    return;

  const Entity *entity = nullptr;
  for (const Entity &_entity : get_vector (BUILDER_UNIT))
    {
      if (   !is_busy (_entity)
          && (!entity || get_distance (pos, _entity, buildType) < get_distance (pos, *entity, buildType)))
        entity = &_entity;
    }
  if (!entity)
    return;

  const EntityProperties &properties = playerView->entityProperties.at (entity->entityType);
  const EntityProperties &buildProperties = playerView->entityProperties.at (buildType);

  std::shared_ptr<MoveAction>   moveAction   = nullptr;
  std::shared_ptr<BuildAction>  buildAction  = nullptr;
  std::shared_ptr<AttackAction> atackAction  = nullptr;
  std::shared_ptr<RepairAction> repairAction = nullptr;

  if (   entity->position.x == pos.x + buildProperties.size - 1 && entity->position.y == pos.y + buildProperties.size
      || entity->position.y == pos.y + buildProperties.size - 1 && entity->position.x == pos.x + buildProperties.size)
    buildAction = std::shared_ptr<BuildAction> (new BuildAction (buildType, pos));
  else if (pos.x + buildProperties.size - 1 == entity->position.x && pos.y + buildProperties.size - 1 == entity->position.y)
    moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (pos.x + buildProperties.size, pos.y + buildProperties.size - 1), true, true));
  else
    moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (pos.x + buildProperties.size - 1, pos.y + buildProperties.size - 1), true, true));

  make_busy (*entity);
  buy_entity (buildType);
          
  result.entityActions[entity->id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
}

void game_step_t::train_unit (const EntityType factoryType, Action &result)
{
  for (const Entity &entity : get_vector (factoryType))
    {
      const EntityProperties &properties = playerView->entityProperties.at (entity.entityType);
      EntityType buildType = properties.build->options[0];

      std::shared_ptr<MoveAction>   moveAction   = nullptr;
      std::shared_ptr<BuildAction>  buildAction  = nullptr;
      std::shared_ptr<AttackAction> atackAction  = nullptr;
      std::shared_ptr<RepairAction> repairAction = nullptr;

      if (need_build (buildType))
        {
          //TO-DO: build in different places + check emptiness
          buildAction = std::shared_ptr<BuildAction> (new BuildAction (buildType, Vec2Int (entity.position.x + properties.size, entity.position.y + properties.size - 1)));

          make_busy (entity);
          buy_entity (buildType);
        }

      result.entityActions[entity.id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
    }
}

void game_step_t::check_repair (const EntityType repairType, Action &result)
{
  for (const Entity &repair_entity : get_vector (repairType))
    {
      const EntityProperties &properties = playerView->entityProperties.at (repair_entity.entityType);

      if (repair_entity.health >= properties.maxHealth)
        continue;

      // TO-DO: let rebuild all workers near building
      const Entity *entity = nullptr;
      for (const Entity &_entity : get_vector (BUILDER_UNIT))
        {
          if (!is_busy (_entity) && (!entity || get_distance (repair_entity, _entity) < get_distance (repair_entity, *entity)))
            entity = &_entity;
        }
      if (!entity)
        continue;

      std::shared_ptr<MoveAction>   moveAction   = std::shared_ptr<MoveAction> (new MoveAction (repair_entity.position, true, true));
      std::shared_ptr<BuildAction>  buildAction  = nullptr;
      std::shared_ptr<AttackAction> atackAction  = nullptr;
      std::shared_ptr<RepairAction> repairAction = std::shared_ptr<RepairAction> (new RepairAction (repair_entity.id));

      make_busy (*entity);
      result.entityActions[entity->id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
    }
}

void game_step_t::move_builders (Action &result)
{
  const EntityType type = BUILDER_UNIT;
  const EntityProperties &properties = playerView->entityProperties.at (type);

  for (const Entity &entity : get_vector (type))
    {
      if (is_busy (entity))
        continue;

      std::shared_ptr<MoveAction>   moveAction   = std::shared_ptr<MoveAction> (new MoveAction (get_res_pos (), true, true));
      std::shared_ptr<BuildAction>  buildAction  = nullptr;
      std::shared_ptr<AttackAction> atackAction  = std::shared_ptr<AttackAction> (new AttackAction (nullptr, std::shared_ptr<AutoAttack> (new AutoAttack (properties.sightRange, {RESOURCE}))));
      std::shared_ptr<RepairAction> repairAction = nullptr;

      result.entityActions[entity.id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
    }
}

void game_step_t::move_army (const EntityType type, Action& result)
{
  const EntityProperties &properties = playerView->entityProperties.at (type);
  
  for (const Entity &entity : get_vector (type))
    {
      if (is_busy (entity))
        continue;

      std::shared_ptr<MoveAction>   moveAction   = nullptr;
      std::shared_ptr<BuildAction>  buildAction  = nullptr;
      std::shared_ptr<AttackAction> atackAction  = std::shared_ptr<AttackAction> (new AttackAction (nullptr, std::shared_ptr<AutoAttack> (new AutoAttack (properties.sightRange, {}))));;
      std::shared_ptr<RepairAction> repairAction = nullptr;

      if (get_army_count () > 20)
        moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (playerView->mapSize - 1, playerView->mapSize - 1), true, true));
      else
        moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (10 + rand () % 14, 10 + rand () % 14), true, false));

      result.entityActions[entity.id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
    }
}

void game_step_t::turn_on_turrets (Action &result)
{
  const EntityType type = TURRET;
  const EntityProperties &properties = playerView->entityProperties.at (type);

  for (const Entity &entity : get_vector (type))
    {
      std::shared_ptr<MoveAction>   moveAction   = nullptr;
      std::shared_ptr<BuildAction>  buildAction  = nullptr;
      std::shared_ptr<AttackAction> atackAction  = std::shared_ptr<AttackAction> (new AttackAction (nullptr, std::shared_ptr<AutoAttack> (new AutoAttack (properties.sightRange, {}))));
      std::shared_ptr<RepairAction> repairAction = nullptr;

      result.entityActions[entity.id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
    }
}

void game_step_t::run_tasks ()
{
  std::unordered_set<int> finished;
  for (auto &task : game_step_t::tasks)
    {
      int id = task.first;
      auto func = task.second;
      if (!m_entity_by_id.count (id) || func (id))
        {
          finished.insert (id);
          continue;
        }
      make_busy (id);
    }
  for (const int id : finished)
    game_step_t::tasks.erase (id);
}

void game_step_t::add_task (const int id, std::function<bool (int)> func)
{
  game_step_t::tasks[id] = func;
}
