// SPDX-License-Identifier: MIT
#include "common.h"
#define NUM_FX 5
static int8_t bullet_sprites[MAX_UNIQUE_BULLETS];
static sprite_animation_t bullet_fx[NUM_FX];

static weapon_data_t pistol =
{
    .automatic = false,
    .bullet_index = 0,
    .damage = 2,
    .fire_rate = 200,
    .muzzle_fx = 0,
    .muzzle_sfx = 0,
    .speed = 10,
    .accuracy = 100, //Lower = better
    .bullets_at_once = 1,
};

static weapon_data_t shotgun =
{
    .automatic = false,
    .bullet_index = 1,
    .damage = 2,
    .fire_rate = 250,
    .muzzle_fx = 1,
    .muzzle_sfx = 1,
    .speed = 7,
    .accuracy = 100, //Lower = better
    .bullets_at_once = 3,
};

static weapon_data_t automatic =
{
    .automatic = true,
    .bullet_index = 2,
    .damage = 1,
    .fire_rate = 100,
    .muzzle_fx = 2,
    .muzzle_sfx = 2,
    .speed = 10,
    .accuracy = 200, //Lower = better
    .bullets_at_once = 1,
};

static weapon_data_t enemy_pistol =
{
    .automatic = false,
    .bullet_index = 0,
    .damage = 1,
    .fire_rate = 1000,
    .muzzle_fx = 0,
    .muzzle_sfx = 0,
    .speed = 3,
    .accuracy = 200, //Lower = better
    .bullets_at_once = 1,
};

static weapon_data_t enemy_shotgun =
{
    .automatic = false,
    .bullet_index = 1,
    .damage = 1,
    .fire_rate = 1250,
    .muzzle_fx = 1,
    .muzzle_sfx = 1,
    .speed = 2,
    .accuracy = 100, //Lower = better
    .bullets_at_once = 6,
};

static weapon_data_t enemy_boss_spread =
{
    .automatic = false,
    .bullet_index = 1,
    .damage = 1,
    .fire_rate = 1250,
    .muzzle_fx = 1,
    .muzzle_sfx = 1,
    .speed = 2,
    .accuracy = 100, //Lower = better
    .bullets_at_once = 12,
};

static weapon_data_t enemy_boss_auto =
{
    .automatic = false,
    .bullet_index = 1,
    .damage = 1,
    .fire_rate = 100,
    .muzzle_fx = 2,
    .muzzle_sfx = 2,
    .speed = 3,
    .accuracy = 100, //Lower = better
    .bullets_at_once = 4,
};

static weapon_data_t enemy_boss_none =
{
    .automatic = false,
    .bullet_index = 1,
    .damage = 1,
    .fire_rate = 5000,
    .muzzle_fx = 2,
    .muzzle_sfx = 2,
    .speed = 3,
    .accuracy = 100, //Lower = better
    .bullets_at_once = 1,
};

static weapon_data_t *weapons[] =
{
    NULL,
    &pistol,
    &shotgun,
    &automatic,
    &enemy_pistol,
    &enemy_shotgun,
    &enemy_boss_spread,
    &enemy_boss_auto,
    &enemy_boss_none,
};

static void bullet_destroy(game_sprite_t *bullet)
{
    if (camera_is_sprite_onscreen(bullet))
    {
        game_sprite_t *destroy_anim = game_sprite_create(MIDDLE_LAYER, TYPE_MAPOBJECT, bullet->x, bullet->y, 100, 100, false, false);
        memcpy(&destroy_anim->animations[0], &bullet_fx[rand() % 3], sizeof(sprite_animation_t));
    }
    bullet->destroy = true;
}

static void bullet_collision_callback(game_sprite_t *this, game_sprite_t *that)
{
    if (that->type == TYPE_PLAYER && this->type == TYPE_ENEMY_BULLET && this->destroy == false)
    {
        player_data_t *pdata = (player_data_t *)that->user_data;
        bullet_data_t *bdata = (bullet_data_t *)this->user_data;
        if (pdata->rolling)
        {
            return;
        }
        pdata->health -= bdata->damage;
        sfx_play(SFX_OUCH);
    }
    else if (that->type == TYPE_ENEMY && this->type == TYPE_PLAYER_BULLET)
    {
        enemy_data_t *edata = (enemy_data_t *)that->user_data;
        bullet_data_t *bdata = (bullet_data_t *)this->user_data;
        edata->health -= bdata->damage;
        sfx_play(SFX_OUCH);
    }
    else
    {
        return;
    }

    if (this->type == TYPE_ENEMY_BULLET || this->type == TYPE_PLAYER_BULLET)
    {
        bullet_destroy(this);
    } 
    else if (that->type == TYPE_ENEMY_BULLET || that->type == TYPE_PLAYER_BULLET)
    {
        bullet_destroy(that);
    }
}

