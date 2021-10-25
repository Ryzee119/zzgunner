// SPDX-License-Identifier: MIT
#include "common.h"

static map_tileset_t map_tileset[MAX_MAP_TILES];
static int loaded_map_width;
static int loaded_map_height;
static int loaded_map_num_tiles;
static int loaded_level;
static bool *control_pad[3];

static const char *map_files[] =
{
    "",
    "rom:/map1.json",
    "rom:/map2.json",
    "rom:/map3.json",
    "rom:/map4.json",
    "rom:/map5.json",
};

static void item_callback(game_sprite_t *this, game_sprite_t *that)
{
    if (that->type != TYPE_PLAYER)
        return;
    if (this->type != TYPE_ITEM)
        return;
    if (this->destroy || that->destroy)
        return;

    item_data_t *item = (item_data_t *)this->user_data;
    player_data_t *pdata = (player_data_t *)that->user_data;

    if (item->weapon_index > 0)
    {
        if (pdata->current_weapon != item->weapon_index)
        {
            pdata->current_weapon = item->weapon_index;
            pdata->ammo = item->ammo_cnt;
        }
        else
        {
            pdata->ammo += item->ammo_cnt;
        }
        sfx_play(SFX_PICKUP_ITEM);
        this->destroy = true;
        return;
    }
    if (item->medi_chunks && pdata->health < PLAYER_MAX_HEALTH)
    {
        pdata->health += item->medi_chunks;

        if (pdata->health > PLAYER_MAX_HEALTH)
        {
            pdata->health = PLAYER_MAX_HEALTH;
        }
        sfx_play(SFX_PICKUP_ITEM);
        this->destroy = true;
        return;
    }
    if (item->is_trophy)
    {
        player_increment_trophy_count();
        sfx_play(SFX_CONTROLPAD);
        this->destroy = true;
    }
}

static void control_pad_callback(game_sprite_t *this, game_sprite_t *that)
{
    if (that->type != TYPE_PLAYER)
        return;
    this->visible = true;
    bool *state = (bool *)this->user_data;
    if (!*state)
    {
        sfx_play(SFX_CONTROLPAD);
        *state = true;
    }
}

static void exit_callback(game_sprite_t *this, game_sprite_t *that)
{
    if (that->type != TYPE_PLAYER)
        return;

    if (*control_pad[0] && *control_pad[1] && *control_pad[2])
    {
        set_level(loaded_level + 1);
        debugf("level %d\n", loaded_level + 1);
    }
}

static void create_medipak(int x, int y, int health_bars)
{
    item_data_t *item_data = malloc(sizeof(item_data_t));
    memset(item_data, 0x00, sizeof(item_data_t));
    item_data->medi_chunks = health_bars;

    game_sprite_t *object = game_sprite_create_static(sprite_load_from_png("rom:/misc/medikit.png"), TYPE_ITEM,
                                                       x, y, 100, 100, false, false);
    object->user_data = item_data;
    object->collision_callback = item_callback;
}

static void create_weaponbox(int x, int y, int weapon, int ammo)
{
    item_data_t *item_data = malloc(sizeof(item_data_t));
    memset(item_data, 0x00, sizeof(item_data_t));
    item_data->weapon_index = weapon;
    item_data->ammo_cnt = ammo;

    const char *path = (weapon == WEAPON_AUTOMATIC) ? "rom:/misc/auto.png" : "rom:/misc/shotgun.png";
    game_sprite_t *object = game_sprite_create_static(sprite_load_from_png(path), TYPE_ITEM,
                                                       x, y, 100, 100, false, false);
    object->user_data = item_data;
    object->collision_callback = item_callback;
}

static void create_trophy(int x, int y)
{
    item_data_t *item_data = malloc(sizeof(item_data_t));
    memset(item_data, 0x00, sizeof(item_data_t));
    item_data->is_trophy = true;
    game_sprite_t *object = game_sprite_create_static(
        sprite_load_from_png("rom:/misc/trophy.png"),
        TYPE_ITEM, x, y, 100, 100, false, false
    );
    object->user_data = item_data;
    object->collision_callback = item_callback;
}

