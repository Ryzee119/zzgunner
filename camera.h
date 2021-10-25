// SPDX-License-Identifier: MIT
#ifndef _CAMERA_H
#define _CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif

void camera_set_offset(int x, int y);
void camera_get_offset(int *x, int *y);
int camera_increment_offset_x(int x);
int camera_increment_offset_y(int y);

//There is two coordinate sysems used. One is relative to the screen. (0,0) is top left of screen
//and there is an absolute coord system, (0,0) is top left of the overall map. Most objects use t he
//absolute system, the player uses the relative system.
int camera_relative_to_absolute_x(int x);
int camera_relative_to_absolute_y(int y);
int camera_absolute_to_relative_x(int x);
int camera_absolute_to_relative_y(int y);

bool camera_is_sprite_onscreen(game_sprite_t *game_sprite);
#ifdef __cplusplus
}
#endif

#endif