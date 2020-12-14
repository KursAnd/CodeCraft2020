#include "game_step.hpp"


std::unordered_map<int, int> game_step_t::repair_tasks;
std::unordered_map<int, Vec2Int> game_step_t::attack_move_tasks;
std::unordered_set<int> game_step_t::destroyed_pos;
int game_step_t::cleaner_lv = 0;

int game_step_t::choose_atack_pos (const Vec2Int old_pos)
{
  if (destroyed_pos.size () == attack_pos.size ())
    destroyed_pos.clear ();

  if (old_pos.x != -1)
    {
      int old_dir = get_id_pos_by_vec (old_pos);
      if ((old_dir == 0 || old_dir == 2) && !destroyed_pos.count (1))
        return 1;
    }

  if (old_pos.x == -1 && (!destroyed_pos.count (0) || !destroyed_pos.count (2)))
    {
      int dir = -1;
      do
        {
          dir = (rand () % 2) * 2;
        }
      while (destroyed_pos.count (dir));
      return dir;
    }

  int dir = -1;
  do
    {
      dir = rand () % attack_pos.size ();
    }
  while (destroyed_pos.count (dir));

  return dir;
}

int game_step_t::get_id_pos_by_vec (const Vec2Int pos)
{
  int dir = -1;
  for (int i = 0; i < attack_pos.size (); ++i)
    if (attack_pos[i] == pos)
      {
        dir = i;
        break;
      }
  return dir;
}

game_step_t::game_step_t (const PlayerView &_playerView, Action &_result)
  : playerView (&_playerView), result (&_result),
    m_res_pos (_playerView.mapSize - 1, _playerView.mapSize - 1)
{
  // hack to avoid const wraper that doesn't work on platform ..
  for (const EntityType tupe : {WALL, HOUSE, BUILDER_BASE, BUILDER_UNIT, MELEE_BASE,  MELEE_UNIT, RANGED_BASE, RANGED_UNIT, RESOURCE, TURRET})
    m_entity[tupe] = std::vector <Entity> ();

  {
    std::unordered_map<EntityType, std::vector<Vec2Int>> &res = priority_places_for_building;
    res[HOUSE]  = { {0, 0}, {4, 0}, {0, 4}, {7, 0}, {0, 7}, {10, 0}, {0, 10}, {13, 0}, {0, 13}, {11, 3}, {3, 11}, {16, 0}, {0, 16}, {11, 6}, {6, 11}, {13, 11}, {10, 11} };
    res[TURRET] = { {15, 15}, {20, 7}, {7, 20}, {5, 20}, {20, 5}, {16, 13}, {13, 16}, {2, 20}, {20, 2}, {17, 11}, {11, 17}, {11, 19}, {19, 11}, {20, 0}, {0, 20} };
    res[BUILDER_BASE] = { {5, 5} };
    res[MELEE_BASE] = { {5, 15} };
    res[RANGED_BASE] = { {15, 5} };
    res[WALL] = { {22, 7}, {22, 8}, {7, 22}, {8, 22}, {22, 6}, {6, 22}, {22, 9}, {9, 22}, {22, 5}, {5, 22}, {22, 4}, {4, 22}, {21, 9}, {9, 21}, {17, 15}, {15, 17}, {17, 16}, {16, 17},
      {18, 13}, {13, 18}, {18, 14}, {14, 18}, {19, 13}, {13, 19}, {20, 13}, {13, 20}, {21, 12}, {12, 21}, {21, 11}, {11, 21}, {22, 3}, {3, 22}, {22, 2}, {2, 22}, {22, 1}, {1, 22}, {22, 0}, {0, 22}};

    // for safety in .at ()
    res[BUILDER_UNIT] = {};
    res[MELEE_UNIT] = {};
    res[RANGED_UNIT] = {};
    res[RESOURCE] = {};
  }
  {
    attack_pos = {{playerView->mapSize - 5, 5}, {playerView->mapSize - 5, playerView->mapSize - 5}, {5, playerView->mapSize - 5}};
  }
  {
    cleaner_aims = {
        {BUILDER_BASE, 0},
                                  {HOUSE, 0}, {HOUSE, 1}, {HOUSE, 2}, {HOUSE, 3}, {HOUSE, 4},
        {RANGED_BASE, 0},
        {MELEE_BASE, 0},
        {TURRET, 0}, {TURRET, 1},
                                  {HOUSE, 5}, {HOUSE, 6}, {HOUSE, 7}, {HOUSE, 8}, {HOUSE, 9},
        {TURRET, 2}, {TURRET, 3},
                                  {HOUSE, 10}, {HOUSE, 11}, {HOUSE, 16},
        {TURRET, 4}, {TURRET, 5}, {TURRET, 6}, {TURRET, 14}
    };
  }

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
        m_population_max_future += properties.populationProvide;
        m_population_use += properties.populationUse;
      }
    else if (entity.entityType == RESOURCE)
      {
        if (m_res_pos.x + m_res_pos.y > entity.position.x + entity.position.y)
          m_res_pos = entity.position;
      }

  if (m_res_pos.x == _playerView.mapSize - 1 && m_res_pos.y == _playerView.mapSize - 1)
    m_res_pos = Vec2Int (0, 0);

  map = std::vector<std::vector<int>> (playerView->mapSize);
  map_id = std::vector<std::vector<int>> (playerView->mapSize);
  for (int i = 0; i < playerView->mapSize; ++i)
    {
      map[i] = std::vector<int> (playerView->mapSize, 0);
      map_id[i] = std::vector<int> (playerView->mapSize, -1);
    }
  for (const Entity &entity : playerView->entities)
    {
      const EntityProperties &properties = playerView->entityProperties.at (entity.entityType);
      for (int x = entity.position.x; x < entity.position.x + properties.size; ++x)
        for (int y = entity.position.y; y < entity.position.y + properties.size; ++y)
          {
            map[x][y] = entity.entityType + 1;
            map_id[x][y] = entity.id;
          }
    }
}

