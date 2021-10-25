// SPDX-License-Identifier: MIT
#ifdef __cplusplus
extern "C" {
#endif

#ifdef N64
#include <libdragon.h>
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <assert.h>
#include "video.h"
#include "sprite.h"
#include "player.h"
#include "weapon.h"
#include "map.h"
#include "sound.h"
#include "camera.h"
#include "gui.h"
#include "enemy.h"

#include "cute_tiled.h"
#include "stb_image.h"

#ifndef PI
#define PI 3.14159
#endif
enum{
    MOVE_RIGHT,
    MOVE_UPRIGHT,
    MOVE_UP,
    MOVE_UPLEFT,
    MOVE_LEFT,
    MOVE_DOWNLEFT,
    MOVE_DOWN,
    MOVE_DOWNRIGHT
};

void set_level(int new_level);

#ifdef __cplusplus
}
#endif