static void bullet_register_sprite(int index)
{
    for (int i = 0; i < MAX_UNIQUE_BULLETS; i++)
    {
        if (bullet_sprites[i] == -1)
        {
            bullet_sprites[i] = index;
            return;
        }
    }
    assertf(0, "Could not register bullet. Increase MAX_UNIQUE_BULLETS\n");
}

static game_sprite_t *bullet_create(int bullet_number, int type, float angle, float speed, int damage, int x, int y)
{
    game_sprite_t *bullet = game_sprite_create_static(bullet_sprites[rand() % 4], type, x, y, 50, 50, false, false);
    bullet->collision_box.x = game_sprite_get_width(bullet) / 2;
    bullet->collision_box.y = game_sprite_get_height(bullet) / 2;
    bullet->collision_box.w = game_sprite_get_width(bullet) / 2;
    bullet->collision_box.h = game_sprite_get_height(bullet) / 2;
    bullet->user_data = malloc(sizeof(bullet_data_t));
    bullet_data_t *bdata = (bullet_data_t *)bullet->user_data;
    memset(bdata, 0, sizeof(bullet_data_t));
    bdata->speed = speed;
    bdata->travel_angle = angle;
    bdata->damage = damage;
    bullet->collision_callback = bullet_collision_callback;
    return bullet;
}

void weapon_init()
{
    memset(bullet_sprites, -1, sizeof(bullet_sprites));

    bullet_register_sprite(sprite_load_from_png("rom:/misc/bullet1.png"));
    bullet_register_sprite(sprite_load_from_png("rom:/misc/bullet2.png"));
    bullet_register_sprite(sprite_load_from_png("rom:/misc/bullet3.png"));
    bullet_register_sprite(sprite_load_from_png("rom:/misc/bullet4.png"));

    bullet_fx[0].sprite_indexes[0] = sprite_load_from_png("rom:/misc/bullet_fx1/tile000.png");
    bullet_fx[0].sprite_indexes[1] = sprite_load_from_png("rom:/misc/bullet_fx1/tile001.png");
    bullet_fx[0].sprite_indexes[2] = sprite_load_from_png("rom:/misc/bullet_fx1/tile002.png");
    bullet_fx[0].sprite_indexes[3] = sprite_load_from_png("rom:/misc/bullet_fx1/tile003.png");
    bullet_fx[0].sprite_indexes[4] = sprite_load_from_png("rom:/misc/bullet_fx1/tile004.png");
    bullet_fx[0].sprite_indexes[5] = sprite_load_from_png("rom:/misc/bullet_fx1/tile005.png");
    bullet_fx[0].total_frames = 6;
    bullet_fx[0].current_frame = 0;
    bullet_fx[0].loop = false;
    bullet_fx[0].ms_per_frame = 20;

    bullet_fx[1].sprite_indexes[0] = sprite_load_from_png("rom:/misc/bullet_fx2/tile000.png");
    bullet_fx[1].sprite_indexes[1] = sprite_load_from_png("rom:/misc/bullet_fx2/tile001.png");
    bullet_fx[1].sprite_indexes[2] = sprite_load_from_png("rom:/misc/bullet_fx2/tile002.png");
    bullet_fx[1].sprite_indexes[3] = sprite_load_from_png("rom:/misc/bullet_fx2/tile003.png");
    bullet_fx[1].sprite_indexes[4] = sprite_load_from_png("rom:/misc/bullet_fx2/tile004.png");
    bullet_fx[1].sprite_indexes[5] = sprite_load_from_png("rom:/misc/bullet_fx2/tile005.png");
    bullet_fx[1].sprite_indexes[6] = sprite_load_from_png("rom:/misc/bullet_fx2/tile006.png");
    bullet_fx[1].sprite_indexes[7] = sprite_load_from_png("rom:/misc/bullet_fx2/tile007.png");
    bullet_fx[1].total_frames = 8;
    bullet_fx[1].current_frame = 0;
    bullet_fx[1].loop = false;
    bullet_fx[1].ms_per_frame = 20;

    bullet_fx[2].sprite_indexes[0] = sprite_load_from_png("rom:/misc/bullet_fx3/tile000.png");
    bullet_fx[2].sprite_indexes[1] = sprite_load_from_png("rom:/misc/bullet_fx3/tile001.png");
    bullet_fx[2].sprite_indexes[2] = sprite_load_from_png("rom:/misc/bullet_fx3/tile002.png");
    bullet_fx[2].sprite_indexes[3] = sprite_load_from_png("rom:/misc/bullet_fx3/tile003.png");
    bullet_fx[2].sprite_indexes[4] = sprite_load_from_png("rom:/misc/bullet_fx3/tile004.png");
    bullet_fx[2].total_frames = 5;
    bullet_fx[2].current_frame = 0;
    bullet_fx[2].loop = false;
    bullet_fx[2].ms_per_frame = 20;
}

