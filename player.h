// SPDX-License-Identifier: MIT
#ifndef _PLAYER_H
#define _PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DEFAULT_PLAYER_SPEED
#define DEFAULT_PLAYER_SPEED 3
#endif

#ifndef PLAYER_MAX_HEALTH
#define PLAYER_MAX_HEALTH 6
#endif

#include "weapon.h"

typedef struct
{
    bool rolling;
    uint32_t roll_timer;
    bool firing;
    int current_weapon;
    int last_fire_tick;
    int ammo;
    uint8_t health;
    float speed;
    float aiming_angle;
    int control_scheme_stick;
    int control_scheme_dpad;
    int control_scheme_cpad;
} player_data_t;

extern int PLAYER_IDLE, PLAYER_RUNNING, PLAYER_ROLLING;
game_sprite_t *player_init();
void player_handle_logic(game_sprite_t * player_sprite);
bool player_in_center_x(game_sprite_t *player_sprite);
bool player_in_center_y(game_sprite_t *player_sprite);
void player_increment_kill_count();
void player_increment_death_count();
int player_get_kill_count();
int player_get_death_count();
void player_increment_trophy_count();
int player_get_trophy_count();
void player_set_kill_count(int val);
void player_set_death_count(int val);
void player_set_trophy_count(int val);


#ifdef __cplusplus
}
#endif

#endif