// SPDX-License-Identifier: MIT
#ifndef _MAP_H
#define _MAP_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_MAP_TILES
#define MAX_MAP_TILES 2500
#endif

#ifndef MAP_TILE_SIZE
#define MAP_TILE_SIZE 32
#endif

#define TILE_X(a) (MAP_TILE_SIZE * (a))
#define TILE_Y(a) (MAP_TILE_SIZE * (a))

typedef struct
{
    int sprite_index;
    int x;
    int y;
    bool passable;
} map_tileset_t;

typedef struct
{
    bool is_opened;
} door_data_t;

typedef struct
{
    int medi_chunks;
    int ammo_cnt;
    int weapon_index;
    bool is_trophy;
} item_data_t;

extern int GATE_CLOSED, GATE_OPENED;
extern int SWITCH_INACTIVE, SWITCH_ACTIVE;

void map_startup();
void map_init(int level);
void map_draw();
void map_put_player_at_start_pos();

int map_get_width();
int map_get_height();

//Pass a collision box to detect a collision against a tile that is passable or not
//x and y are in the abolute coords relative to the map 0,0 (top left)
//Simple rectangular collision detection for now
bool map_on_passable_tile(collision_box_t *box);
void map_deinit();

//Given a absolute x and y coord, convert this to the tile number on the map. Each tile is nominally 32x32 pixels (Set by MAP_TILE_SIZE)
int map_absolute_coord_to_tile(int x, int y);

#ifdef __cplusplus
}
#endif

#endif