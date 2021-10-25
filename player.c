// SPDX-License-Identifier: MIT
#include "common.h"

int PLAYER_IDLE, PLAYER_RUNNING, PLAYER_ROLLING;

static int kill_count = 0;
static int death_count = 0;
static int collected_trophies = 0;

void player_increment_kill_count()
{
    kill_count++;
}

void player_increment_death_count()
{
    death_count++;
}

void player_increment_trophy_count()
{
    collected_trophies++;
    debugf("player_increment_trophy_count %d \n", collected_trophies);
}

int player_get_kill_count()
{
    return kill_count;
}

int player_get_death_count()
{
    return death_count;
}

int player_get_trophy_count()
{
    return collected_trophies;
}

void player_set_kill_count(int val)
{
    kill_count = val;
}

void player_set_death_count(int val)
{
    death_count = val;
}

void player_set_trophy_count(int val)
{
    collected_trophies = val;
}

static void player_callback(game_sprite_t *this, game_sprite_t *that)
{
    if (this->type == TYPE_PLAYER)
    {
        player_data_t *data = (player_data_t *)this->user_data;
        if (data->health <= 0)
        {
            map_put_player_at_start_pos();
            data->health = PLAYER_MAX_HEALTH;
            player_increment_death_count();
            static const int8_t screen_shaker[] = {0, -10, 0, 10, 0, -10, 0, 10, 0, -10, 0, 10, 0, -10, 0, 10, 0,
                                                   -10, 0, 10, 0, -10, 0, 10, 0, -10, 0, 10, 0, -10, 0, 10, 0};
            video_shake(screen_shaker, NULL, sizeof(screen_shaker), false);
        }
    }
}

game_sprite_t *player_init(int x, int y)
{
    game_sprite_t *player_sprite = game_sprite_create(FOREGROUND_LAYER, TYPE_PLAYER,
                                                      0, 0, 100, 100, false, false);
    player_sprite->user_data = malloc(sizeof(player_data_t));
    memset(player_sprite->user_data, 0x00, sizeof(player_data_t));

    uint8_t player_idle_anim[4];
    player_idle_anim[0] = sprite_load_from_png("rom:/player/player_idle1.png");
    player_idle_anim[1] = sprite_load_from_png("rom:/player/player_idle2.png");
    player_idle_anim[2] = sprite_load_from_png("rom:/player/player_idle3.png");
    player_idle_anim[3] = sprite_load_from_png("rom:/player/player_idle2.png");
    PLAYER_IDLE = game_sprite_add_animation(player_sprite, player_idle_anim, 4, 200, true);

    uint8_t player_walk_anim[4];
    player_walk_anim[0] = sprite_load_from_png("rom:/player/player_walk1.png");
    player_walk_anim[1] = sprite_load_from_png("rom:/player/player_walk2.png");
    player_walk_anim[2] = sprite_load_from_png("rom:/player/player_walk3.png");
    player_walk_anim[3] = sprite_load_from_png("rom:/player/player_walk4.png");
    PLAYER_RUNNING = game_sprite_add_animation(player_sprite, player_walk_anim, sizeof(player_walk_anim), 150, true);

    uint8_t player_roll_anim[5];
    player_roll_anim[0] = sprite_load_from_png("rom:/player/player_roll2.png");
    player_roll_anim[1] = sprite_load_from_png("rom:/player/player_roll3.png");
    player_roll_anim[2] = sprite_load_from_png("rom:/player/player_roll4.png");
    player_roll_anim[3] = sprite_load_from_png("rom:/player/player_roll5.png");
    player_roll_anim[4] = sprite_load_from_png("rom:/player/player_roll6.png");
    PLAYER_ROLLING = game_sprite_add_animation(player_sprite, player_roll_anim, sizeof(player_roll_anim), 50, true);

    player_data_t *player_state = (player_data_t *)player_sprite->user_data;
    player_state->health = 6;
    player_state->current_weapon = WEAPON_PISTOL;

    player_sprite->collision_box.x = game_sprite_get_width(player_sprite) / 2;
    player_sprite->collision_box.y = game_sprite_get_height(player_sprite) * 3 / 4;
    player_sprite->collision_box.w = game_sprite_get_width(player_sprite);
    player_sprite->collision_box.h = game_sprite_get_height(player_sprite);
    player_sprite->collision_callback = player_callback;

    return player_sprite;
}