static void create_controlpad(int x, int y, int index)
{
    assertf(index < 3, "Currently only support 3 control pad in a level\n");
    game_sprite_t *object = game_sprite_create_static(sprite_load_from_png("rom:/room/tile002.png"), TYPE_MAPOBJECT,
                                           x, y, 100, 100, false, false);
    object->user_data = control_pad[index];
    object->visible = false;
    object->collision_callback = control_pad_callback;
}

static void create_exitpad(int x, int y, int dir)
{
    const char* path = (dir == 0) ? "rom:/room/tile000.png" : //Up
                       (dir == 1) ? "rom:/room/tile001.png" : //Down
                       (dir == 2) ? "rom:/room/tile014.png" : //Left
                       (dir == 3) ? "rom:/room/tile015.png" : "";//Right
    game_sprite_t *object = game_sprite_create_static(sprite_load_from_png(path), TYPE_MAPOBJECT,
                                                   x, y, 100, 100, false, false);
    object->collision_callback = exit_callback;
}

static void create_fire(int x, int y, int scale)
{
    game_sprite_t *object = game_sprite_create(BACKGROUND_LAYER, TYPE_MAPOBJECT, x, y, scale, scale, rand() % 1, false);
    uint8_t fire_anim[8];
    fire_anim[0] = sprite_load_from_png("rom:/misc/fire_fx1/tile000.png");
    fire_anim[1] = sprite_load_from_png("rom:/misc/fire_fx1/tile001.png");
    fire_anim[2] = sprite_load_from_png("rom:/misc/fire_fx1/tile002.png");
    fire_anim[3] = sprite_load_from_png("rom:/misc/fire_fx1/tile003.png");
    fire_anim[4] = sprite_load_from_png("rom:/misc/fire_fx1/tile004.png");
    fire_anim[5] = sprite_load_from_png("rom:/misc/fire_fx1/tile005.png");
    fire_anim[6] = sprite_load_from_png("rom:/misc/fire_fx1/tile006.png");
    fire_anim[7] = sprite_load_from_png("rom:/misc/fire_fx1/tile007.png");
    game_sprite_add_animation(object, fire_anim, 8, 100, true);
}

static void create_alien_ship(int x, int y)
{
    game_sprite_create_static(sprite_load_from_png("rom:/misc/alien_ship/tile001.png"),
                              TYPE_MAPOBJECT, x + MAP_TILE_SIZE, y, 100, 100, false, false);
    game_sprite_create_static(sprite_load_from_png("rom:/misc/alien_ship/tile002.png"),
                              TYPE_MAPOBJECT, x, y + MAP_TILE_SIZE, 100, 100, false, false);
    game_sprite_create_static(sprite_load_from_png("rom:/misc/alien_ship/tile003.png"),
                              TYPE_MAPOBJECT, x + MAP_TILE_SIZE, y + MAP_TILE_SIZE, 100, 100, false, false);
    game_sprite_create_static(sprite_load_from_png("rom:/misc/alien_ship/tile004.png"),
                              TYPE_MAPOBJECT, x + 2 * MAP_TILE_SIZE, y + MAP_TILE_SIZE, 100, 100, false, false);
    game_sprite_create_static(sprite_load_from_png("rom:/misc/alien_ship/tile005.png"),
                              TYPE_MAPOBJECT, x, y + 2 * MAP_TILE_SIZE, 100, 100, false, false);
    game_sprite_create_static(sprite_load_from_png("rom:/misc/alien_ship/tile006.png"),
                              TYPE_MAPOBJECT, x + MAP_TILE_SIZE, y + 2 * MAP_TILE_SIZE, 100, 100, false, false);
    game_sprite_create_static(sprite_load_from_png("rom:/misc/alien_ship/tile007.png"),
                              TYPE_MAPOBJECT, x + 2 * MAP_TILE_SIZE, y + 2 * MAP_TILE_SIZE, 100, 100, false, false);
}

