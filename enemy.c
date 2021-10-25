#include "common.h"

static game_sprite_t *explosion = NULL;
static game_sprite_t *boss = NULL;

static void enemy_callback(game_sprite_t *this, game_sprite_t *that)
{
    if (this->type != TYPE_ENEMY)
        return;

    if (that->type != TYPE_PLAYER_BULLET)
        return;

    enemy_data_t *data = (enemy_data_t *)this->user_data;
    if (data->health <= 0)
    {
        int scale = 200 + rand() % 100;
        if (explosion != NULL)
        {
            explosion->animations[0].current_frame = 0;

            game_sprite_t *bang = game_sprite_copy(explosion,
                                                   0,
                                                   0,
                                                   scale, scale, false, false);
            //Put explosion x and y in the center of the enemy.
            int _x = this->x + (game_sprite_get_width(this) - game_sprite_get_width(bang)) / 2;
            int _y = this->y + (game_sprite_get_height(this) - game_sprite_get_height(bang)) / 2;
            game_sprite_set_pos(bang, _x, _y);
            bang->visible = true;
            bang->animations[0].current_frame = 0;
            bang->animations[0].loop = false;
        }
        this->destroy = true;
        sfx_play(SFX_EXPLODE);
        player_increment_kill_count();
    }
}

static void enemy_boss_callback(game_sprite_t *this, game_sprite_t *that)
{
    if (this->type != TYPE_ENEMY)
        return;

    enemy_data_t *data = (enemy_data_t *)this->user_data;

    if (that->type != TYPE_PLAYER_BULLET)
        return;

    if (data->health <= 0)
    {
        boss = NULL;
        if (explosion != NULL)
        {
            explosion->animations[0].current_frame = 0;
            game_sprite_t *bang = game_sprite_copy(explosion,
                                                   0,
                                                   0,
                                                   1000, 1000, false, false);
            int _x = this->x + (game_sprite_get_width(this) - game_sprite_get_width(bang)) / 2;
            int _y = this->y + (game_sprite_get_height(this) - game_sprite_get_height(bang)) / 2;
            game_sprite_set_pos(bang, _x, _y);
            bang->visible = true;
            bang->animations[0].current_frame = 0;
            bang->animations[0].loop = false;
        }
        this->destroy = true;
        sfx_play(SFX_EXPLODE);
        player_increment_kill_count();
        game_sprite_t *player = game_sprite_get_type(TYPE_PLAYER, true);
        game_sprite_set_pos(player, TILE_X(35), TILE_Y(24));
        static const int8_t screen_shaker[] = {0, -10, 0, 10, 0, -10, 0, 10, 0, -10, 0, 10, 0, -10, 0, 10, 0,
                                               -10, 0, 10, 0, -10, 0, 10, 0, -10, 0, 10, 0, -10, 0, 10, 0,
                                               -10, 0, 10, 0, -10, 0, 10, 0, -10, 0, 10, 0, -10, 0, 10, 0,
                                               -10, 0, 10, 0, -10, 0, 10, 0, -10, 0, 10, 0, -10, 0, 10, 0,
                                               -10, 0, 10, 0, -10, 0, 10, 0, -10, 0, 10, 0, -10, 0, 10, 0,
                                               -10, 0, 10, 0, -10, 0, 10, 0, -10, 0, 10, 0, -10, 0, 10, 0};
        video_shake(screen_shaker, NULL, sizeof(screen_shaker), false);
    }
}

void enemy_init()
{
    explosion = game_sprite_create(MIDDLE_LAYER, TYPE_MAPOBJECT, 0, 0, 100, 100, false, false);
    uint8_t explosion_anim[7];
    explosion_anim[0] = sprite_load_from_png("rom:/misc/explosion_fx1/tile000.png");
    explosion_anim[1] = sprite_load_from_png("rom:/misc/explosion_fx1/tile001.png");
    explosion_anim[2] = sprite_load_from_png("rom:/misc/explosion_fx1/tile002.png");
    explosion_anim[3] = sprite_load_from_png("rom:/misc/explosion_fx1/tile003.png");
    explosion_anim[4] = sprite_load_from_png("rom:/misc/explosion_fx1/tile004.png");
    explosion_anim[5] = sprite_load_from_png("rom:/misc/explosion_fx1/tile005.png");
    explosion_anim[6] = sprite_load_from_png("rom:/misc/explosion_fx1/tile006.png");
    game_sprite_add_animation(explosion, explosion_anim, sizeof(explosion_anim), 25, true);
    explosion->visible = false;
}