void player_handle_logic(game_sprite_t *player)
{
    player_data_t *player_state = (player_data_t *)player->user_data;
    struct controller_data keys = get_keys_pressed();
    struct controller_data keys_down = get_keys_down();

    if (abs(keys.c[0].y) > 20 || abs(keys.c[0].x) > 20)
    {
        player_state->aiming_angle = atan2(-keys.c[0].y, keys.c[0].x + 0.0001);
    }

    struct SI_condat c = keys.c[0];
    if (c.C_right && c.C_down)
        player_state->aiming_angle = 1 * PI / 4;
    else if (c.C_down && c.C_left)
        player_state->aiming_angle = 3 * PI / 4;
    else if (c.C_left && c.C_up)
        player_state->aiming_angle = 5 * PI / 4;
    else if (c.C_up && c.C_right)
        player_state->aiming_angle = 7 * PI / 4;
    else
    {
        if (c.C_right)
            player_state->aiming_angle = 0 * PI / 4;
        else if (c.C_down)
            player_state->aiming_angle = 2 * PI / 4;
        else if (c.C_left)
            player_state->aiming_angle = 4 * PI / 4;
        else if (c.C_up)
            player_state->aiming_angle = 6 * PI / 4;
    }

    if (keys_down.c[0].L)
    {
        game_sprite_set_animation(player, PLAYER_ROLLING);
        player_state->roll_timer = get_milli_tick() + 250;
        player_state->speed = DEFAULT_PLAYER_SPEED * 1.5;
        player_state->rolling = true;
    }

    weapon_data_t *w = weapon_get_data(player_state->current_weapon);
    if (((w->automatic && (keys.c[0].R || keys.c[0].Z)) || (keys_down.c[0].R || keys_down.c[0].Z)) && player_state->rolling == false)
    {
        if (weapon_fire(player_state->current_weapon, TYPE_PLAYER_BULLET,
                        player_state->aiming_angle,
                        (int)player->x + game_sprite_get_width(player) / 2,
                        (int)player->y + game_sprite_get_height(player) / 3,
                        &player_state->last_fire_tick))
        {
            if (player_state->current_weapon != WEAPON_PISTOL)
            {
                player_state->ammo--;
                if (player_state->ammo <= 0)
                    player_state->current_weapon = WEAPON_PISTOL;
            }
        }
    }

    int direction = get_dpad_direction(0);
    if (direction >= 0 && (get_milli_tick() > player_state->roll_timer))
    {
        game_sprite_set_animation(player, PLAYER_RUNNING);
        player_state->speed = DEFAULT_PLAYER_SPEED;
        player_state->roll_timer = 0;
        player_state->rolling = false;
    }

    if (direction == -1)
    {
        game_sprite_set_animation(player, PLAYER_IDLE);
        player_state->rolling = false;
        player_state->speed = DEFAULT_PLAYER_SPEED;
    }

    int movex = 0, movey = 0;
    switch (direction)
    {
    case MOVE_RIGHT:
        movex = player_state->speed;
        break;
    case MOVE_LEFT:
        movex = -player_state->speed;
        break;
    case MOVE_UP:
        movey = -player_state->speed;
        break;
    case MOVE_DOWN:
        movey = player_state->speed;
        break;
    case MOVE_UPRIGHT:
        movex = player_state->speed * 0.7f;
        movey = -player_state->speed * 0.7f;
        break;
    case MOVE_UPLEFT:
        movex = -player_state->speed * 0.7f;
        movey = -player_state->speed * 0.7f;
        break;
    case MOVE_DOWNRIGHT:
        movex = player_state->speed * 0.7f;
        movey = player_state->speed * 0.7f;
        break;
    case MOVE_DOWNLEFT:
        movex = -player_state->speed * 0.7f;
        movey = player_state->speed * 0.7f;
        break;
    }

    if (movex)
        player->mirror_x = (movex < 0);

    float new_pos_x = player->x + movex;
    float new_pos_y = player->y + movey;

    collision_box_t box;
    game_sprite_get_collision_box(player, &box);

    box.x += movex;
    if (map_on_passable_tile(&box))
    {
        player->x = new_pos_x;
    }

    box.y += movey;
    if (map_on_passable_tile(&box))
    {
        player->y = new_pos_y;
    }

    //Center the camera around the player, with a small offset in the direction of aim
    camera_set_offset(-player->x - keys.c[0].x / 10 + video_get_width() / 2,
                      -player->y + keys.c[0].y / 10 + video_get_height() / 2);
}