void map_startup()
{
    for (int i = 0; i < MAX_MAP_TILES; i++)
    {
        map_tileset[i].sprite_index = -1;
    }
    loaded_map_width = 0;
    loaded_map_height = 0;
    loaded_map_num_tiles = 0;
}

void map_put_player_at_start_pos()
{
    game_sprite_t *player = game_sprite_get_type(TYPE_PLAYER, true);
    if (player == NULL)
        return;

    if (loaded_level == 1)
    {
        game_sprite_set_pos(player, TILE_X(8.5 + 5), TILE_Y(43 + 4));
    }
    else if (loaded_level == 2)
    {
        game_sprite_set_pos(player, TILE_X(7.5), TILE_Y(18));
    }
    else if (loaded_level == 3)
    {
        game_sprite_set_pos(player, TILE_X(20), TILE_Y(12));
    }
    else if (loaded_level == 4)
    {
        game_sprite_set_pos(player, TILE_X(20), TILE_Y(12));
    }
    else if (loaded_level == 5)
    {
        game_sprite_set_pos(player, TILE_X(11), TILE_Y(23));
    }
}

int map_absolute_coord_to_tile(int x, int y)
{
    return (int)(y / MAP_TILE_SIZE) * loaded_map_width + (int)(x / MAP_TILE_SIZE);
}

