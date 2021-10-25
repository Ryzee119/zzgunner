// SPDX-License-Identifier: MIT
#ifndef _ENEMY_H
#define _ENEMY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "weapon.h"

typedef struct
{
    int health;
    int current_weapon;
    int last_fire_tick;
    int initial_x;
    int initial_y;
    float speed;
} enemy_data_t;

void enemy_init();
void enemy_create(int x, int y, int scale);
void enemy_boss_create(int x, int y, int scale);
void enemy_handle_logic();

#ifdef __cplusplus
}
#endif

#endif