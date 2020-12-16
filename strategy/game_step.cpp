#include "game_step.hpp"


std::vector<int> game_step_t::cleaner_lvs = {0, 2};


game_step_t::game_step_t (const PlayerView &_playerView, Action &_result)
  : playerView (&_playerView), result (&_result),
    m_res_pos (_playerView.mapSize - 1, _playerView.mapSize - 1)
{
  // hack to avoid const wraper that doesn't work on platform ..
  for (const EntityType type : {WALL, HOUSE, BUILDER_BASE, BUILDER_UNIT, MELEE_BASE,  MELEE_UNIT, RANGED_BASE, RANGED_UNIT, RESOURCE, TURRET})
    {
      m_entity[type] = std::vector <Entity> ();
      m_enemy[type] = std::vector<Entity> ();
    }

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
    {
      m_entity_by_id[entity.id] = entity;
      if (entity.playerId != nullptr)
        {
          if (*entity.playerId == m_id)
            {
              m_entity[entity.entityType].push_back (entity);

              const EntityProperties &properties = playerView->entityProperties.at (entity.entityType);
              if (entity.active)
                m_population_max += properties.populationProvide;
              m_population_max_future += properties.populationProvide;
              m_population_use += properties.populationUse;
            }
          else
            {
              m_enemy[entity.entityType].push_back (entity);
            }
        }
      else if (entity.entityType == RESOURCE)
        {
          if (m_res_pos.x + m_res_pos.y > entity.position.x + entity.position.y)
            m_res_pos = entity.position;
        }
    }
  if (m_res_pos.x == _playerView.mapSize - 1 && m_res_pos.y == _playerView.mapSize - 1)
    m_res_pos = Vec2Int (0, 0);

  const int custom_value = (playerView->fogOfWar ? 100 : 0);
  map = std::vector<std::vector<int>> (playerView->mapSize);
  map_id = std::vector<std::vector<int>> (playerView->mapSize);
  for (int i = 0; i < playerView->mapSize; ++i)
    {
      map[i] = std::vector<int> (playerView->mapSize, custom_value);
      map_id[i] = std::vector<int> (playerView->mapSize, -1);
    }
  if (playerView->fogOfWar)
    {
      for (const Entity &entity : playerView->entities)
        {
          if (entity.playerId != nullptr && *entity.playerId == m_id)
            {
              const EntityProperties &p = playerView->entityProperties.at (entity.entityType);
              for (int x = std::max (0, entity.position.x - p.sightRange); x < std::min (playerView->mapSize - 1, entity.position.x + p.size + p.sightRange); ++x)
                for (int y = entity.position.y; y < entity.position.y + p.size; ++y)
                  map[x][y] = 0;
              for (int x = entity.position.x; x < entity.position.x + p.size; ++x)
                {
                  for (int y = std::max (0, entity.position.y - p.sightRange); y < entity.position.y; ++y)
                    map[x][y] = 0;
                  for (int y = entity.position.y + p.size; y < std::min (playerView->mapSize - 1, entity.position.y + p.size + p.sightRange); ++y)
                    map[x][y] = 0;
                }
              for (int k = 1, x = entity.position.x + p.size; x < std::min (playerView->mapSize - 1, entity.position.x + p.size + p.sightRange - 1); ++x, ++k)
                {
                  for (int y = entity.position.y - 1; y >= std::max (0, entity.position.y - p.sightRange + k); --y)
                    map[x][y] = 0;
                  for (int y = entity.position.y + p.size; y < std::min (playerView->mapSize - 1, entity.position.y + p.size + p.sightRange - k); ++y)
                    map[x][y] = 0;
                }
              for (int k = 1, x = entity.position.x - 1; x >= std::max (0, entity.position.x - p.sightRange + 1); --x, ++k)
                {
                  for (int y = entity.position.y - 1; y >= std::max (0, entity.position.y - p.sightRange + k); --y)
                    map[x][y] = 0;
                  for (int y = entity.position.y + p.size; y < std::min (playerView->mapSize - 1, entity.position.y + p.size + p.sightRange - k); ++y)
                    map[x][y] = 0;
                }
            }
        }
    }
  for (const Entity &entity : playerView->entities)
    {
      const EntityProperties &properties = playerView->entityProperties.at (entity.entityType);
      for (int x = entity.position.x; x < entity.position.x + properties.size; ++x)
        for (int y = entity.position.y; y < entity.position.y + properties.size; ++y)
          {
            if (entity.playerId == nullptr || *entity.playerId == m_id)
              map[x][y] = entity.entityType + 1;
            else
              map[x][y] = -(entity.entityType + 1);
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
  return INCORRECT_VEC2INT;
}

bool game_step_t::need_build (const EntityType type) const
{
  if (m_resource < entity_price (type))
    return false;
  switch (type)
    {
      case BUILDER_UNIT: return get_count (BUILDER_UNIT) < std::max (MIN_BUILDER_UNITS, m_population_max * 4 / 10)
                             || (get_count (BUILDER_UNIT) < MIN_BUILDER_UNITS * 2 && !can_build (RANGED_UNIT) && !can_build (MELEE_UNIT));
      case RANGED_UNIT : return get_count (BUILDER_UNIT) >= MIN_BUILDER_UNITS;
      case MELEE_UNIT  : return get_count (BUILDER_UNIT) >= MIN_BUILDER_UNITS
                             && (   (m_population_use <  40                          && get_count (MELEE_UNIT)     < get_count (RANGED_UNIT)    )
                                 || (m_population_use >= 40 && m_population_use < 80 && get_count (MELEE_UNIT) * 3 < get_count (RANGED_UNIT) * 2)
                                 || (m_population_max >= 80                          && get_count (MELEE_UNIT)     < get_count (RANGED_UNIT) * 2)
                                 || !can_build (RANGED_UNIT));

      case HOUSE       :
        return (get_count (BUILDER_UNIT) >= MIN_BUILDER_UNITS || m_population_use < MIN_BUILDER_UNITS)
            && m_population_use + 10 >= m_population_max_future
            && get_count (BUILDER_BASE) > 0;
      case TURRET      :
        return get_count (BUILDER_UNIT) >= MIN_BUILDER_UNITS
            && m_population_max > 30
            && m_population_use > 25
            && 1.0 * m_population_use / m_population_max > 0.65
            && get_count (BUILDER_BASE) > 0;
      case WALL        :
        return get_count (BUILDER_UNIT) >= MIN_BUILDER_UNITS
            && m_population_max > 30
            && m_population_use > 25
            && (   get_count (WALL) < 2
                || (   get_count (TURRET) > 3
                    && get_count (WALL) * 2 < get_count (TURRET) * 3
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
  return get_count (BUILDER_UNIT) > 0  && is_correct (get_place_for (type));
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

bool game_step_t::is_place_contain_enemy (const int x, const int y, const EntityType type) const
{
  return map.at (x).at (y) == -(type + 1); // 0 is empty
}

int game_step_t::get_max_distance () const
{
  return playerView->mapSize * 2;
}

int game_step_t::get_distance (const Vec2Int pos_a, const Vec2Int pos_b)
{
  return std::abs (pos_a.x - pos_b.x) + std::abs (pos_a.y - pos_b.y);
}

bool game_step_t::is_correct (const Vec2Int pos) const
{
  return pos.x >= 0 && pos.y >= 0 && pos.x < playerView->mapSize && pos.y < playerView->mapSize;
}

bool game_step_t::is_correct_xy (const int x, const int y) const
{
  return x >= 0 && y >= 0 && x < playerView->mapSize && y < playerView->mapSize;
}

Vec2Int game_step_t::get_cleaner_aim (const int cleaner_id) const
{
  if (game_step_t::cleaner_lvs[cleaner_id] >= game_step_t::cleaner_aims.size ())
    return INCORRECT_VEC2INT;
  while (game_step_t::cleaner_lvs[cleaner_id] < game_step_t::cleaner_aims.size ())
    {
      const EntityType type = game_step_t::cleaner_aims.at (game_step_t::cleaner_lvs[cleaner_id]).first;
      const int lv          = game_step_t::cleaner_aims.at (game_step_t::cleaner_lvs[cleaner_id]).second;
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
      game_step_t::cleaner_lvs[cleaner_id]++;      
    }
  return INCORRECT_VEC2INT;
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

void game_step_t::get_nearest_worker_and_best_pos (const Vec2Int build_pos, const EntityType buildType, const Entity *&entity, Vec2Int &best_pos) const
{
  Vec2Int temp_pos = INCORRECT_VEC2INT;
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
  res.reserve (static_cast<size_t> (properties.size) * 4ll);
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
  if (!is_correct (build_pos))
    return;
  
  const EntityProperties &buildProperties = playerView->entityProperties.at (buildType);

  int builders_cnt = 0;
  while (builders_cnt < count_workers_to_repair (buildType))
    {
      const Entity *entity = nullptr;
      Vec2Int best_pos = INCORRECT_VEC2INT;
      
      get_nearest_worker_and_best_pos (build_pos, buildType, entity, best_pos);

      if (!entity || !is_correct (best_pos))
        break;

      std::shared_ptr<MoveAction>   moveAction   = std::shared_ptr<MoveAction> (new MoveAction (best_pos, true, true));
      std::shared_ptr<BuildAction>  buildAction  = std::shared_ptr<BuildAction> (new BuildAction (buildType, build_pos));
      std::shared_ptr<AttackAction> atackAction  = nullptr;
      std::shared_ptr<RepairAction> repairAction = nullptr;

      make_busy (*entity);
      result->entityActions[entity->id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
      builders_cnt++;
      map[best_pos.x][best_pos.y] = BUILDER_UNIT + 10; // hack 
      // map_id[best_pos.x][best_pos.y] = entity->id; // TO-DO: I'm not sure in it
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
          Vec2Int best_pos = INCORRECT_VEC2INT;
      
          get_nearest_worker_and_best_pos (repair_entity.position, repairType, entity, best_pos);

          if (!entity || !is_correct (best_pos))
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

      if (entity.id % 4 < 2)
        moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (10 + rand () % 13, 10 + rand () % 13), true, false));
      else if (entity.id % 4 == 2)
        moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (20 + rand () % 4, 3 + rand () % 12), true, false));
      else
        moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (3 + rand () % 12, 20 + rand () % 4), true, false));

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

void game_step_t::send_cleaners ()
{
  if (cleaner_lvs.size () == 2 && m_population_max > 15)
    cleaner_lvs.push_back (cleaner_lvs.back () + 2);
  for (int cleaner_id = 0; cleaner_id < game_step_t::cleaner_lvs.size (); ++cleaner_id)
    {
      const Vec2Int pos = get_cleaner_aim (cleaner_id);
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
}

void game_step_t::save_builders ()
{
  const EntityType type = BUILDER_UNIT;

  for (const Entity &entity : get_vector (type))
    {
      if (is_busy (entity))
        continue;

      Vec2Int safe_pos = INCORRECT_VEC2INT;
      if (   find_escape_way_for_workers (RANGED_UNIT, entity.position, safe_pos)
          || find_escape_way_for_workers (MELEE_UNIT , entity.position, safe_pos))
        {
          std::shared_ptr<MoveAction>   moveAction   = std::shared_ptr<MoveAction> (new MoveAction (safe_pos, true, true));
          std::shared_ptr<BuildAction>  buildAction  = nullptr;
          std::shared_ptr<AttackAction> atackAction  = nullptr;
          std::shared_ptr<RepairAction> repairAction = nullptr;

          make_busy (entity.id);
          map[safe_pos.x][safe_pos.y] = BUILDER_UNIT + 10; // hack 
          // map_id[best_pos.x][best_pos.y] = entity->id; // TO-DO: I'm not sure in it
          result->entityActions[entity.id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
        }
    }
}

void game_step_t::heal_nearest ()
{
  const EntityType healer_type = BUILDER_UNIT;

  for (const Entity &healer : get_vector (healer_type))
    {
      if (is_busy (healer))
        continue;

      for (const Vec2Int repair_pos : {Vec2Int (healer.position.x + 1, healer.position.y    ),
                                       Vec2Int (healer.position.x    , healer.position.y + 1),
                                       Vec2Int (healer.position.x - 1, healer.position.y    ),
                                       Vec2Int (healer.position.x    , healer.position.y - 1)})
        {
          if (!is_correct (repair_pos))
            continue;
          const int repair_id = map_id[repair_pos.x][repair_pos.y];
          if (repair_id < 0)
            continue;
          const Entity &repair_entity = m_entity_by_id.at (repair_id);
          if (   (repair_entity.playerId == nullptr || *repair_entity.playerId != m_id)
              || (repair_entity.entityType != RANGED_UNIT && repair_entity.entityType != MELEE_UNIT && repair_entity.entityType != BUILDER_UNIT))
            continue;
          const EntityProperties &p = playerView->entityProperties.at (repair_entity.entityType);
          if (repair_entity.health < p.maxHealth)
            {
              std::shared_ptr<MoveAction>   moveAction   = nullptr;
              std::shared_ptr<BuildAction>  buildAction  = nullptr;
              std::shared_ptr<AttackAction> atackAction  = nullptr;
              std::shared_ptr<RepairAction> repairAction = std::shared_ptr<RepairAction> (new RepairAction (repair_entity.id));;

              make_busy (healer.id);
              result->entityActions[healer.id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
              break;
            }
        }
      //for (const EntityType repair_type : {RANGED_UNIT, MELEE_UNIT, BUILDER_UNIT})
      //  {
      //    const EntityProperties &p = playerView->entityProperties.at (repair_type);
      //    for (const Entity &repair_entity : get_vector (repair_type))
      //      {
      //        if (repair_entity.health < p.maxHealth && get_distance (healer.position, repair_entity.position) == 1)
      //          {
      //            std::shared_ptr<MoveAction>   moveAction   = nullptr;
      //            std::shared_ptr<BuildAction>  buildAction  = nullptr;
      //            std::shared_ptr<AttackAction> atackAction  = nullptr;
      //            std::shared_ptr<RepairAction> repairAction = std::shared_ptr<RepairAction> (new RepairAction (repair_entity.id));;

      //            make_busy (healer.id);
      //            result->entityActions[healer.id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
      //            break;
      //          }
      //      }
      //    if (is_busy (healer))
      //      break;
      //  }
    }
}

bool game_step_t::find_escape_way_for_workers (const EntityType type, const Vec2Int worker_pos, Vec2Int &safe_pos) const
{
  const int safe_range = playerView->entityProperties.at (type).attack->attackRange + 5;
  const int l = 0, r = 1, d = 2, u = 3;
  //                       l  r  d  u
  std::vector<int> escape {0, 0, 0, 0};
  for (const Entity &enemy : m_enemy.at (type))
    {
      if (get_distance (worker_pos, enemy.position) <= safe_range)
        {
          if (enemy.position.x > worker_pos.x) escape[l]++;
          if (enemy.position.x < worker_pos.x) escape[r]++;
          if (enemy.position.y > worker_pos.y) escape[d]++;
          if (enemy.position.y < worker_pos.y) escape[u]++;
        }
    }

  std::vector<Vec2Int> ways, bad_way, near_pos = {
    Vec2Int (worker_pos.x - 1, worker_pos.y    ),
    Vec2Int (worker_pos.x + 1, worker_pos.y    ),
    Vec2Int (worker_pos.x    , worker_pos.y - 1),
    Vec2Int (worker_pos.x    , worker_pos.y + 1)
  };
  ways.reserve (4);

  auto choose_dir = [&] (const int l, const int u, const int d)
    {
      if (escape[l])
        {
          if (is_correct (near_pos[l]) && is_place_free (near_pos[l].x, near_pos[l].y))
            {
              ways.push_back (near_pos[l]);
            }
          else if (ways.empty () && bad_way.empty ())
            {
              if (!escape[u] && is_correct (near_pos[d]) && is_place_free (near_pos[d].x, near_pos[d].y))
                bad_way.push_back (near_pos[d]);
              else if (!escape[d] && is_correct (near_pos[u]) && is_place_free (near_pos[u].x, near_pos[u].y))
                bad_way.push_back (near_pos[u]);
            } 
        }
    };

  choose_dir (l, u, d);
  choose_dir (r, u, d);
  choose_dir (d, l, r);
  choose_dir (u, l, r);

  if (!ways.empty ())
    {
      if (ways.size () == 4)
        return false;

      safe_pos = ways[rand () % ways.size ()];
      return true;
    }
  if (!bad_way.empty ())
    {
      safe_pos = bad_way.back ();
      return true;
    }

  return false;
}