void map_init(int level)
{
    map_deinit();
    debugf("Current level %d\n", level);
    loaded_level = level;

    cute_tiled_map_t *map = cute_tiled_load_map_from_file(map_files[level], NULL);
    assert(map != NULL);

    loaded_map_width = map->width;
    loaded_map_height = map->height;
    loaded_map_num_tiles = map->width * map->width;

    //FIXME This seems really inefficient
    cute_tiled_layer_t *layer = map->layers;
    while (layer)
    {
        int tile_id = -1;        
        for (int i = 0; i < layer->data_count; i++)
        {
            tile_id = layer->data[i] - 1;
            cute_tiled_tileset_t *tileset = map->tilesets;
            while (tileset)
            {
                cute_tiled_tile_descriptor_t *tiles = tileset->tiles;
                while (tiles)
                {
                    if (tiles->tile_index == tile_id)
                    {
                        char file_path[256];
                        sprintf(file_path, "rom:/%s", tiles->image.ptr);
                        map_tileset[i].sprite_index = sprite_load_from_png(file_path);
                        //debugf("Map tile: %s at %d with sprite %d\n", file_path, i, map_tileset[i].sprite_index);
                        map_tileset[i].x = (i % map->width) * MAP_TILE_SIZE;
                        map_tileset[i].y = (i / map->width) * MAP_TILE_SIZE;
                        cute_tiled_property_t *properties = tiles->properties;
                        if (properties)
                        {
                            if (strcmp(properties->name.ptr, "passable") == 0)
                            {
                                //debugf("tile %d %s passable %d, %d\n", i,  file_path, properties->data.boolean, tile_id);
                                map_tileset[i].passable = (bool)properties->data.boolean;
                            }
                        }
                    }
                    tiles = tiles->next;
                }
                tileset = tileset->next;
            }
        }
        layer = layer->next;
    }

    cute_tiled_free_map(map);
    
    control_pad[0] = malloc(sizeof(bool));
    control_pad[1] = malloc(sizeof(bool));
    control_pad[2] = malloc(sizeof(bool));
    *control_pad[0] = false;
    *control_pad[1] = false;
    *control_pad[2] = false;
    if (level == 1)
    {
        game_sprite_create_static(sprite_load_from_png("rom:/misc/black-hole.png"), TYPE_MAPOBJECT,
                                  TILE_X(4), TILE_Y(25), 100, 100, false, false);

        game_sprite_create_static(sprite_load_from_png("rom:/misc/planet-1.png"), TYPE_MAPOBJECT,
                                  TILE_X(27), TILE_Y(45), 100, 100, false, false);

        game_sprite_create_static(sprite_load_from_png("rom:/misc/planet-2.png"), TYPE_MAPOBJECT,
                                  TILE_X(20), TILE_Y(19), 100, 100, false, false);

        create_fire(TILE_X(14), TILE_Y(46), 100);
        create_fire(TILE_X(10), TILE_Y(15), 100);
        create_fire(TILE_X(28), TILE_Y(23), 100);
        create_fire(TILE_X(3), TILE_Y(38), 100);
        create_fire(TILE_X(24), TILE_Y(15), 100);
        create_fire(TILE_X(1), TILE_Y(20), 100);
        create_fire(TILE_X(23), TILE_Y(32), 100);
        create_fire(TILE_X(17), TILE_Y(41), 100);
        create_fire(TILE_X(12), TILE_Y(43), 100);
 
        create_medipak(TILE_X(18.25), TILE_Y(15.25), 2);

        create_weaponbox(TILE_X(2.25), TILE_Y(33.25), WEAPON_SHOTGUN, 12);
        create_weaponbox(TILE_X(24.25), TILE_Y(42.25), WEAPON_AUTOMATIC, 50);

        enemy_create(TILE_X(13), TILE_Y(38), 200);
        enemy_create(TILE_X(7), TILE_Y(29), 200);
        enemy_create(TILE_X(10), TILE_Y(25), 200);
        enemy_create(TILE_X(12), TILE_Y(17), 200);
        enemy_create(TILE_X(21), TILE_Y(24), 200);
        enemy_create(TILE_X(27), TILE_Y(18), 200);
        enemy_create(TILE_X(30), TILE_Y(21), 200);
        enemy_create(TILE_X(32), TILE_Y(30), 200);
        enemy_create(TILE_X(22), TILE_Y(35), 200);
        enemy_create(TILE_X(22), TILE_Y(41), 200);
        enemy_create(TILE_X(36), TILE_Y(31), 200);
        enemy_create(TILE_X(34), TILE_Y(34.5), 200);
        enemy_create(TILE_X(35), TILE_Y(38), 200);
        enemy_create(TILE_X(39), TILE_Y(38), 200);
        enemy_create(TILE_X(36), TILE_Y(42), 200);
        enemy_create(TILE_X(18), TILE_Y(23), 200);
        enemy_create(TILE_X(21), TILE_Y(16), 200);

        create_controlpad(TILE_X(13), TILE_Y(43), 0);
        create_controlpad(TILE_X(21), TILE_Y(41), 1);
        create_controlpad(TILE_X(27), TILE_Y(16), 2);

        create_exitpad(TILE_X(20), TILE_Y(15), 0);

        create_trophy(TILE_X(31), TILE_Y(42));
    }
    else if (level == 2)
    {
        game_sprite_create_static(sprite_load_from_png("rom:/misc/black-hole.png"), TYPE_MAPOBJECT,
                               TILE_X(15), TILE_Y(28), 100, 100, false, false);

        game_sprite_create_static(sprite_load_from_png("rom:/misc/planet-1.png"), TYPE_MAPOBJECT,
                               TILE_X(21), TILE_Y(36), 100, 100, false, false);

        game_sprite_create_static(sprite_load_from_png("rom:/misc/planet-2.png"), TYPE_MAPOBJECT,
                                TILE_X(20), TILE_Y(19), 150, 150, false, false);

        create_fire(TILE_X(22), TILE_Y(26), 100);
        create_fire(TILE_X(16), TILE_Y(11), 100);
        create_fire(TILE_X(17), TILE_Y(11), 100);
        create_fire(TILE_X(21), TILE_Y(38), 100);
        create_fire(TILE_X(22), TILE_Y(38), 100);
        create_fire(TILE_X(23), TILE_Y(38), 100);
        create_fire(TILE_X(29), TILE_Y(14), 100);
        create_fire(TILE_X(9), TILE_Y(17), 100);
        create_fire(TILE_X(13), TILE_Y(25), 100);

        create_weaponbox(TILE_X(28.25), TILE_Y(42.25), WEAPON_AUTOMATIC, 50);

        create_medipak(TILE_X(33), TILE_Y(24), 2);

        create_controlpad(TILE_X(28), TILE_Y(33), 0);
        create_controlpad(TILE_X(28), TILE_Y(39), 1);
        create_controlpad(TILE_X(28), TILE_Y(45), 2);

        enemy_create(TILE_X(15), TILE_Y(14), 200);
        enemy_create(TILE_X(25), TILE_Y(14), 200);
        enemy_create(TILE_X(22), TILE_Y(40), 200);
        enemy_create(TILE_X(23), TILE_Y(45), 200);
        enemy_create(TILE_X(28), TILE_Y(24), 200);
        enemy_create(TILE_X(18), TILE_Y(15), 200);
        enemy_create(TILE_X(23), TILE_Y(13), 200);
        enemy_create(TILE_X(27), TILE_Y(16), 200);
        enemy_create(TILE_X(25), TILE_Y(21), 200);

        create_exitpad(TILE_X(33), TILE_Y(26), 3);
    }
    else if (level == 3)
    {

        game_sprite_create_static(sprite_load_from_png("rom:/misc/bullet4.png"), TYPE_MAPOBJECT, //Sun :)
                                TILE_X(25), TILE_Y(19), 150, 150, false, false);
                                
        game_sprite_create_static(sprite_load_from_png("rom:/misc/black-hole.png"), TYPE_MAPOBJECT,
                               TILE_X(15), TILE_Y(28), 100, 100, false, false);

        game_sprite_create_static(sprite_load_from_png("rom:/misc/black-hole.png"), TYPE_MAPOBJECT,
                               TILE_X(7), TILE_Y(31), 100, 100, false, false);                      

        game_sprite_create_static(sprite_load_from_png("rom:/misc/planet-1.png"), TYPE_MAPOBJECT,
                               TILE_X(21), TILE_Y(36), 100, 100, false, false);

        game_sprite_create_static(sprite_load_from_png("rom:/misc/planet-2.png"), TYPE_MAPOBJECT,
                                TILE_X(20), TILE_Y(19), 500, 500, false, false);


        create_fire(TILE_X(5), TILE_Y(16), 100);
        create_fire(TILE_X(6), TILE_Y(16), 100);
        create_fire(TILE_X(7), TILE_Y(16), 100);
        create_fire(TILE_X(8), TILE_Y(16), 100);
        create_fire(TILE_X(9), TILE_Y(16), 100);
        create_fire(TILE_X(10), TILE_Y(16), 100);
        create_fire(TILE_X(11), TILE_Y(16), 100);
        create_fire(TILE_X(12), TILE_Y(16), 100);
        create_fire(TILE_X(13), TILE_Y(16), 100);
        create_fire(TILE_X(14), TILE_Y(16), 100);
        create_fire(TILE_X(15), TILE_Y(16), 100);
        create_fire(TILE_X(16), TILE_Y(16), 100);

        create_fire(TILE_X(7), TILE_Y(33), 100);
        create_fire(TILE_X(10), TILE_Y(33), 100);
        create_fire(TILE_X(12), TILE_Y(33), 100);
        create_fire(TILE_X(13), TILE_Y(33), 100);
        create_fire(TILE_X(16), TILE_Y(33), 100);
        create_fire(TILE_X(17), TILE_Y(33), 100);
        create_fire(TILE_X(26), TILE_Y(33), 100);
        create_fire(TILE_X(29), TILE_Y(33), 100);
        create_fire(TILE_X(32), TILE_Y(33), 100);

        create_weaponbox(TILE_X(21.25), TILE_Y(30.25), WEAPON_AUTOMATIC, 50);

        create_medipak(TILE_X(31.25), TILE_Y(15.25), 2);
        create_medipak(TILE_X(29.25), TILE_Y(15.25), 2);

        enemy_create(TILE_X(14), TILE_Y(17), 200);
        enemy_create(TILE_X(4), TILE_Y(19), 200);
        enemy_create(TILE_X(4), TILE_Y(19), 200);
        enemy_create(TILE_X(4), TILE_Y(22), 200);
        enemy_create(TILE_X(10), TILE_Y(34), 200);
        enemy_create(TILE_X(21), TILE_Y(34), 200);
        enemy_create(TILE_X(32), TILE_Y(34), 200);
        enemy_create(TILE_X(38), TILE_Y(30), 200);
        enemy_create(TILE_X(38), TILE_Y(26), 200);
        enemy_create(TILE_X(38), TILE_Y(17), 200);
        enemy_create(TILE_X(30), TILE_Y(17), 200);

        create_alien_ship(TILE_X(9), TILE_Y(30));
        create_alien_ship(TILE_X(12), TILE_Y(30));
        create_alien_ship(TILE_X(15), TILE_Y(30));
        create_alien_ship(TILE_X(23), TILE_Y(30));
        create_alien_ship(TILE_X(26), TILE_Y(30));
        create_alien_ship(TILE_X(29), TILE_Y(30));
        create_alien_ship(TILE_X(32), TILE_Y(30));

        create_controlpad(TILE_X(8), TILE_Y(26), 0);
        create_controlpad(TILE_X(21), TILE_Y(31), 1);
        create_controlpad(TILE_X(34), TILE_Y(26), 2);
        create_exitpad(TILE_X(30), TILE_Y(14), 0);

        create_trophy(TILE_X(27), TILE_Y(45.5));
    }
    else if (level == 4)
    {
 
        game_sprite_create_static(sprite_load_from_png("rom:/misc/black-hole.png"), TYPE_MAPOBJECT,
                               TILE_X(23), TILE_Y(14), 100, 100, false, false);

        game_sprite_create_static(sprite_load_from_png("rom:/misc/planet-1.png"), TYPE_MAPOBJECT,
                               TILE_X(23), TILE_Y(27), 100, 100, false, false);

        create_alien_ship(TILE_X(16), TILE_Y(11));
        create_alien_ship(TILE_X(22), TILE_Y(11));
        create_alien_ship(TILE_X(19), TILE_Y(27));

        create_fire(TILE_X(4), TILE_Y(16), 100);
        create_fire(TILE_X(5), TILE_Y(16), 100);
        create_fire(TILE_X(8), TILE_Y(16), 100);
        create_fire(TILE_X(10), TILE_Y(16), 100);
        create_fire(TILE_X(12), TILE_Y(16), 100);
        create_fire(TILE_X(12), TILE_Y(16), 100);
        create_fire(TILE_X(14), TILE_Y(16), 100);
        create_fire(TILE_X(26), TILE_Y(33), 100);
        create_fire(TILE_X(16), TILE_Y(33), 100);
        create_fire(TILE_X(21), TILE_Y(22), 100);
        create_fire(TILE_X(6), TILE_Y(21), 100);
        create_fire(TILE_X(6), TILE_Y(36), 100);
        create_fire(TILE_X(6), TILE_Y(41), 100);
        create_fire(TILE_X(7), TILE_Y(41), 100);
        create_fire(TILE_X(8), TILE_Y(41), 100);
        create_fire(TILE_X(12), TILE_Y(41), 100);
        create_fire(TILE_X(13), TILE_Y(41), 100);
        create_fire(TILE_X(15), TILE_Y(41), 100);
        create_fire(TILE_X(17), TILE_Y(41), 100);
        create_fire(TILE_X(18), TILE_Y(41), 100);
        create_fire(TILE_X(25), TILE_Y(41), 100);
        create_fire(TILE_X(26), TILE_Y(41), 100);
        create_fire(TILE_X(28), TILE_Y(41), 100);
        create_fire(TILE_X(30), TILE_Y(41), 100);
        create_fire(TILE_X(33), TILE_Y(41), 100);
        create_fire(TILE_X(34), TILE_Y(41), 100);

        enemy_create(TILE_X(6), TILE_Y(18), 200);
        enemy_create(TILE_X(6), TILE_Y(24), 200);
        enemy_create(TILE_X(6), TILE_Y(30), 200);
        enemy_create(TILE_X(6), TILE_Y(36), 200);
        enemy_create(TILE_X(7), TILE_Y(38), 200);

        enemy_create(TILE_X(16), TILE_Y(21), 200);
        enemy_create(TILE_X(16), TILE_Y(25), 200);
        enemy_create(TILE_X(15), TILE_Y(32), 200);
        enemy_create(TILE_X(16), TILE_Y(37), 200);


        enemy_create(TILE_X(26), TILE_Y(21), 200);
        enemy_create(TILE_X(26), TILE_Y(25), 200);
        enemy_create(TILE_X(27), TILE_Y(32), 200);
        enemy_create(TILE_X(26), TILE_Y(37), 200);

        enemy_create(TILE_X(36), TILE_Y(18), 200);
        enemy_create(TILE_X(36), TILE_Y(24), 200);
        enemy_create(TILE_X(36), TILE_Y(30), 200);
        enemy_create(TILE_X(36), TILE_Y(36), 200);
        enemy_create(TILE_X(35), TILE_Y(38), 200);

        create_weaponbox(TILE_X(18.25), TILE_Y(40.25), WEAPON_AUTOMATIC, 50);
        create_weaponbox(TILE_X(19.25), TILE_Y(40.25), WEAPON_AUTOMATIC, 50);

        create_weaponbox(TILE_X(23.25), TILE_Y(40.25), WEAPON_SHOTGUN, 25);
        create_weaponbox(TILE_X(24.25), TILE_Y(40.25), WEAPON_SHOTGUN, 25);

        create_medipak(TILE_X(20.25), TILE_Y(38.25), 2);
        create_medipak(TILE_X(21.25), TILE_Y(38.25), 2);
        create_medipak(TILE_X(22.25), TILE_Y(38.25), 2);
        create_medipak(TILE_X(11.25), TILE_Y(32.25), 2);
        create_medipak(TILE_X(30.25), TILE_Y(32.25), 2);

        create_controlpad(TILE_X(4), TILE_Y(41), 0);
        create_controlpad(TILE_X(21), TILE_Y(32), 1);
        create_controlpad(TILE_X(38), TILE_Y(41), 2);
        create_exitpad(TILE_X(21), TILE_Y(43), 1);
    }
    else if (level == 5)
    {
        enemy_boss_create(TILE_X(16), TILE_Y(18), 600);

        game_sprite_create_static(sprite_load_from_png("rom:/misc/planet-2.png"), TYPE_MAPOBJECT,
                                  TILE_X(44), TILE_Y(24), 100, 100, false, false);

        create_controlpad(TILE_X(39), TILE_Y(23), 0);
        create_controlpad(TILE_X(39), TILE_Y(24), 1);
        create_controlpad(TILE_X(39), TILE_Y(25), 2);
        create_exitpad(TILE_X(43), TILE_Y(24), 3);

        create_trophy(TILE_X(3), TILE_Y(35));
    }

    enemy_init();
    map_put_player_at_start_pos();
}