Vec2Int game_step_t::get_place_for (const EntityType type) const
{
  if (priority_places_for_building.at (type).size () > get_count (type))
    {
      for (const Vec2Int &vec2 : priority_places_for_building.at (type))
        {
          if (is_empty_space_for_type (vec2, type))
            return vec2;
        }
    }
  return Vec2Int (-1, -1);
}

bool game_step_t::need_build (const EntityType type) const
{
  if (m_resource < entity_price (type))
    return false;
  switch (type)
    {
      case BUILDER_UNIT: return get_count (BUILDER_UNIT) < std::max (MIN_BUILDER_UNITS, m_population_max * 4 / 10);
      case RANGED_UNIT : return get_count (BUILDER_UNIT) >= MIN_BUILDER_UNITS;
      case MELEE_UNIT  : return get_count (BUILDER_UNIT) >= MIN_BUILDER_UNITS
                             && (   get_count (MELEE_UNIT) < get_count (RANGED_UNIT) * 2
                                 || !can_build (RANGED_UNIT));

      case HOUSE       :
        return (get_count (BUILDER_UNIT) >= MIN_BUILDER_UNITS || m_population_use < MIN_BUILDER_UNITS)
            && m_population_use + 5 >= m_population_max_future;
      case TURRET      :
        return get_count (BUILDER_UNIT) >= MIN_BUILDER_UNITS
            && m_population_max >= 30
            && 1.0 * m_population_use / m_population_max > 0.65
            && get_count (BUILDER_BASE) > 0;
      case WALL        :
        return get_count (BUILDER_UNIT) >= MIN_BUILDER_UNITS
            && m_population_max >= 30
            && (   get_count (WALL) < 2
                || (   get_count (TURRET) > 3
                    && get_count (WALL) < get_count (TURRET) * 2
                    && 1.0 * m_population_use / m_population_max > 0.7))
            && get_base_count () >= 2;

      case BUILDER_BASE:
      case RANGED_BASE :
      case MELEE_BASE  :
        return get_count (type) == 0;
      default: break;
    }
  return false;
}

