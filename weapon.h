// SPDX-License-Identifier: MIT
#ifndef _WEAPON_H
#define _WEAPON_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_UNIQUE_BULLETS
#define MAX_UNIQUE_BULLETS 8
#endif

#ifndef MAX_ON_SCREEN_BULLETS
#define MAX_ON_SCREEN_BULLETS 64
#endif

enum{
    WEAPON_NONE,
    WEAPON_PISTOL,
    WEAPON_SHOTGUN,
    WEAPON_AUTOMATIC,
    WEAPON_MAX,
};

typedef struct
{
    float travel_angle;
    float speed;
    int damage;
} bullet_data_t;

typedef struct
{
    float speed;
    int damage;
    int bullet_index;
    int muzzle_fx;
    int muzzle_sfx;
    int fire_rate;
    unsigned int last_fire_tick;
    bool automatic;
    int accuracy;
    int bullets_at_once;
} weapon_data_t;

extern int PLAYER_IDLE, PLAYER_RUNNING, PLAYER_ROLLING;
void weapon_init();
void weapon_handle_logic();
int weapon_fire(int weapon_index, int type, float angle, int start_x, int start_y, int* last_tick);
weapon_data_t *weapon_get_data(int weapon_index);

#ifdef __cplusplus
}
#endif

#endif