void enemy_create(int x, int y, int scale)
{
    game_sprite_t *enemy = game_sprite_create(MIDDLE_LAYER, TYPE_ENEMY, x, y, scale, scale, false, false);
    uint8_t enemy_idle_anim[4];
    enemy_idle_anim[0] = sprite_load_from_png("rom:/enemy/tile000.png");
    enemy_idle_anim[1] = sprite_load_from_png("rom:/enemy/tile001.png");
    enemy_idle_anim[2] = sprite_load_from_png("rom:/enemy/tile002.png");
    enemy_idle_anim[3] = sprite_load_from_png("rom:/enemy/tile001.png");
    game_sprite_add_animation(enemy, enemy_idle_anim, 4, 200, true);
    enemy->collision_box.x = game_sprite_get_width(enemy) / 2;
    enemy->collision_box.y = game_sprite_get_height(enemy) / 2;
    enemy->collision_box.w = game_sprite_get_width(enemy);
    enemy->collision_box.h = game_sprite_get_height(enemy);
    enemy->collision_callback = enemy_callback;
    enemy->user_data = malloc(sizeof(enemy_data_t));
    enemy_data_t *enemy_data = (enemy_data_t *)enemy->user_data;
    memset(enemy_data, 0, sizeof(enemy_data_t));
    enemy_data->health = rand() % 5 + 3;
    enemy_data->current_weapon = rand() % 2 + 4;
    enemy_data->speed = 1;
}

void enemy_boss_create(int x, int y, int scale)
{
    game_sprite_t *enemy = game_sprite_create(MIDDLE_LAYER, TYPE_ENEMY, x, y, scale, scale, false, false);
    uint8_t enemy_idle_anim[4];
    enemy_idle_anim[0] = sprite_load_from_png("rom:/enemy/tile000.png");
    enemy_idle_anim[1] = sprite_load_from_png("rom:/enemy/tile001.png");
    enemy_idle_anim[2] = sprite_load_from_png("rom:/enemy/tile002.png");
    enemy_idle_anim[3] = sprite_load_from_png("rom:/enemy/tile001.png");
    game_sprite_add_animation(enemy, enemy_idle_anim, 4, 200, true);
    enemy->collision_box.x = game_sprite_get_width(enemy) / 2;
    enemy->collision_box.y = game_sprite_get_height(enemy) / 2;
    enemy->collision_box.w = (game_sprite_get_width(enemy) * 3) / 4;
    enemy->collision_box.h = (game_sprite_get_height(enemy) * 3) / 4;
    enemy->collision_callback = enemy_boss_callback;
    enemy->user_data = malloc(sizeof(enemy_data_t));
    enemy_data_t *enemy_data = (enemy_data_t *)enemy->user_data;
    memset(enemy_data, 0, sizeof(enemy_data_t));
    enemy_data->health = 300;
    enemy_data->current_weapon = rand() % 3 + 5;
    enemy_data->speed = 1;
    boss = enemy;
}

void enemy_handle_logic()
{
    game_sprite_t *player = game_sprite_get_type(TYPE_PLAYER, true);
    game_sprite_t *enemy = game_sprite_get_type(TYPE_ENEMY, true);
    while (enemy != NULL)
    {
        if (camera_is_sprite_onscreen(enemy))
        {
            float angle = atan2(enemy->y - player->y, enemy->x - player->x + 0.0001) + PI;
            enemy_data_t *enemy_data = (enemy_data_t *)enemy->user_data;
            weapon_fire(enemy_data->current_weapon,
                        TYPE_ENEMY_BULLET,
                        angle, enemy->x + game_sprite_get_width(enemy) / ((enemy->mirror_x) ? 2 : 3),
                        enemy->y + game_sprite_get_height(enemy) / 5,
                        &enemy_data->last_fire_tick);
            enemy->mirror_x = (player->x > enemy->x);
        }
        enemy = game_sprite_get_type(TYPE_ENEMY, false);
    }

    if (boss != NULL)
    {
        static int weapon_change_tick_timeout = 0;
        enemy_data_t *data = (enemy_data_t *)boss->user_data;
        if (get_milli_tick() > weapon_change_tick_timeout)
        {
            data->current_weapon = rand() % 3 + 6;
            int new_time = (1000 + rand() % 2000);
            weapon_change_tick_timeout = get_milli_tick() + new_time;
        }
    }
}