bool game_step_t::can_build (const EntityType type) const
{
  if (m_resource < entity_price (type))
    return false;
  switch (type)
    {
      case BUILDER_UNIT:
        return get_count (BUILDER_BASE) != 0 && m_entity.at (BUILDER_BASE)[0].active;
      case RANGED_UNIT:
        return get_count (RANGED_BASE) != 0 && m_entity.at (RANGED_BASE)[0].active;
      case MELEE_UNIT:
        return get_count (MELEE_BASE) != 0 && m_entity.at (MELEE_BASE)[0].active;
      default: break;
    }
  return get_count (BUILDER_UNIT) > 0  && get_place_for (type).x != -1;
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
  // safe becàuse of hack in game_step_t::game_step_t
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

int game_step_t::get_base_count () const
{
  return get_count (BUILDER_BASE) + get_count (RANGED_BASE) + get_count (MELEE_BASE);
}

bool game_step_t::is_busy (const Entity &entity) const
{
  return ids_was.count (entity.id);
}

bool game_step_t::is_empty_space_for_type (const Vec2Int pos, const EntityType type) const
{
  const EntityProperties &properties = playerView->entityProperties.at (type);
  for (int x = pos.x; x < pos.x + properties.size; ++x)
    for (int y = pos.y; y < pos.y + properties.size; ++y)
      {
        if (!is_place_free (x, y))
          return false;
      }
  return true;
}

bool game_step_t::is_first_building_attaked (const EntityType type) const
{
  if (get_count (type) == 0)
    return true;
  const EntityProperties &properties = playerView->entityProperties.at (type);
  const Entity &entity = m_entity.at (type)[0];
  if (entity.active && entity.health < properties.maxHealth)
    return true;
  return false;
}

bool game_step_t::is_place_free_or_my (const int x, const int y, const int id) const
{
  return is_place_free (x, y) || map_id.at (x).at (y) == id;
}

bool game_step_t::is_place_free (const int x, const int y) const
{
  return map.at (x).at (y) == 0;
}

bool game_step_t::is_place_contain (const int x, const int y, const EntityType type) const
{
  return map.at (x).at (y) == type + 1; // 0 is empty
}

int game_step_t::get_max_distance () const
{
  return playerView->mapSize * 2;
}

int game_step_t::get_distance (const Vec2Int pos_a, const Vec2Int pos_b)
{
  return std::abs (pos_a.x - pos_b.x) + std::abs (pos_a.y - pos_b.y);
}

Vec2Int game_step_t::get_cleaner_aim () const
{
  if (game_step_t::cleaner_lv >= game_step_t::cleaner_aims.size ())
    return Vec2Int (-1, -1);
  while (game_step_t::cleaner_lv < game_step_t::cleaner_aims.size ())
    {
      const EntityType type = game_step_t::cleaner_aims.at (game_step_t::cleaner_lv).first;
      const int lv          = game_step_t::cleaner_aims.at (game_step_t::cleaner_lv).second;
      const Vec2Int pos = priority_places_for_building.at (type).at (lv);
      const EntityProperties &properties = playerView->entityProperties.at (type);
      if (!is_place_contain (pos.x, pos.y, type))
        {
          for (int x = pos.x; x < pos.x + properties.size; ++x)
            for (int y = pos.y; y < pos.y + properties.size; ++y)
              {
                if (is_place_contain (x, y, RESOURCE))
                  return Vec2Int (x, y);
              }
        }
      game_step_t::cleaner_lv++;      
    }
  return Vec2Int (-1, -1);
}

int game_step_t::get_distance_for_base (const int id_a, const Vec2Int &pos_b, const EntityType type_b, Vec2Int &best_pos) const
{
  const Vec2Int pos_a = m_entity_by_id.at (id_a).position;
  int res = get_max_distance ();
  for (const Vec2Int pos : get_nearest_free_places_for_me (id_a, pos_b, type_b))
    {
      int dist = get_distance (pos_a, pos);
      if (res > dist)
        {
          res = dist;
          best_pos = pos;
          if (res == 0)
            break;
        }
    }
  return res;
}

bool game_step_t::get_pos_for_safe_operation (const EntityType type, Vec2Int &pos) const
{
  bool attacked = false;
  const EntityProperties &properties = playerView->entityProperties.at (type);
  if (get_count (type) == 0)
    {
      if (type == BUILDER_BASE || type == MELEE_BASE || type == RANGED_BASE)
        {
          pos = priority_places_for_building.at (type)[0];
          pos.x += properties.size / 2 + rand () % properties.size;
          pos.y += properties.size / 2 + rand () % properties.size;
          attacked = true;
        }
    }
  else
    {
      for (const Entity &entity : get_vector (type))
        {
          if (entity.active && entity.health < properties.maxHealth)
            {
              pos = entity.position;
              pos.x += properties.size / 2 + rand () % properties.size;
              pos.y += properties.size / 2 + rand () % properties.size;
              attacked = true;
              break;
            }
        }
    }
  return attacked;
}

void game_step_t::get_nearest_worker_and_best_pos (const Vec2Int build_pos, const EntityType buildType, const Entity *&entity, Vec2Int &best_pos) const
{
  Vec2Int temp_pos (-1, -1);
  int dist = get_max_distance ();
  for (const Entity &_entity : get_vector (BUILDER_UNIT))
    {
      if (!is_busy (_entity))
        {
          const int new_dist = get_distance_for_base (_entity.id, build_pos, buildType, temp_pos);
          if (new_dist == get_max_distance ())
            continue;
          if (!entity || new_dist < dist)
            {
              entity = &_entity;
              best_pos = temp_pos;
              if (new_dist == 0)
                break;
              dist = new_dist;
            }
        }
    }
}

std::vector<Vec2Int> game_step_t::get_nearest_free_places_for_me (const int id_a, const int id_b) const
{
  const Entity &entity = m_entity_by_id.at (id_b);
  return get_nearest_free_places_for_me (id_a, entity.position, entity.entityType);
}

std::vector<Vec2Int> game_step_t::get_nearest_free_places_for_me (const int id_worker, const Vec2Int pos, const EntityType type) const
{
  const EntityProperties &properties = playerView->entityProperties.at (type);
  std::vector<Vec2Int> res;
  if (pos.x > 0)
    for (int y = pos.y; y < pos.y + properties.size; ++y)
      if (is_place_free_or_my (pos.x - 1, y, id_worker))
        res.push_back (Vec2Int (pos.x - 1, y));
  if (pos.y > 0)
    for (int x = pos.x; x < pos.x + properties.size; ++x)
      if (is_place_free_or_my (x, pos.y - 1, id_worker))
        res.push_back (Vec2Int (x, pos.y - 1));
  if (pos.x < playerView->mapSize - 1)
    for (int y = pos.y; y < pos.y + properties.size; ++y)
      if (is_place_free_or_my (pos.x + properties.size, y, id_worker))
        res.push_back (Vec2Int (pos.x + properties.size, y));
  if (pos.x < playerView->mapSize - 1)
    for (int x = pos.x; x < pos.x + properties.size; ++x)
      if (is_place_free_or_my (x, pos.y + properties.size, id_worker))
        res.push_back (Vec2Int (x, pos.y + properties.size));
  return std::move (res);
}

int game_step_t::count_workers_to_repair (const EntityType type) const
{
  // TO-DO: make it by id and count free places
  switch (type)
    {
      case BUILDER_BASE:
      case MELEE_BASE:
      case RANGED_BASE:
        return 6;
      case HOUSE:
      case TURRET:
        return 3;
      case WALL:
        return 2;
    }
  return 0;
}

void game_step_t::try_build (const EntityType buildType)
{
  if (!need_build (buildType))
    return;

  Vec2Int build_pos = get_place_for (buildType);
  if (build_pos.x < 0)
    return;
  
  const EntityProperties &buildProperties = playerView->entityProperties.at (buildType);

  int builders_cnt = 0;
  while (builders_cnt < count_workers_to_repair (buildType))
    {
      const Entity *entity = nullptr;
      Vec2Int best_pos (-1, -1);
      
      get_nearest_worker_and_best_pos (build_pos, buildType, entity, best_pos);

      if (!entity || best_pos.x == -1)
        break;

      std::shared_ptr<MoveAction>   moveAction   = std::shared_ptr<MoveAction> (new MoveAction (best_pos, true, true));
      std::shared_ptr<BuildAction>  buildAction  = std::shared_ptr<BuildAction> (new BuildAction (buildType, build_pos));
      std::shared_ptr<AttackAction> atackAction  = nullptr;
      std::shared_ptr<RepairAction> repairAction = nullptr;

      make_busy (*entity);
      result->entityActions[entity->id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
      builders_cnt++;
      map[best_pos.x][best_pos.y] = BUILDER_UNIT + 10; // hack 
      map_id[best_pos.x][best_pos.y] = entity->id; // TO-DO: I'm not sure in it
    }

  if (builders_cnt > 0)
    buy_entity (buildType);
}

void game_step_t::train_unit (const EntityType factoryType)
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

      result->entityActions[entity.id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
    }
}

void game_step_t::check_repair (const EntityType repairType)
{
  for (const Entity &repair_entity : get_vector (repairType))
    {
      const EntityProperties &properties = playerView->entityProperties.at (repair_entity.entityType);

      if (repair_entity.health >= properties.maxHealth)
        continue;

      int repairs_cnt = 0;
      while (repairs_cnt < count_workers_to_repair (repairType))
        {
          const Entity *entity = nullptr;
          Vec2Int best_pos (-1, -1);
      
          get_nearest_worker_and_best_pos (repair_entity.position, repairType, entity, best_pos);

          if (!entity || best_pos.x == -1)
            break;

          std::shared_ptr<MoveAction>   moveAction   = std::shared_ptr<MoveAction> (new MoveAction (best_pos, true, true));
          std::shared_ptr<BuildAction>  buildAction  = nullptr;
          std::shared_ptr<AttackAction> atackAction  = nullptr;
          std::shared_ptr<RepairAction> repairAction = std::shared_ptr<RepairAction> (new RepairAction (repair_entity.id));

          //add_repair_task (entity->id, repair_entity.id);
          make_busy (entity->id);
          result->entityActions[entity->id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
          repairs_cnt++;
          map[best_pos.x][best_pos.y] = BUILDER_UNIT + 10; // hack 
          map_id[best_pos.x][best_pos.y] = entity->id; // TO-DO: I'm not sure in it
        }
    }
}

void game_step_t::move_builders ()
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

      result->entityActions[entity.id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
    }
}

void game_step_t::move_army (const EntityType type)
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

      Vec2Int pos;
      if (   get_pos_for_safe_operation (BUILDER_BASE, pos)
          || get_pos_for_safe_operation (RANGED_BASE , pos)
          || get_pos_for_safe_operation (MELEE_BASE  , pos)
          || get_pos_for_safe_operation (TURRET      , pos)
          || get_pos_for_safe_operation (HOUSE       , pos)
          || get_pos_for_safe_operation (WALL        , pos))
        moveAction = std::shared_ptr<MoveAction> (new MoveAction (pos, true, false));
      else
        {
          if (entity.id % 4 < 2)
            moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (10 + rand () % 13, 10 + rand () % 13), true, false));
          else if (entity.id % 4 == 2)
            moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (20 + rand () % 4, 3 + rand () % 12), true, false));
          else
            moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (3 + rand () % 12, 20 + rand () % 4), true, false));
        }

      result->entityActions[entity.id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
    }
}