int map_get_width()
{
    return loaded_map_width * MAP_TILE_SIZE;
}

int map_get_height()
{
    return loaded_map_height * MAP_TILE_SIZE;
}

bool map_on_passable_tile(collision_box_t *box)
{
    int x = box->x;
    int y = box->y;
    int w = box->w;
    int h = box->h;

    unsigned int tile1 = map_absolute_coord_to_tile(x - w / 2, y - h / 2);
    unsigned int tile2 = map_absolute_coord_to_tile(x - w / 2, y + h / 2);
    unsigned int tile3 = map_absolute_coord_to_tile(x + w / 2, y - h / 2);
    unsigned int tile4 = map_absolute_coord_to_tile(x + w / 2, y + h / 2);

    assert(tile1 < MAX_MAP_TILES);
    assert(tile2 < MAX_MAP_TILES);
    assert(tile3 < MAX_MAP_TILES);
    assert(tile4 < MAX_MAP_TILES);

    if (tile1 >= MAX_MAP_TILES) tile1 = MAX_MAP_TILES - 1;
    if (tile2 >= MAX_MAP_TILES) tile2 = MAX_MAP_TILES - 1;
    if (tile3 >= MAX_MAP_TILES) tile3 = MAX_MAP_TILES - 1;
    if (tile4 >= MAX_MAP_TILES) tile4 = MAX_MAP_TILES - 1;

    if (map_tileset[tile1].passable && map_tileset[tile2].passable &&
        map_tileset[tile3].passable && map_tileset[tile4].passable)
    {
        return 1;
    }

    return 0;
}

