#include "common.h"

static int camera_offset_x = 0;
static int camera_offset_y = 0;

void camera_set_offset(int x, int y)
{
    camera_offset_x = x;
    camera_offset_y = y;
    
    if (camera_offset_x < -map_get_width() + video_get_width()) camera_offset_x = -map_get_width() + video_get_width();
    if (camera_offset_x > 0) camera_offset_x = 0;

    if (camera_offset_y < -map_get_height() + video_get_height()) camera_offset_y = -map_get_height() + video_get_height();
    if (camera_offset_y > 0) camera_offset_y = 0;
    
}

void camera_get_offset(int *x, int *y)
{
    *x = camera_offset_x;
    *y = camera_offset_y;
}

int camera_increment_offset_x(int x)
{
    int max_offset_x = map_get_width() * MAP_TILE_SIZE - video_get_width();
    camera_offset_x += x;
    
    if (camera_offset_x > 0)
    {
        camera_offset_x = 0;
        return 0;
    }
    if (camera_offset_x <= -max_offset_x)
    {
        camera_offset_x = -max_offset_x;
        return 0;
    }
    return 1;
}

int camera_increment_offset_y(int y)
{
    int max_offset_y = map_get_height() * MAP_TILE_SIZE - video_get_height();
    camera_offset_y += y;

    if (camera_offset_y > 0)
    {
        camera_offset_y = 0;
        return 0;
    }
    if (camera_offset_y <= -max_offset_y)
    {
        camera_offset_y = -max_offset_y;
        return 0;
    }
    return 1;
}

int camera_relative_to_absolute_x(int x)
{
    return (x - camera_offset_x);
}

int camera_relative_to_absolute_y(int y)
{
    return (y - camera_offset_y);
}

int camera_absolute_to_relative_x(int x)
{
    return (camera_offset_x + x);
}

int camera_absolute_to_relative_y(int y)
{
    return (camera_offset_y + y);
}

bool camera_is_sprite_onscreen(game_sprite_t *game_sprite)
{
    int _x = camera_absolute_to_relative_x(game_sprite->x);
    int _y = camera_absolute_to_relative_y(game_sprite->y);
    if (_x > video_get_width()) return false;
    if (_y > video_get_height()) return false;

    if (_x + game_sprite_get_width(game_sprite) < 0) return false;
    if (_y + game_sprite_get_height(game_sprite) < 0) return false;

    return true;
}