#include "game_step.hpp"

#include <iterator>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <unordered_set>
#include <unordered_map>

std::unordered_set<int> game_step_t::destroyed_pos;
std::unordered_map<int, Vec2Int> game_step_t::attack_move_tasks;


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
    attack_pos = {{playerView->mapSize - 5, 5}, {playerView->mapSize - 5, playerView->mapSize - 5}, {5, playerView->mapSize - 5}};
  }

  m_id = playerView->myId;
  for (const auto &player : playerView->players)
    if (player.id == m_id)
      {
        m_resource = player.resource;
        break;
      }

  for (const Entity &entity : playerView->entities)
    {
      if (entity.playerId != nullptr)
        {
          if (*entity.playerId == m_id)
            {
              m_entity_by_id[entity.id] = find_id_t (false, entity.entityType, m_entity[entity.entityType].size ());
              m_entity[entity.entityType].push_back (entity);
              const EntityProperties &properties = playerView->entityProperties.at (entity.entityType);
              if (entity.active)
                m_population_max += properties.populationProvide;
              m_population_max_future += properties.populationProvide;
              m_population_use += properties.populationUse;
            }
          else
            {
              m_entity_by_id[entity.id] = find_id_t (true, entity.entityType, m_enemy[entity.entityType].size ());
              m_enemy[entity.entityType].push_back (entity);
            }
        }
      else if (entity.entityType == RESOURCE)
        {
          m_entity_by_id[entity.id] = find_id_t (false, entity.entityType, m_entity[entity.entityType].size ());
          m_entity[entity.entityType].push_back (entity);
          if (m_res_pos.x + m_res_pos.y > entity.position.x + entity.position.y)
            m_res_pos = entity.position;
        }
    }

  if (m_res_pos.x == _playerView.mapSize - 1 && m_res_pos.y == _playerView.mapSize - 1)
    m_res_pos = Vec2Int (0, 0);

  calculate_enemies_near_base ();

  if (m_entity[RESOURCE].empty () && playerView->currentTick > 30)
    {
      builders_is_attakers = true;
      const EntityProperties &properties = playerView->entityProperties.at (BUILDER_UNIT);
      for (const Entity &entity : get_vector (BUILDER_UNIT))
        {
          std::shared_ptr<MoveAction>   moveAction   = nullptr;
          std::shared_ptr<AttackAction> atackAction  = std::shared_ptr<AttackAction> (new AttackAction (nullptr, std::shared_ptr<AutoAttack> (new AutoAttack (properties.sightRange, {}))));
          switch (entity.id % 3)
            {
              case 0: moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (playerView->mapSize / 2 + rand () % (playerView->mapSize / 2), rand () % (playerView->mapSize / 2)), true, true)); break;
              case 1: moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (rand () % (playerView->mapSize / 2), playerView->mapSize / 2 + rand () % (playerView->mapSize / 2)), true, true)); break;
              case 2: moveAction = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (playerView->mapSize / 2 + rand () % (playerView->mapSize / 2), playerView->mapSize / 2 + rand () % (playerView->mapSize / 2)), true, true)); break;
            }

          make_busy (entity.id);
          result->entityActions[entity.id] = EntityAction (moveAction, nullptr, atackAction, nullptr);
        }
    }

  const int custom_value = (playerView->fogOfWar ? 100 : 0);
  map = std::vector<std::vector<int>> (playerView->mapSize);
  map_id = std::vector<std::vector<int>> (playerView->mapSize);
  map_damage = std::vector<std::vector<int>> (playerView->mapSize);
  map_damage_melee = std::vector<std::vector<int>> (playerView->mapSize);
  map_damage_who = std::vector<std::vector<std::unordered_set<int>>> (playerView->mapSize);
  for (int i = 0; i < playerView->mapSize; ++i)
    {
      map[i] = std::vector<int> (playerView->mapSize, custom_value);
      map_id[i] = std::vector<int> (playerView->mapSize, -1);
      map_damage[i] = std::vector<int> (playerView->mapSize, 0);
      map_damage_melee[i] = std::vector<int> (playerView->mapSize, 0);
      map_damage_who[i] = std::vector<std::unordered_set<int>> (playerView->mapSize, std::unordered_set<int> ());
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
  for (const EntityType type : {/*BUILDER_UNIT, */MELEE_UNIT, RANGED_UNIT, TURRET})
    for (const Entity &entity : get_enemy_vector (type))
      if (entity.active)
        set_map_damage (entity);

  recalculate_map_run ();
}

Vec2Int game_step_t::get_place_for (const EntityType type) const
{
  if (type == HOUSE)
    {
      Vec2Int pos = INCORRECT_VEC2INT;
      for (int l = 0; l < MAX_LINES_OF_HOUSES; ++l)
        {
          for (int i = 0; i < l; ++i)
            {
              for (int j = 0; j < l; ++j)
                {
                  Vec2Int p = Vec2Int (1 + i * 4, 1 + j * 4);
                  if (is_empty_space_for_type (p, type))
                    {
                      pos = p;
                      break;
                    }
                }
              if (is_correct (pos))
                break;
            }
          if (is_correct (pos))
            break;
          Vec2Int p = Vec2Int (1 + l * 4, 1 + l * 4);
          if (is_empty_space_for_type (p, type))
            {
              pos = p;
              break;
            }
        }
      return pos;
    }
  else if (type == RANGED_BASE)
    {
      Vec2Int pos = INCORRECT_VEC2INT;
      for (int l = 0; l < MAX_POSITION_OF_RANGED_BASE; ++l)
        {
          for (int x = 0; x < l; ++x)
            {
              if (x != 0 && x % 4 == 0)
                continue;
              for (int y = 0; y < l; ++y)
                {
                  if (y != 0 && y % 4 == 0)
                    continue;
                  Vec2Int p = Vec2Int (x, y);
                  if (is_empty_space_for_type (p, type))
                    {
                      pos = p;
                      break;
                    }
                }
              if (is_correct (pos))
                break;
            }
          if (is_correct (pos))
            break;
          Vec2Int p = Vec2Int (l, l);
          if (is_empty_space_for_type (p, type))
            {
              pos = p;
              break;
            }
        }
      return pos;
    }
  return INCORRECT_VEC2INT;
}