static int cmpfunc (const void * a, const void * b) {
   map_tileset_t **_a = (map_tileset_t **)a;
   map_tileset_t **_b = (map_tileset_t **)b;
   return (_a[0]->sprite_index - _b[0]->sprite_index);
}

static map_tileset_t *sorted_map_set[150];
void map_draw()
{
    if (loaded_map_num_tiles == 0)
        return;

    int width = video_get_width() / MAP_TILE_SIZE + 1;
    int height = video_get_height() / MAP_TILE_SIZE + 1;

    int first_tile = map_absolute_coord_to_tile(camera_relative_to_absolute_x(0),
                                                camera_relative_to_absolute_y(0));
    int _x, _y;
    camera_get_offset(&_x, &_y);
    //I sort all the tiles we need to draw by its sprite index.
    //This will allow a single texture copy (into TMEM) to handle all the same tiles on the screen
    //This should be more efficient than thrashing TMEM
    int _t = 0;
    for (int i = 0; i <= height; i++)
    {
        int tile = first_tile + i * loaded_map_width;
        if (tile >= MAX_MAP_TILES)
            break;
        for (int j = 0; j < width; j++)
        {
            sorted_map_set[_t] = &map_tileset[tile];
            _t++;
            tile++;
            if (tile >= MAX_MAP_TILES)
                break;
        }
    }

    qsort(sorted_map_set, _t, sizeof(map_tileset_t *), cmpfunc);

    for (int i = 0; i < _t; i++)
    {
        if (sorted_map_set[i])
        {
            sprite_draw(sorted_map_set[i]->sprite_index,
                        sorted_map_set[i]->x + _x,
                        sorted_map_set[i]->y + _y, 100, 100, false, false);
        }
    }
}