void game_step_t::turn_on_turrets ()
{
  const EntityType type = TURRET;
  const EntityProperties &properties = playerView->entityProperties.at (type);

  for (const Entity &entity : get_vector (type))
    {
      std::shared_ptr<MoveAction>   moveAction   = nullptr;
      std::shared_ptr<BuildAction>  buildAction  = nullptr;
      std::shared_ptr<AttackAction> atackAction  = std::shared_ptr<AttackAction> (new AttackAction (nullptr, std::shared_ptr<AutoAttack> (new AutoAttack (properties.sightRange, {}))));
      std::shared_ptr<RepairAction> repairAction = nullptr;

      result->entityActions[entity.id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
    }
}

void game_step_t::make_atack_groups ()
{
  if (get_army_count () - game_step_t::attack_move_tasks.size () > 12 || (!game_step_t::destroyed_pos.empty () && get_count (TURRET) >= 3))
    {
      int dir = choose_atack_pos ();
      if (dir < 0)
        return;
      for (const Entity &entity : get_vector (RANGED_UNIT))
        {
          if (rand () % 20 == 0 || is_busy (entity)) // 95%
            continue;
          move_solder (entity, attack_pos[dir]);
        }
      for (const Entity &entity : get_vector (MELEE_UNIT))
        {
          if (rand () % 20 == 0 || is_busy (entity)) // 95%
            continue;
          move_solder (entity, attack_pos[dir]);
        }
    }
}

void game_step_t::move_solder (const Entity &entity, const Vec2Int &pos, bool need_add_task)
{
  const EntityProperties &properties = playerView->entityProperties.at (entity.entityType);

  std::shared_ptr<MoveAction>   moveAction   = std::shared_ptr<MoveAction> (new MoveAction (pos, true, true));;
  std::shared_ptr<BuildAction>  buildAction  = nullptr;
  std::shared_ptr<AttackAction> atackAction  = std::shared_ptr<AttackAction> (new AttackAction (nullptr, std::shared_ptr<AutoAttack> (new AutoAttack (properties.sightRange, {}))));;
  std::shared_ptr<RepairAction> repairAction = nullptr;
  
  if (need_add_task)
    add_move_task (entity.id, pos);
  else
    make_busy (entity.id);
  result->entityActions[entity.id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
}

void game_step_t::send_cleaner ()
{
  const Vec2Int pos = get_cleaner_aim ();
  if (pos.x < 0)
    return;

  const EntityType type = BUILDER_UNIT;
  const EntityProperties &properties = playerView->entityProperties.at (type);

  for (const Entity &entity : get_vector (type))
    {
      if (is_busy (entity))
        continue;

      std::shared_ptr<MoveAction>   moveAction   = std::shared_ptr<MoveAction> (new MoveAction (pos, false, true));
      std::shared_ptr<BuildAction>  buildAction  = nullptr;
      std::shared_ptr<AttackAction> atackAction  = std::shared_ptr<AttackAction> (new AttackAction (std::shared_ptr<int> (new int (map_id[pos.x][pos.y])), nullptr));
      std::shared_ptr<RepairAction> repairAction = nullptr;

      make_busy (entity.id);
      result->entityActions[entity.id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
      break;
    }
}

void game_step_t::run_tasks ()
{
  std::unordered_set<int> finished;
  for (auto &task : game_step_t::repair_tasks)
    {
      int id = task.first;
      int id_rep = task.second;
      if (!m_entity_by_id.count (id) || !m_entity_by_id.count (id_rep))
        {
          repair_ids[id_rep]--;
          finished.insert (id);
          continue;
        }
      const Entity &entity = m_entity_by_id[id_rep];
      const EntityProperties &properties = playerView->entityProperties.at (entity.entityType);
      if (entity.health >= properties.maxHealth)
        {
          repair_ids.erase (id_rep);
          finished.insert (id);
          continue;
        }
      repair_ids[id_rep]++;
      make_busy (id);
    }
  for (const int id : finished)
    game_step_t::repair_tasks.erase (id);
  finished.clear ();

  bool need_redirect = false;
  Vec2Int vec_old (0,0), vec_new (0, 0);
  for (auto &task : game_step_t::attack_move_tasks)
    {
      int id = task.first;
      Vec2Int pos = task.second;
      if (!m_entity_by_id.count (id))
        {
          finished.insert (id);
          continue;
        }
      const Entity &entity = m_entity_by_id[id];
      // TO-DO: cancel and go to heal by health
      if (get_distance (entity.position, pos) <= 1)
        {
          int old_dir = get_id_pos_by_vec (pos);
          destroyed_pos.insert (old_dir);

          int new_dir = choose_atack_pos (pos);
          if (new_dir == -1)
            finished.insert (id);
          else
            {
              need_redirect = true;
              vec_old = pos;
              vec_new = attack_pos[new_dir];
            }
          continue;
        }
      make_busy (id);
    }
  for (const int id : finished)
    game_step_t::attack_move_tasks.erase (id);
  if (need_redirect)
    redirect_all_atack_move_tasks (vec_old, vec_new);
}


void game_step_t::add_repair_task (const int id, const int id_rep)
{
  if (repair_ids[id_rep] >= count_workers_to_repair (m_entity_type_by_id[id_rep]))
    return;
  game_step_t::repair_tasks[id] = id_rep;
  repair_ids[id_rep]++;
  make_busy (id);
}

void game_step_t::add_move_task (const int id, const Vec2Int pos)
{
  game_step_t::attack_move_tasks[id] = pos;
  make_busy (id);
}

void game_step_t::redirect_all_atack_move_tasks (const Vec2Int old_pos, const Vec2Int new_pos)
{
  for (auto &task : game_step_t::attack_move_tasks)
    {
      const int id = task.first;
      const Vec2Int pos = task.second;
      if (pos == old_pos)
        {
          move_solder (m_entity_by_id[id], new_pos, false);
          task.second = new_pos;
        }
    }
}