weapon_data_t *weapon_get_data(int weapon_index)
{
    assertf(weapon_index < (sizeof(weapons) / sizeof(weapon_data_t *)), "Invalid weapon_index\n");
    return weapons[weapon_index];
}

//Returns 1 if successfully fired, 0 otherwise
int weapon_fire(int weapon_index, int type, float angle, int start_x, int start_y, int* last_tick)
{
    assertf(weapon_index < (sizeof(weapons) / sizeof(weapon_data_t *)), "Invalid weapon_index\n");
    weapon_data_t *w = weapons[weapon_index];

    if (get_milli_tick() - *last_tick < w->fire_rate)
    {
        return 0;
    }

    float spread = (((type == TYPE_PLAYER_BULLET) ? 30 : 100) * PI / 180);
    float random_angle = angle + (float)(rand() % w->accuracy) / 750;    
    if (w->bullets_at_once > 1)
    {
        random_angle -= spread / 2;
    }

    for (int i = 0; i < w->bullets_at_once; i++)
    {
        bullet_create(w->bullet_index, type, random_angle, w->speed, w->damage, start_x, start_y);
        random_angle += spread / (w->bullets_at_once - 1);
    }

    sfx_play(w->muzzle_sfx);
    if (type == TYPE_PLAYER_BULLET)
    {
        static const int8_t screen_shaker[] = {0, -2, 0, 2, 0, -2, 0, 2, 0};
        video_shake(screen_shaker, NULL, sizeof(screen_shaker), false);
    }
    
    *last_tick = get_milli_tick();
    return 1;
}

void weapon_handle_logic()
{
    game_sprite_t *bullet = game_sprite_get_type(TYPE_PLAYER_BULLET, true);
    while(bullet != NULL)
    {
        collision_box_t box;
        game_sprite_get_collision_box(bullet, &box);
        if (!map_on_passable_tile(&box))
        {
            //Hit a non passable map tile
            bullet_destroy(bullet);
        }
        else
        {
            //Add x and y based on angle and speed
            bullet_data_t *bullet_data = (bullet_data_t *)bullet->user_data;
            bullet->x += cosf(bullet_data->travel_angle) * bullet_data->speed;
            bullet->y += sinf(bullet_data->travel_angle) * bullet_data->speed;
        }

        if (bullet->x <= 0 || bullet->x >= MAP_TILE_SIZE * MAX_MAP_TILES)
            bullet->destroy = true;

        if (bullet->y <= 0 || bullet->y >= MAP_TILE_SIZE * MAX_MAP_TILES)
            bullet->destroy = true;

        bullet = game_sprite_get_type(TYPE_PLAYER_BULLET, false);
    }

    bullet = game_sprite_get_type(TYPE_ENEMY_BULLET, true);
    while(bullet != NULL)
    {
        collision_box_t box;
        game_sprite_get_collision_box(bullet, &box);
        if (!map_on_passable_tile(&box))
        {
            //Hit a non passable map tile
            bullet_destroy(bullet);
        }
        else
        {
            //Add x and y based on angle and speed
            bullet_data_t *bullet_data = (bullet_data_t *)bullet->user_data;
            bullet->x += cosf(bullet_data->travel_angle) * bullet_data->speed;
            bullet->y += sinf(bullet_data->travel_angle) * bullet_data->speed;
        }

        if (bullet->x <= 0 || bullet->x >= MAP_TILE_SIZE * MAX_MAP_TILES)
            bullet->destroy = true;

        if (bullet->y <= 0 || bullet->y >= MAP_TILE_SIZE * MAX_MAP_TILES)
            bullet->destroy = true;

        bullet = game_sprite_get_type(TYPE_ENEMY_BULLET, false);
    } 
}