void map_deinit()
{
    for (int i = 0; i < MAX_MAP_TILES; i++)
    {
        sprite_free(map_tileset[i].sprite_index);
        map_tileset[i].sprite_index = -1;
    }
    loaded_map_width = 0;
    loaded_map_height = 0;
    loaded_map_num_tiles = 0;

    game_sprite_t *map_object = game_sprite_get_type(TYPE_MAPOBJECT, true);
    while (map_object != NULL)
    {
        map_object->destroy = true;
        map_object = game_sprite_get_type(TYPE_MAPOBJECT, false);
    }

    game_sprite_t *item_object = game_sprite_get_type(TYPE_ITEM, true);
    while (item_object != NULL)
    {
        item_object->destroy = true;
        item_object = game_sprite_get_type(TYPE_ITEM, false);
    }

    game_sprite_t *enemy_object = game_sprite_get_type(TYPE_ENEMY, true);
    while (enemy_object != NULL)
    {
        enemy_object->destroy = true;
        enemy_object = game_sprite_get_type(TYPE_ENEMY, false);
    }

    game_sprite_t *enemy_bullet = game_sprite_get_type(TYPE_ENEMY_BULLET, true);
    while (enemy_bullet != NULL)
    {
        enemy_bullet->destroy = true;
        enemy_bullet = game_sprite_get_type(TYPE_ENEMY_BULLET, false);
    }

    game_sprite_t *player_bullet = game_sprite_get_type(TYPE_PLAYER_BULLET, true);
    while (player_bullet != NULL)
    {
        player_bullet->destroy = true;
        player_bullet = game_sprite_get_type(TYPE_PLAYER_BULLET, false);
    }
}