bool game_step_t::need_build (const EntityType type) const
{
  if (m_resource < entity_price (type))
    return false;
  switch (type)
    {
      case BUILDER_UNIT: return (   get_count (BUILDER_UNIT) < std::max (MIN_BUILDER_UNITS, m_population_max * 4 / 10)
                                 || get_count (RANGED_UNIT) == 0)
                             && get_count (RESOURCE) > get_count (BUILDER_UNIT)
                             && !builders_is_attakers;
      case RANGED_UNIT : return get_count (BUILDER_UNIT) >= MIN_BUILDER_UNITS
                             || !game_step_t::enemy_near_base_ids.empty ();
      case MELEE_UNIT  : return !can_build (RANGED_UNIT)
                             && !game_step_t::enemy_near_base_ids.empty ();

      case HOUSE       :
        return m_population_use + 10 > m_population_max_future
            && (get_count (RANGED_BASE) > 0 || get_count (BUILDER_UNIT) < MIN_BUILDER_UNITS)
            && (get_count (RANGED_BASE) > 0 || get_count (HOUSE) < 5)
            && game_step_t::enemy_near_base_ids.empty ();
      case TURRET      :
        return get_count (BUILDER_UNIT) >= MIN_BUILDER_UNITS
            && m_population_use > 30
            && 1.0 * m_population_use / m_population_max > 0.65
            && get_count (BUILDER_BASE) > 0
            && get_count (RANGED_BASE) > 0
            && game_step_t::enemy_near_base_ids.empty ();
      case WALL        :
        return get_count (BUILDER_UNIT) >= MIN_BUILDER_UNITS
            && m_population_use > 30
            && 1.0 * m_population_use / m_population_max > 0.7
            && get_count (BUILDER_BASE) > 0
            && get_count (RANGED_BASE) > 0
            && game_step_t::enemy_near_base_ids.empty ();

      case BUILDER_BASE:
      case RANGED_BASE :
      case MELEE_BASE  :
        return get_count (type) == 0
            && (game_step_t::enemy_near_base_ids.empty () || playerView->currentTick < 100);
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

void game_step_t::set_map_damage (const Entity &entity, bool add)
{
  const EntityProperties &p = playerView->entityProperties.at (entity.entityType);

  for (const Vec2Int pos : get_all_poses_in_area_of_entity (entity, p.attack->attackRange))
    {
      map_damage[pos.x][pos.y] += add ? p.attack->damage : -p.attack->damage;
      if (entity.entityType == MELEE_UNIT)
        map_damage_melee[pos.x][pos.y] += add ? p.attack->damage : -p.attack->damage;
      else
        {
          if (add)
            map_damage_who[pos.x][pos.y].insert (entity.id);
          else
            map_damage_who[pos.x][pos.y].erase (entity.id);
        }
    }
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

Vec2Int game_step_t::get_step_back_for_entity (const Entity &entity, const int level) const
{
  Vec2Int pos = entity.position;
  Vec2Int best_pos = INCORRECT_VEC2INT;
  std::vector<std::vector<Vec2Int>> best_poses = std::vector<std::vector<Vec2Int>> (level);
  for (const Vec2Int p : get_poses_around (pos))
    {
      if (is_place_free (p) && map_run[p.x][p.y] < level)
        best_poses[map_run[p.x][p.y]].push_back (p);
    }
  for (int i = 0; i < level; ++ i)
    if (!best_poses[i].empty ())
      {
        best_pos = best_poses[i][rand () % best_poses[i].size ()];
        break;
      }
  return best_pos;
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

std::vector<Entity> &game_step_t::get_enemy_vector (const EntityType type)
{
  return m_enemy[type];
}

const std::vector<Entity> &game_step_t::get_enemy_vector (const EntityType type) const
{
  // safe becàuse of hack in game_step_t::game_step_t
  return m_enemy.at (type);
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

Entity &game_step_t::get_entity_by_id (const int id)
{
  const find_id_t opt = m_entity_by_id.at (id);
  if (opt.is_enemy)
    return m_enemy[opt.type][opt.it];
  else
    return m_entity[opt.type][opt.it];
}

void game_step_t::recalculate_map_run ()
{
  map_run = std::vector<std::vector<int>> (playerView->mapSize);
  map_run_melee = std::vector<std::vector<int>> (playerView->mapSize);
  map_damage_who_run = std::vector<std::vector<std::unordered_set<int>>> (playerView->mapSize);
  for (int i = 0; i < playerView->mapSize; ++i)
    {
      map_run[i] = std::vector<int> (playerView->mapSize, 0);
      map_run_melee[i] = std::vector<int> (playerView->mapSize, 0);
      map_damage_who_run[i] = std::vector<std::unordered_set<int>> (playerView->mapSize, std::unordered_set<int> ());
    }

  // attack zone
  for (int x = 0; x < playerView->mapSize; ++x)
    for (int y = 0; y < playerView->mapSize; ++y)
      {
        if (map_damage[x][y] > 0)
          map_run[x][y] = 3;
        if (map_damage_melee[x][y] > 0)
          map_run_melee[x][y] = 3;
        map_damage_who_run[x][y] = map_damage_who[x][y];
      }

  // set map_<>[x1][y1] = lv1 if map_<>[x1][y1] == 0 && map_<>[x2][y2] == lv2
  auto updater = [&] (const int x1, const int y1, const int lv1, const int x2, const int y2, const int lv2)
    {
      if (map_run[x1][y1] == 0 && map_run[x2][y2] == lv2)
        {
          map_run[x1][y1] = lv1;
          map_damage_who_run[x1][y1] =   lv2 == 3
                                       ? map_damage_who[x2][y2]
                                       : map_damage_who_run[x2][y2];
        }
      if (map_run_melee[x1][y1] == 0 && map_run_melee[x2][y2] == lv2)
        map_run_melee[x1][y1] = lv1;
    };

  // attack zone in 1 step
  for (int x = 1; x < playerView->mapSize; ++x)
    for (int y = 0; y < playerView->mapSize; ++y)
      {
        updater (x    , y    , 2, x - 1, y    , 3);
        updater (x - 1, y    , 2, x    , y    , 3);
        updater (y    , x    , 2, y    , x - 1, 3);
        updater (y    , x - 1, 2, y    , x    , 3);
      }
  // attack zone in 2 step
  for (int x = 1; x < playerView->mapSize; ++x)
    for (int y = 0; y < playerView->mapSize; ++y)
      {
        updater (x    , y    , 1, x - 1, y    , 2);
        updater (x - 1, y    , 1, x    , y    , 2);
        updater (y    , x    , 1, y    , x - 1, 2);
        updater (y    , x - 1, 1, y    , x    , 2);
      }
}

void game_step_t::calculate_enemies_near_base ()
{
  int base_zone_x = 0, base_zone_y = 0;
  for (const EntityType type : {BUILDER_BASE, RANGED_BASE, MELEE_BASE, HOUSE, WALL, TURRET}) // TO-DO: think about TURRET
    {
      const EntityProperties &p = playerView->entityProperties.at (type);
      for (const Entity &entity : get_vector (type))
        {
          for (const EntityType enemy_type : {MELEE_UNIT, RANGED_UNIT})
            {
              const EntityProperties &p_enemy = playerView->entityProperties.at (enemy_type);
              for (const Entity &enemy : get_enemy_vector (enemy_type))
                {
                  if (get_distance (entity, enemy) <= p_enemy.sightRange)
                    enemy_near_base_ids.push_back (entity.id);
                }
            }
          base_zone_x = std::max (base_zone_x, entity.position.x + p.size);
          base_zone_y = std::max (base_zone_y, entity.position.y + p.size);
        }
    }

  // return attack group
  if (!enemy_near_base_ids.empty ())
    {
      std::vector<int> need_return;
      for (const std::pair<int, Vec2Int> &task : game_step_t::attack_move_tasks)
        {
          const int entity_id = task.first;
          if (!m_entity_by_id.count (entity_id))
            {
              need_return.push_back (entity_id);
              continue;
            }
          const Entity &entity = get_entity_by_id (entity_id);
          if (entity.position.x <= base_zone_x + 5 && entity.position.y <= base_zone_y + 5)
            need_return.push_back (entity_id);
        }
      for (const int id : need_return)
        game_step_t::attack_move_tasks.erase (id);
    }
}

const Entity &game_step_t::get_entity_by_id (const int id) const
{
  const find_id_t opt = m_entity_by_id.at (id);
  if (opt.is_enemy)
    return m_enemy.at (opt.type).at (opt.it);
  else
    return m_entity.at (opt.type).at (opt.it);
}

bool game_step_t::is_busy (const int id) const
{
  return ids_was.count (id);
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

bool game_step_t::is_place_free (const Vec2Int pos) const
{
  return is_correct (pos) &&  map.at (pos.x).at (pos.y) == 0;
}

bool game_step_t::is_place_free (const int x, const int y) const
{
  return is_correct_xy (x, y) &&  map.at (x).at (y) == 0;
}

bool game_step_t::is_place_contain (const int x, const int y, const EntityType type) const
{
  return map.at (x).at (y) == type + 1; // 0 is empty
}

bool game_step_t::is_place_contain_enemy (const int x, const int y, const EntityType type) const
{
  return map.at (x).at (y) == -(type + 1); // 0 is empty
}

bool game_step_t::is_place_contain_us (const int x, const int y) const
{
  return map.at (x).at (y) > 0 && map.at (x).at (y) != RESOURCE;
}

bool game_step_t::is_place_contain_enemy (const int x, const int y) const
{
  return map.at (x).at (y) < 0 && map.at (x).at (y) != RESOURCE;;
}

int game_step_t::get_max_distance () const
{
  return playerView->mapSize * 2;
}

int game_step_t::get_distance (const Vec2Int pos_a, const Vec2Int pos_b)
{
  return std::abs (pos_a.x - pos_b.x) + std::abs (pos_a.y - pos_b.y);
}

// only TURRETS support optimized
int game_step_t::get_build_distance (const int id /*build*/, const Vec2Int pos_b) const
{
  const Entity &build = get_entity_by_id (id);
  const EntityProperties &properties = playerView->entityProperties.at (build.entityType);
  int dist = get_max_distance ();
  for (int x = build.position.x; x < build.position.x + properties.size; ++x)
    for (int y = build.position.y; y < build.position.y + properties.size; ++y)
      {
        dist = std::min (dist, get_distance (Vec2Int (x, y), pos_b));
      }
  return dist;
}

// only TURRETS support optimized
int game_step_t::get_build_to_build_distance (const Entity &a, const Entity &b) const
{
  const EntityProperties &prop_a = playerView->entityProperties.at (a.entityType);
  const EntityProperties &prop_b = playerView->entityProperties.at (b.entityType);
  int dist = get_max_distance ();
  for (int x = a.position.x; x < a.position.x + prop_a.size; ++x)
    for (int y = a.position.y; y < a.position.y + prop_b.size; ++y)
      {
        for (int q = b.position.x; q < b.position.x + prop_a.size; ++q)
          for (int p = b.position.y; p < b.position.y + prop_b.size; ++p)
            dist = std::min (dist, get_distance (Vec2Int (x, y), Vec2Int (q, p)));
      }
  return dist;
}

int game_step_t::get_distance (const Entity &a, const Entity &b) const
{
  if (is_building (a.entityType))
    {
      if (is_building (b.entityType))
        return get_build_to_build_distance (a, b);
      else
        return get_build_distance (a.id, b.position);
    }
  else
    {
      if (is_building (b.entityType))
        return get_build_distance (b.id, a.position);
      else
        return get_distance (a.position, b.position);
    }
}

bool game_step_t::is_correct (const Vec2Int pos) const
{
  return pos.x >= 0 && pos.y >= 0 && pos.x < playerView->mapSize && pos.y < playerView->mapSize;
}

bool game_step_t::is_correct_xy (const int x, const int y) const
{
  return x >= 0 && y >= 0 && x < playerView->mapSize && y < playerView->mapSize;
}

int game_step_t::get_distance_for_base (const int id_a, const Vec2Int &pos_b, const EntityType type_b, Vec2Int &best_pos) const
{
  const Vec2Int pos_a = get_entity_by_id (id_a).position;
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
  const Entity &entity = get_entity_by_id (id_b);
  return get_nearest_free_places_for_me (id_a, entity.position, entity.entityType);
}

std::vector<Vec2Int> game_step_t::get_nearest_free_places_for_me (const int id_worker, const Vec2Int pos, const EntityType type) const
{
  const EntityProperties &properties = playerView->entityProperties.at (type);
  std::vector<Vec2Int> res;
  res.reserve (static_cast<size_t> (properties.size) * 4ll);
  if (pos.x > 0)
    for (int y = pos.y; y < pos.y + properties.size; ++y)
      if (is_place_free_or_my (pos.x - 1, y, id_worker) && map_run[pos.x - 1][y] == 0)
        res.push_back (Vec2Int (pos.x - 1, y));
  if (pos.y > 0)
    for (int x = pos.x; x < pos.x + properties.size; ++x)
      if (is_place_free_or_my (x, pos.y - 1, id_worker) && map_run[x][pos.y - 1] == 0)
        res.push_back (Vec2Int (x, pos.y - 1));
  if (pos.x < playerView->mapSize - 1)
    for (int y = pos.y; y < pos.y + properties.size; ++y)
      if (is_place_free_or_my (pos.x + properties.size, y, id_worker) && map_run[pos.x + properties.size][y] == 0)
        res.push_back (Vec2Int (pos.x + properties.size, y));
  if (pos.x < playerView->mapSize - 1)
    for (int x = pos.x; x < pos.x + properties.size; ++x)
      if (is_place_free_or_my (x, pos.y + properties.size, id_worker) && map_run[x][pos.y + properties.size] == 0)
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
        return 10;
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
          Vec2Int best_pos = INCORRECT_VEC2INT;
          for (const Vec2Int pos : {Vec2Int (entity.position.x + properties.size    , entity.position.y + properties.size - 1),
                                    Vec2Int (entity.position.x + properties.size - 1, entity.position.y + properties.size    ),
                                    Vec2Int (entity.position.x + properties.size    , entity.position.y + properties.size - 2),
                                    Vec2Int (entity.position.x + properties.size - 2, entity.position.y + properties.size    ),
                                    Vec2Int (entity.position.x + properties.size    , entity.position.y + properties.size - 3),
                                    Vec2Int (entity.position.x + properties.size - 3, entity.position.y + properties.size    ),
                                    Vec2Int (entity.position.x + properties.size    , entity.position.y + properties.size - 4),
                                    Vec2Int (entity.position.x + properties.size - 4, entity.position.y + properties.size    ),
                                    Vec2Int (entity.position.x + properties.size    , entity.position.y + properties.size - 5),
                                    Vec2Int (entity.position.x + properties.size - 5, entity.position.y + properties.size    ),
                                    Vec2Int (entity.position.x + properties.size - 1, entity.position.y + properties.size - 1),
                                    Vec2Int (entity.position.x +                 - 1, entity.position.y + properties.size - 1),
                                    Vec2Int (entity.position.x + properties.size - 2, entity.position.y + properties.size - 1),
                                    Vec2Int (entity.position.x +                 - 1, entity.position.y + properties.size - 2),
                                    Vec2Int (entity.position.x + properties.size - 3, entity.position.y + properties.size - 1),
                                    Vec2Int (entity.position.x +                 - 1, entity.position.y + properties.size - 3),
                                    Vec2Int (entity.position.x + properties.size - 4, entity.position.y + properties.size - 1),
                                    Vec2Int (entity.position.x +                 - 1, entity.position.y + properties.size - 4),
                                    Vec2Int (entity.position.x + properties.size - 5, entity.position.y + properties.size - 1),
                                    Vec2Int (entity.position.x +                 - 1, entity.position.y + properties.size - 5)})
            {
              if (is_place_free (pos))
                {
                  best_pos = pos;
                  break;
                }
            }
          if (!is_correct (best_pos))
            continue;

          buildAction = std::shared_ptr<BuildAction> (new BuildAction (buildType, best_pos));
          make_busy (entity);
          buy_entity (buildType);
        }

      result->entityActions[entity.id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
    }
}

bool game_step_t::is_building (const EntityType type) const
{
  switch (type)
    {
      case TURRET:
      case WALL:
      case HOUSE:
      case BUILDER_BASE:
      case MELEE_BASE:
      case RANGED_BASE:
        return true;
      case BUILDER_UNIT:
      case MELEE_UNIT:
      case RANGED_UNIT:
      case RESOURCE:
        return false;
      default: break;
    }
  return false;
}

std::vector<Vec2Int> game_step_t::get_poses_around (const Vec2Int pos) const
{
  return {Vec2Int (pos.x    , pos.y - 1), Vec2Int (pos.x - 1, pos.y    ),
          Vec2Int (pos.x + 1, pos.y    ), Vec2Int (pos.x    , pos.y + 1)};
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

      Entity *aim_tree = nullptr;
      int dis = get_max_distance ();
      for (Entity &tree : get_vector (RESOURCE))
        {
          if (tree_was.count (tree.id))
            continue;
          int temp_dis = get_distance (entity.position, tree.position);
          if (dis > temp_dis)
            {
              dis = temp_dis;
              aim_tree = &tree;
            }
        }

      std::shared_ptr<MoveAction>   moveAction   = nullptr;
      std::shared_ptr<AttackAction> atackAction  = nullptr;
      if (aim_tree)
        {
          moveAction  = std::shared_ptr<MoveAction> (new MoveAction (aim_tree->position, true, true));
          atackAction = std::shared_ptr<AttackAction> (new AttackAction (std::shared_ptr<int> (new int (aim_tree->id)), nullptr));
          tree_was.insert (aim_tree->id);
        }
      else
        {
          moveAction  = std::shared_ptr<MoveAction> (new MoveAction (Vec2Int (rand () % playerView->mapSize, rand() % playerView->mapSize), true, true));
          atackAction = std::shared_ptr<AttackAction> (new AttackAction (nullptr, std::shared_ptr<AutoAttack> (new AutoAttack (properties.sightRange, {WALL, HOUSE, BUILDER_BASE, BUILDER_UNIT, MELEE_BASE,  MELEE_UNIT, RANGED_BASE, RANGED_UNIT, RESOURCE, TURRET}))));
        }

      make_busy (entity.id);
      result->entityActions[entity.id] = EntityAction (moveAction, nullptr, atackAction, nullptr);
    }
}

void game_step_t::attack_in_zone ()
{
  std::unordered_map<int, int> enemy_hurt; // max damage
  std::unordered_map<int, std::unordered_set<int>> enemy_to_us; // enemy_id -> our_ids   who  can attack
  std::unordered_map<int, std::unordered_set<int>> we_to_enemy; // our_id   -> enemy_ids whom can attack
  for (const EntityType type : {RANGED_UNIT, MELEE_UNIT, TURRET})
    {
      const EntityProperties &prop = playerView->entityProperties.at (type);
      for (const EntityType enemy_type : {RANGED_UNIT, MELEE_UNIT, TURRET})
        {
          const EntityProperties &enemy_prop = playerView->entityProperties.at (enemy_type);
          for (const Entity &entity : get_vector (type))
            {
              if (is_busy (entity))
                continue;
              for (const Entity &enemy : get_enemy_vector (enemy_type))
                {
                  if (enemy.health <= 0 || !entity.active || get_distance (entity, enemy) > prop.attack->attackRange)
                    continue;
                  enemy_hurt[enemy.id] += prop.attack->damage;
                  enemy_to_us[enemy.id].insert (entity.id);
                  we_to_enemy[entity.id].insert (enemy.id);
                }
            }
        }
    }

  // 1 step: KILL EVERYONE WHOM CAN
  // 2 step: HURT OTHERS
  for (int step = 0; step < 2; ++step)
    {
      std::vector<int> enemy_ids_sort;
      enemy_ids_sort.reserve (enemy_to_us.size ());
      std::transform (enemy_to_us.begin (), enemy_to_us.end (), std::back_inserter (enemy_ids_sort),
                     [&enemy_ids_sort] (const std::pair<int, const std::unordered_set<int>> &a) { return a.first; });
      std::sort (enemy_ids_sort.begin (), enemy_ids_sort.end (),
                [&] (const int id_a, const int id_b)
                  {
                      const int min_life_stay_a = get_entity_by_id (id_a).health - enemy_hurt[id_a];
                      const int min_life_stay_b = get_entity_by_id (id_b).health - enemy_hurt[id_b];
                      return min_life_stay_a < min_life_stay_b;
                  });
      for (const int enemy_id : enemy_ids_sort)
        {
          std::unordered_set<int> &our_ids = enemy_to_us[enemy_id];
          Entity &enemy = get_entity_by_id (enemy_id);
          if (enemy.health <= 0)
            continue; // smth wrong
          if (step == 0 && enemy_hurt[enemy_id] < enemy.health)
            continue;

          std::vector<int> our_ids_sort;
          our_ids_sort.reserve (our_ids.size ());
          std::copy (our_ids.begin (), our_ids.end (), std::back_inserter (our_ids_sort));
          std::sort (our_ids_sort.begin (), our_ids_sort.end (),
                    [&] (const int id_a, const int id_b)
                      {
                         const int cnt_of_a_enemies = we_to_enemy[id_a].size ();
                         const int cnt_of_b_enemies = we_to_enemy[id_b].size ();
                         return cnt_of_a_enemies < cnt_of_b_enemies;
                         // TO-DO: add order by other objects
                      });

          for (const int id : our_ids_sort)
            {
              if (is_busy (id))
                continue; // smth wrong
              const Entity &entity = get_entity_by_id (id);
              const EntityProperties &prop = playerView->entityProperties.at (entity.entityType);
              for (const int enemy_id_ : we_to_enemy[id])
                {
                  enemy_to_us[enemy_id_].erase (id);
                  enemy_hurt[enemy_id_] -= prop.attack->damage;
                }
              we_to_enemy.erase (id);

              std::shared_ptr<AttackAction> atackAction  = std::shared_ptr<AttackAction> (new AttackAction (std::shared_ptr<int> (new int (enemy_id)), nullptr));
              result->entityActions[id] = EntityAction (nullptr, nullptr, atackAction, nullptr);
              make_busy (id);
              enemy.health -= prop.attack->damage;

              if (enemy.health <= 0)
                {
                  set_map_damage (enemy, false);
                  enemy_to_us.erase (enemy_id);
                  break;
                }
            }
        }
      for (auto it = enemy_to_us.begin (); it != enemy_to_us.end ();)
        {
          if (it->second.empty ())
            it = enemy_to_us.erase (it);
          else
            ++it;
        }
    }

  recalculate_map_run ();
}

void game_step_t::attack_step_back ()
{
  const EntityType type = RANGED_UNIT;
  const EntityProperties &prop = playerView->entityProperties.at (type);
  for (const Entity &entity : get_vector (type))
    {
      Vec2Int pos = entity.position;
      if (   is_busy (entity)
          || map_run[pos.x][pos.y] == 0
          || (entity.position.x <= MIN_POSITION_RANGED_DO_STEP_BACK && entity.position.y <= MIN_POSITION_RANGED_DO_STEP_BACK)) // TO-DO: exclude it
        continue;

      std::shared_ptr<MoveAction>   moveAction   = nullptr;
      std::shared_ptr<AttackAction> atackAction  = nullptr;
      if (map_run[pos.x][pos.y] == 3)
        pos = pos; // smth wrong
      if (map_run[pos.x][pos.y] == 2)
        {
          Vec2Int best_pos = get_step_back_for_entity (entity, map_run[pos.x][pos.y]);
          if (is_correct (best_pos))
            {
              moveAction   = std::shared_ptr<MoveAction> (new MoveAction (best_pos, false, true));
              map[best_pos.x][best_pos.y] = entity.entityType + 10;
            }
        }
      else if (map_run[pos.x][pos.y] == 1)
        {
        }

      result->entityActions[entity.id] = EntityAction (moveAction, nullptr, atackAction, nullptr);
      make_busy (entity.id);
    }
}

void game_step_t::attack_step_back_from_melee ()
{
  const EntityType type = RANGED_UNIT;
  const EntityProperties &prop = playerView->entityProperties.at (type);
  for (const Entity &entity : get_vector (type))
    {
      Vec2Int pos = entity.position;
      if (   is_busy (entity)
          || map_run_melee[pos.x][pos.y] < 2
          || (entity.position.x <= MIN_POSITION_RANGED_DO_STEP_BACK && entity.position.y <= MIN_POSITION_RANGED_DO_STEP_BACK)) // TO-DO: exclude it
        continue;

      std::shared_ptr<MoveAction>   moveAction   = nullptr;
      std::shared_ptr<AttackAction> atackAction  = nullptr;
      if (map_run_melee[pos.x][pos.y] == 2)
        {
          Vec2Int best_pos = get_step_back_for_entity (entity, map_run_melee[pos.x][pos.y]); // use the same func as map_run
          if (is_correct (best_pos))
            {
              moveAction   = std::shared_ptr<MoveAction> (new MoveAction (best_pos, false, true));
              map[best_pos.x][best_pos.y] = entity.entityType + 10;
            }
        }

      result->entityActions[entity.id] = EntityAction (moveAction, nullptr, atackAction, nullptr);
      make_busy (entity.id);
    }
}

void game_step_t::builder_step_back ()
{
  const EntityType type = BUILDER_UNIT;
  const EntityProperties &prop = playerView->entityProperties.at (type);
  for (const Entity &entity : get_vector (type))
    {
      Vec2Int pos = entity.position;
      const int danger_lv = map_run[pos.x][pos.y];
      if (is_busy (entity) || danger_lv == 0)
        continue;

      Vec2Int best_pos = get_step_back_for_entity (entity, danger_lv);
      if (is_correct (best_pos))
        {
          std::shared_ptr<MoveAction>   moveAction   = std::shared_ptr<MoveAction> (new MoveAction (best_pos, false, true));
          result->entityActions[entity.id] = EntityAction (moveAction, nullptr, nullptr, nullptr);
          make_busy (entity.id);
          map[best_pos.x][best_pos.y] = entity.entityType + 10;
        }
      else
        {
          // try to do anything
          std::shared_ptr<AttackAction> atackAction  = nullptr;
          std::shared_ptr<RepairAction> repairAction = nullptr;
          for (const Vec2Int p : get_poses_around (pos))
            {
              if (!is_correct (p))
                continue;
              const int p_id = map_id[p.x][p.y];
              if (p_id == -1)
                continue;
              Entity &entity = get_entity_by_id (p_id);
              if (   is_place_contain (p.x, p.y, RESOURCE)
                  || (is_place_contain_enemy (p.x, p.y) && entity.health > 0))
                {
                  atackAction = std::shared_ptr<AttackAction> (new AttackAction (std::shared_ptr<int> (new int (p_id)), nullptr));
                  break;
                }
              if (is_place_contain_us (p.x, p.y))
                {
                  const EntityProperties &p_prop = playerView->entityProperties.at (type);
                  if (entity.health < p_prop.maxHealth)
                    {
                      repairAction = std::shared_ptr<RepairAction> (new RepairAction (p_id));
                      break;
                    }
                }
            }
          if (!atackAction && !repairAction)
            atackAction = std::shared_ptr<AttackAction> (new AttackAction (nullptr, std::shared_ptr<AutoAttack> (new AutoAttack (prop.sightRange, {}))));

          result->entityActions[entity.id] = EntityAction (nullptr, nullptr, atackAction, nullptr);
          make_busy (entity.id);
        }
    }
}

void game_step_t::attack_out_zone ()
{
  std::unordered_map<int, std::unordered_set<int>> enemy_near_us[3]; // enemy_id -> our_ids   who already stay in %d zone
  std::unordered_map<int, std::unordered_set<int>> we_near_enemy[3]; // our_id   -> enemy_ids who already stay in %d zone
  std::unordered_map<int, std::unordered_set<int>> enemy_near_us_free[3]; // enemy_id -> our_ids   who already stay in %d zone and can go forward
  std::unordered_map<int, std::unordered_set<int>> we_near_enemy_free[3]; // our_id   -> enemy_ids who already stay in %d zone and can go forward

  const EntityType type = RANGED_UNIT;
  const EntityProperties &prop = playerView->entityProperties.at (type);
  for (const Entity &entity : get_vector (type))
    {
      if (map_run[entity.position.x][entity.position.y] == 0 || map_run[entity.position.x][entity.position.y] == 3)
        continue;
      for (const EntityType enemy_type : {RANGED_UNIT, TURRET})
        {
          const EntityProperties &enemy_prop = playerView->entityProperties.at (enemy_type);
          for (const Entity &enemy : get_enemy_vector (enemy_type))
            {
              const int dist = get_distance (entity, enemy);
              if (enemy.health <= 0 || !entity.active || dist > prop.attack->attackRange + 2)
                continue;
              int lv = 0;
              if (dist == prop.attack->attackRange + 2)
                lv = 1;
              else
                lv = 2;
              enemy_near_us[lv][enemy.id].insert (entity.id);
              we_near_enemy[lv][entity.id].insert (enemy.id);
              
              if (is_busy (entity) || map_run_melee[entity.position.x][entity.position.y] != 0)
                continue;
              for (const Vec2Int p : get_poses_around (entity.position))
                {
                  if (   !is_place_free (p)
                      || (enemy_type == TURRET && get_build_distance (enemy.id, p) >= dist)
                      || (enemy_type != TURRET && get_distance (p, enemy.position) >= dist))
                    continue;
                  enemy_near_us_free[lv][enemy.id].insert (entity.id);
                  we_near_enemy_free[lv][entity.id].insert (enemy.id);
                  break;
                }
            }
        }
    }
  
  std::vector<int> enemy_ids_sort;
  enemy_ids_sort.reserve (enemy_near_us_free[1].size () + enemy_near_us_free[2].size ());
  std::transform (enemy_near_us_free[1].begin (), enemy_near_us_free[1].end (), std::back_inserter (enemy_ids_sort),
                  [&enemy_ids_sort] (const std::pair<int, const std::unordered_set<int>> &a) { return a.first; });
  std::sort (enemy_ids_sort.begin (), enemy_ids_sort.end (),
            [&] (const int id_a, const int id_b)
              {
                  const int min_life_stay_a = get_entity_by_id (id_a).health - enemy_near_us_free[2][id_a].size () * 5 - enemy_near_us_free[1][id_a].size () * 5;
                  const int min_life_stay_b = get_entity_by_id (id_b).health - enemy_near_us_free[2][id_a].size () * 5 - enemy_near_us_free[1][id_a].size () * 5;
                  return min_life_stay_a < min_life_stay_b;
              });
  std::transform (enemy_near_us_free[2].begin (), enemy_near_us_free[2].end (), std::back_inserter (enemy_ids_sort),
                  [&enemy_ids_sort] (const std::pair<int, const std::unordered_set<int>> &a) { return a.first; });
  std::sort (enemy_ids_sort.begin () + enemy_near_us_free[1].size (), enemy_ids_sort.end (),
            [&] (const int id_a, const int id_b)
              {
                  const int min_life_stay_a = get_entity_by_id (id_a).health - enemy_near_us_free[2][id_a].size () * 5;
                  const int min_life_stay_b = get_entity_by_id (id_b).health - enemy_near_us_free[2][id_a].size () * 5;
                  return min_life_stay_a < min_life_stay_b;
              });
  
  for (const int enemy_id : enemy_ids_sort)
    {
      Entity &enemy = get_entity_by_id (enemy_id);
      const int min_life_stay_1 = enemy.health - enemy_near_us_free[2][enemy_id].size () * 5 - enemy_near_us_free[1][enemy_id].size () * 5;
      if (min_life_stay_1 > 0)
        {
          if (enemy.entityType == TURRET)
            {
              if (enemy_near_us[2][enemy_id].size () + enemy_near_us_free[1][enemy_id].size () < 5)
                continue;
            }
          else
            continue;
        }

      for (const int id : enemy_near_us_free[1][enemy_id])
        {
          Entity &entity = get_entity_by_id (id);
          const int dist = get_distance (entity, enemy);
          Vec2Int best_pos = INCORRECT_VEC2INT;
          for (const Vec2Int p : get_poses_around (entity.position))
            {
              if (   !is_place_free (p)
                  || (enemy.entityType == TURRET && get_build_distance (enemy.id, p) >= dist)
                  || (enemy.entityType != TURRET && get_distance (p, enemy.position) >= dist))
                continue;
              best_pos = p;
              break;
            }
          if (!is_place_free (best_pos))
            continue;
          std::shared_ptr<MoveAction>   moveAction   = std::shared_ptr<MoveAction> (new MoveAction (best_pos, false, true));
          result->entityActions[entity.id] = EntityAction (moveAction, nullptr, nullptr, nullptr);
          make_busy (entity.id);
          map[best_pos.x][best_pos.y] = entity.entityType + 10;
        }

      
      const int min_life_stay_2 = enemy.health - enemy_near_us_free[2][enemy_id].size () * 5;
      if (min_life_stay_2 > 0)
        {
          if (enemy.entityType == TURRET)
            {
              if (enemy_near_us_free[2][enemy_id].size () < 5)
                continue;
            }
          else
            continue;
        }

      for (const int id : enemy_near_us_free[2][enemy_id])
        {
          Entity &entity = get_entity_by_id (id);
          const int dist = get_distance (entity, enemy);
          Vec2Int best_pos = INCORRECT_VEC2INT;
          for (const Vec2Int p : get_poses_around (entity.position))
            {
              if (   !is_place_free (p)
                  || (enemy.entityType == TURRET && get_build_distance (enemy.id, p) >= dist)
                  || (enemy.entityType != TURRET && get_distance (p, enemy.position) >= dist))
                continue;
              best_pos = p;
              break;
            }
          if (!is_place_free (best_pos))
            continue;
          std::shared_ptr<MoveAction>   moveAction   = std::shared_ptr<MoveAction> (new MoveAction (best_pos, false, true));
          result->entityActions[entity.id] = EntityAction (moveAction, nullptr, nullptr, nullptr);
          make_busy (entity.id);
          map[best_pos.x][best_pos.y] = entity.entityType + 10;
        }

    }
}

void game_step_t::attack_others ()
{
  // HERE SHOULD STAY ONLY INACTIVE TURRETS
  for (const EntityType enemy_type : {TURRET, BUILDER_UNIT, MELEE_BASE, RANGED_BASE, BUILDER_BASE, HOUSE, WALL})
    {
      const EntityProperties &ememy_prop = playerView->entityProperties.at (enemy_type);
      for (const EntityType type : {MELEE_UNIT, RANGED_UNIT, TURRET, BUILDER_UNIT})
        {
          const EntityProperties &prop = playerView->entityProperties.at (type);
          for (const Entity &entity : get_vector (type))
            {
              if (is_busy (entity))
                continue;
              for (Entity &enemy : get_enemy_vector (enemy_type))
                {
                  if (enemy.health <= 0 || get_distance (entity, enemy) > prop.attack->attackRange)
                    continue;

                  std::shared_ptr<AttackAction> atackAction  = std::shared_ptr<AttackAction> (new AttackAction (std::shared_ptr<int> (new int (enemy.id)), nullptr));
                  result->entityActions[entity.id] = EntityAction (nullptr, nullptr, atackAction, nullptr);
                  make_busy (entity.id);
                  enemy.health -= prop.attack->damage;
                }
            }
        }
    }
}

void game_step_t::move_army (const EntityType type)
{
  const EntityProperties &properties = playerView->entityProperties.at (type);

  int dir = choose_atack_pos ();
  for (const Entity &entity : get_vector (type))
    {
      if (is_busy (entity))
        continue;

      std::shared_ptr<MoveAction>   moveAction   = nullptr;
      std::shared_ptr<BuildAction>  buildAction  = nullptr;
      std::shared_ptr<AttackAction> atackAction  = std::shared_ptr<AttackAction> (new AttackAction (nullptr, std::shared_ptr<AutoAttack> (new AutoAttack (properties.sightRange, {}))));
      std::shared_ptr<RepairAction> repairAction = nullptr;

      if (!enemy_near_base_ids.empty ())
        {
          Vec2Int pos = INCORRECT_VEC2INT;
          int dis = get_max_distance ();
          for (const int enemy_id : enemy_near_base_ids)
            {
              Entity &enemy = get_entity_by_id (enemy_id);
              int new_dis = get_distance (entity.position, enemy.position);
              if (dis > new_dis)
                {
                  pos = enemy.position;
                  dis = new_dis;
                }
            }
          moveAction = std::shared_ptr<MoveAction> (new MoveAction (pos, true, true));
        }
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
      make_busy (entity.id);
    }
}

void game_step_t::turn_on_turrets ()
{
  const EntityType type = TURRET;
  const EntityProperties &properties = playerView->entityProperties.at (type);

  for (const Entity &entity : get_vector (type))
    {
      if (is_busy (entity) || !entity.active)
        continue;
      std::shared_ptr<MoveAction>   moveAction   = nullptr;
      std::shared_ptr<BuildAction>  buildAction  = nullptr;
      std::shared_ptr<AttackAction> atackAction  = std::shared_ptr<AttackAction> (new AttackAction (nullptr, std::shared_ptr<AutoAttack> (new AutoAttack (properties.sightRange, {}))));
      std::shared_ptr<RepairAction> repairAction = nullptr;

      result->entityActions[entity.id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
    }
}

void game_step_t::heal_nearest ()
{
  const EntityType healer_type = BUILDER_UNIT;

  for (const Entity &healer : get_vector (healer_type))
    {
      if (is_busy (healer))
        continue;

      for (const Vec2Int repair_pos : get_poses_around (healer.position))
        {
          if (!is_correct (repair_pos))
            continue;
          const int repair_id = map_id[repair_pos.x][repair_pos.y];
          if (repair_id < 0)
            continue;
          const Entity &repair_entity = get_entity_by_id (repair_id);
          if (   (repair_entity.playerId == nullptr || *repair_entity.playerId != m_id)
              || (repair_entity.entityType != RANGED_UNIT && repair_entity.entityType != MELEE_UNIT && repair_entity.entityType != BUILDER_UNIT))
            continue;
          const EntityProperties &p = playerView->entityProperties.at (repair_entity.entityType);
          if (repair_entity.health < p.maxHealth)
            {
              std::shared_ptr<MoveAction>   moveAction   = nullptr;
              std::shared_ptr<BuildAction>  buildAction  = nullptr;
              std::shared_ptr<AttackAction> atackAction  = nullptr;
              std::shared_ptr<RepairAction> repairAction = std::shared_ptr<RepairAction> (new RepairAction (repair_entity.id));

              make_busy (healer.id);
              result->entityActions[healer.id] = EntityAction (moveAction, buildAction, atackAction, repairAction);
              break;
            }
        }
    }
}

void game_step_t::run_tasks ()
{
  std::unordered_set<int> finished;
  bool need_redirect = false;
  Vec2Int vec_old (0,0), vec_new (0, 0);
  for (auto &task : game_step_t::attack_move_tasks)
    {
      int id = task.first;
      if (!m_entity_by_id.count (id))
        {
          finished.insert (id);
          continue;
        }
      if (is_busy (id))
        continue;

      const Entity &entity = get_entity_by_id (id);
      Vec2Int pos = task.second;

      if (get_distance (entity.position, pos) <= 1)
        {
          int old_dir = get_attack_pos_id_by_vec (pos);
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
        }

      if (map_run[entity.position.x][entity.position.y] != 0)
        pos = pos; // smth wrong

      const EntityProperties &properties = playerView->entityProperties.at (get_entity_by_id (id).entityType);
      std::shared_ptr<MoveAction>   moveAction   = std::shared_ptr<MoveAction> (new MoveAction (pos, true, true));
      // we must not (!) attack everybody after attack_in_zone, they can be already dead, we should make a step or attack others (not melee, arch, turret)
      // TO-DO: make up how to do it (we can't just exclude them from list, we need to hunt them out of attack zone)
      std::shared_ptr<AttackAction>  atackAction = std::shared_ptr<AttackAction> (new AttackAction (nullptr, std::shared_ptr<AutoAttack> (new AutoAttack (properties.sightRange, {}))));
      result->entityActions[entity.id] = EntityAction (moveAction, nullptr, atackAction, nullptr);
      make_busy (id);
    }
  for (const int id : finished)
    game_step_t::attack_move_tasks.erase (id);
  if (need_redirect)
    redirect_all_atack_move_tasks (vec_old, vec_new);
}

std::vector<Vec2Int> game_step_t::get_all_poses_in_area_of_entity (const Entity &entity, const int zone_size) const
{
  std::vector<Vec2Int> res;

  const EntityProperties &p = playerView->entityProperties.at (entity.entityType);
  res.reserve (static_cast <size_t> (p.size) * p.size + static_cast <size_t> (1 + zone_size) * zone_size / 2);

  for (int x = std::max (0, entity.position.x - zone_size); x < std::min (playerView->mapSize - 1, entity.position.x + p.size + zone_size); ++x)
    for (int y = entity.position.y; y < entity.position.y + p.size; ++y)
      res.push_back (Vec2Int (x, y));
  for (int x = entity.position.x; x < entity.position.x + p.size; ++x)
    {
      for (int y = std::max (0, entity.position.y - zone_size); y < entity.position.y; ++y)
        res.push_back (Vec2Int (x, y));
      for (int y = entity.position.y + p.size; y < std::min (playerView->mapSize - 1, entity.position.y + p.size + zone_size); ++y)
        res.push_back (Vec2Int (x, y));
    }
  for (int k = 1, x = entity.position.x + p.size; x < std::min (playerView->mapSize - 1, entity.position.x + p.size + zone_size - 1); ++x, ++k)
    {
      for (int y = entity.position.y - 1; y >= std::max (0, entity.position.y - zone_size + k); --y)
        res.push_back (Vec2Int (x, y));
      for (int y = entity.position.y + p.size; y < std::min (playerView->mapSize - 1, entity.position.y + p.size + zone_size - k); ++y)
        res.push_back (Vec2Int (x, y));
    }
  for (int k = 1, x = entity.position.x - 1; x >= std::max (0, entity.position.x - zone_size + 1); --x, ++k)
    {
      for (int y = entity.position.y - 1; y >= std::max (0, entity.position.y - zone_size + k); --y)
        res.push_back (Vec2Int (x, y));
      for (int y = entity.position.y + p.size; y < std::min (playerView->mapSize - 1, entity.position.y + p.size + zone_size - k); ++y)
        res.push_back (Vec2Int (x, y));
    }

  return res;
}

void game_step_t::make_atack_groups ()
{
  if ((get_army_count () - game_step_t::attack_move_tasks.size () > 15 || !game_step_t::destroyed_pos.empty ())
    && enemy_near_base_ids.empty ())
    {
      int dir = choose_atack_pos ();
      if (dir < 0)
        return;
      for (const Entity &entity : get_vector (RANGED_UNIT))
        {
          if (rand () % 10 == 0 || is_busy (entity)) // 90%
            continue;
          
          const EntityProperties &properties = playerView->entityProperties.at (entity.entityType);
          std::shared_ptr<MoveAction>   moveAction   = std::shared_ptr<MoveAction> (new MoveAction (attack_pos[dir], true, true));
          std::shared_ptr<AttackAction> atackAction  = std::shared_ptr<AttackAction> (new AttackAction (nullptr, std::shared_ptr<AutoAttack> (new AutoAttack (properties.sightRange, {}))));

          result->entityActions[entity.id] = EntityAction (moveAction, nullptr, atackAction, nullptr);
          game_step_t::attack_move_tasks[entity.id] = attack_pos[dir];
          make_busy (entity.id);
        }
      for (const Entity &entity : get_vector (MELEE_UNIT))
        {
          if (rand () % 20 == 0 || is_busy (entity)) // 95%
            continue;

          const EntityProperties &properties = playerView->entityProperties.at (entity.entityType);
          std::shared_ptr<MoveAction>   moveAction   = std::shared_ptr<MoveAction> (new MoveAction (attack_pos[dir], true, true));
          std::shared_ptr<AttackAction> atackAction  = std::shared_ptr<AttackAction> (new AttackAction (nullptr, std::shared_ptr<AutoAttack> (new AutoAttack (properties.sightRange, {}))));

          result->entityActions[entity.id] = EntityAction (moveAction, nullptr, atackAction, nullptr);
          game_step_t::attack_move_tasks[entity.id] = attack_pos[dir];
          make_busy (entity.id);
        }
    }
}

void game_step_t::redirect_all_atack_move_tasks (const Vec2Int old_pos, const Vec2Int new_pos)
{
  for (auto &task : game_step_t::attack_move_tasks)
    {
      const Vec2Int pos = task.second;
      if (pos == old_pos)
        task.second = new_pos;
    }
}


int game_step_t::get_attack_pos_id_by_vec (const Vec2Int pos) const
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

int game_step_t::choose_atack_pos (const Vec2Int old_pos)
{
  if (destroyed_pos.size () == attack_pos.size ())
    destroyed_pos.clear ();

  if (is_correct (old_pos))
    {
      int old_dir = get_attack_pos_id_by_vec (old_pos);
      if ((old_dir == 0 || old_dir == 2) && !destroyed_pos.count (1))
        return 1;
    }

  if (!is_correct (old_pos) && (!destroyed_pos.count (0) || !destroyed_pos.count (2)))
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
