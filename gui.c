#include "common.h"

static char *font_data;
typedef struct
{
    int x;
    int y;
    int len;
    int scale;
    uint32_t dram_address[MAX_STRING_LEN];
} string_cache_t;

static string_cache_t strings[MAX_STRINGS];
static game_sprite_t *cursor;
static game_sprite_t *life_bars[PLAYER_MAX_HEALTH];
static game_sprite_t *weapons[WEAPON_MAX];

static game_sprite_t *gui_create_element(uint8_t sprite_index, int x, int y, int scale_x, int scale_y, bool mirror_x, bool mirror_y)
{
    game_sprite_t *game_sprite = game_sprite_create(FOREGROUND_LAYER, TYPE_GUI, x, y, scale_x, scale_y, mirror_x, mirror_y);
    game_sprite->fixed_on_screen = true;
    game_sprite_add_animation(game_sprite, &sprite_index, 1, 0, true);
    game_sprite->collision_box.x = game_sprite_get_width(game_sprite) / 2;
    game_sprite->collision_box.y = game_sprite_get_height(game_sprite) / 2;
    game_sprite->collision_box.w = game_sprite_get_width(game_sprite);
    game_sprite->collision_box.h = game_sprite_get_height(game_sprite);
    game_sprite->user_data = NULL;
    return game_sprite;
}

void font_init(const char *path)
{
    int w, h, n;
    unsigned char *data = stbi_load(path, &w, &h, &n, 4);
    assertf(data != NULL, "Loading font failed %s\n", stbi_failure_reason());

    //We have the data in RGBA8888 format, actually store the data in RGB5551 format to reduce RAM use
    //and  RGB5551 is compatible with the N64 VI and RDP
    font_data = memalign(16, w * h * sizeof(uint16_t));
    assert(font_data != NULL);
    assertf(w == 8, "Width: %d\n", w);
    assertf(h == 320, "Height: %d\n", h);

    uint32_t *rgba = (uint32_t *)data;
    for (int d = 0; d < w * h; d++)
    {
        uint16_t r, g, b, a;
        r = rgba[d] >> 27 & 0x1F;
        g = rgba[d] >> 19 & 0x1F;
        b = rgba[d] >> 11 & 0x1F;
        a = rgba[d] >> 7 & 0x01;
        font_data[d] = a | (b << 1) | (g << 6) | (r << 11);
    };
#ifdef N64
    data_cache_hit_writeback_invalidate(font_data, w * h * sizeof(uint16_t));
#endif
    //We're done with the PNG image. Free it now
    stbi_image_free(data);
    memset(strings, 0x00, sizeof(strings));
}

void font_write(const char *str, int x, int y, int scale)
{
    int len = strlen(str);
    assert(len < MAX_STRING_LEN);
    char _str[MAX_STRING_LEN];
    strcpy(_str, str);
    for (int i = 0; i < len; i++)
    {
        if (_str[i] >= 'a' && _str[i] <= 'z')      _str[i] -= 32; //Lowercase to uppercase
        if (_str[i] >= 'A' && _str[i] <= 'Z')      _str[i] -= 'A'; //A-Z -> 0-26
        else if (_str[i] >= '0' && _str[i] <= '9') _str[i] -= ('0' - 26); //0-9 -> 26-36
        else if (_str[i] == '-')                   _str[i] = 36;
        else if (_str[i] == '*')                   _str[i] = 37;
        else if (_str[i] == '!')                   _str[i] = 38;
        else                                       _str[i] = -1; //replace unsupported chars with ' '
    }

    for (int i = 0; i < MAX_STRINGS; i++)
    {
        if (strings[i].len != 0)
            continue;

        if (x == ALIGN_CENTER)
        {
            x = video_get_width() / 2 - (len * 8 * scale / 100 / 2);
        }
        if (y == ALIGN_CENTER)
        {
            y = video_get_height() / 2 - (8 * scale / 100 / 2);
        }
        strings[i].len = len;
        strings[i].x = x;
        strings[i].y = y;
        strings[i].scale = scale;
        for (int j = 0; j < len; j++)
        {
            if (_str[j] == -1)
            {
                strings[i].dram_address[j] = 0;
                continue;
            }
            strings[i].dram_address[j] = (uint32_t)font_data + (_str[j] * 64);
        }
        return;
    }
}

static void font_draw()
{
    for (int i = 0; i < MAX_STRINGS; i++)
    {
        if (strings[i].len == 0)
            continue;

        for (int j = 0; j < strings[i].len; j++)
        {
            if (strings[i].dram_address[j] == 0)
            {
                //Space
                continue;
            }
            video_draw_tex_rect((void *)strings[i].dram_address[j],
                                8,
                                strings[i].x + (8 * j * strings[i].scale / 100),
                                strings[i].y,
                                8,
                                8,
                                strings[i].scale,
                                strings[i].scale,
                                false,
                                false);
        }
        strings[i].len = 0;
    }
}

void gui_clear()
{
    game_sprite_t *gui = game_sprite_get_type(TYPE_GUI, true);
    while (gui != NULL)
    {
        gui->destroy = true;
        gui = game_sprite_get_type(TYPE_GUI, false);
    }
    cursor = NULL;
    memset(life_bars, 0, sizeof(life_bars));
    memset(weapons, 0, sizeof(weapons));
}

game_state_t gui_handle(game_state_t game_state)
{
    game_state_t _state = game_state;
    struct controller_data keys_down = get_keys_down();

    if (_state == INIT_MAIN_MENU)
    {
        gui_clear();
        for (int i = 0; i < 64; i++)
        {
            int scale = rand() % 25 + 5;
            gui_create_element(sprite_load_from_png("rom:/misc/star-1.png"),
                                   rand() % video_get_width(),
                                   rand() % video_get_height(),
                                   scale,
                                   scale, rand() % 1, rand() % 1);
        }

        gui_create_element(sprite_load_from_png("rom:/misc/black-hole.png"),
                               30, 30, 100, 100, false, false);

        gui_create_element(sprite_load_from_png("rom:/misc/planet-1.png"),
                               60, video_get_height() - 100, 100, 100, false, false);

        gui_create_element(sprite_load_from_png("rom:/misc/planet-2.png"),
                               video_get_width() - 140, video_get_height() - 100, 300, 300, false, false);

        cursor = gui_create_element(sprite_load_from_png("rom:/misc/pistol.png"),
                                        video_get_width() / 2 - (8 * 12), 80, 100, 100, false, false);

        game_sprite_t *player = game_sprite_get_type(TYPE_PLAYER, true);
        while(player != NULL)
        {
            player->visible = false;
            player = game_sprite_get_type(TYPE_PLAYER, false);
        }
    }

    else if (_state == MAIN_MENU)
    {
        static int cursor_pos = 0;

        if (keys_down.c[0].down)
            cursor_pos = (cursor_pos + 1) % 3;
        if (keys_down.c[0].up)
            cursor_pos--;
        if (cursor_pos < 0)
            cursor_pos = 2;

        game_sprite_set_pos(cursor, cursor->x, 80 + cursor_pos * 40);
        font_write("ZZGUNNER", ALIGN_CENTER, 20, 400);
        font_write("START GAME", ALIGN_CENTER, 80, 150);
        font_write("Made for N64 Game Jam 2021", ALIGN_CENTER, video_get_height() - 40, 100);
        font_write("By Ryzee119", ALIGN_CENTER, video_get_height() - 30, 100);

        if (keys_down.c[0].A)
        {
            if (cursor_pos == 0)
                game_state = INIT_STORY_SCREEN;
        }
    }
    else if (_state == INIT_STORY_SCREEN)
    {

    }
    else if (_state == STORY_SCREEN)
    {
        font_write("You have lost control of your ship.", ALIGN_CENTER, 30, 100);
        font_write("It has been overrun by Aliens!", ALIGN_CENTER, 50, 100);
        font_write("You must enable the self-defense", ALIGN_CENTER, 70, 100);
        font_write("system!. Make your way to the bridge", ALIGN_CENTER, 90, 100);
        font_write("to re-enable it", ALIGN_CENTER, 110, 100);
        font_write("Unfortunately the ship has gone into", ALIGN_CENTER, 130, 100);
        font_write("lockdown!", ALIGN_CENTER, 150, 100);
        font_write("You must hit the 3 control pads on", ALIGN_CENTER, 180, 100);
        font_write("each floor to get to the next stage!.", ALIGN_CENTER, 200, 100);
        
    }
    else if (_state == LOADING_LEVEL)
    {
        gui_clear();
        gui_create_element(sprite_load_from_png("rom:/misc/life-box.png"),
                               10, 10, 100, 100, false, false);

        for (int i = 0; i < PLAYER_MAX_HEALTH; i++)
        {
            life_bars[i] = gui_create_element(sprite_load_from_png("rom:/misc/life-rectangle.png"),
                                                  18 + (8 * i), 15, 100, 100, false, false);
        }

        weapons[WEAPON_PISTOL] = gui_create_element(sprite_load_from_png("rom:/misc/pistol.png"),
                                                        10, 35, 100, 100, false, false);

        weapons[WEAPON_AUTOMATIC] = gui_create_element(sprite_load_from_png("rom:/misc/auto.png"),
                                                        10, 35, 100, 100, false, false);

        weapons[WEAPON_SHOTGUN] = gui_create_element(sprite_load_from_png("rom:/misc/shotgun.png"),
                                                        10, 35, 100, 100, false, false);

        game_sprite_t *player = game_sprite_get_type(TYPE_PLAYER, true);
        while(player != NULL)
        {
            player->visible = true;
            player = game_sprite_get_type(TYPE_PLAYER, false);
        }
    }
    else if (_state == INIT_END_GAME)
    {
        for (int i = 0; i < 64; i++)
        {
            int scale = rand() % 25 + 5;
            gui_create_element(sprite_load_from_png("rom:/misc/star-1.png"),
                                   rand() % video_get_width(),
                                   rand() % video_get_height(),
                                   scale,
                                   scale, rand() % 1, rand() % 1);
        }

        gui_create_element(sprite_load_from_png("rom:/misc/black-hole.png"),
                               30, 30, 100, 100, false, false);

        gui_create_element(sprite_load_from_png("rom:/misc/planet-1.png"),
                               60, video_get_height() - 100, 100, 100, false, false);

        gui_create_element(sprite_load_from_png("rom:/misc/planet-2.png"),
                               video_get_width() - 140, 100, 300, 300, false, false);

        game_sprite_create_static(sprite_load_from_png("rom:/misc/bullet4.png"), TYPE_MAPOBJECT, //Sun :)
                                50, 50, 100, 100, false, false);

    }
    else if (_state == END_GAME)
    {
        font_write("YOU WIN!", ALIGN_CENTER, 20, 300);
        font_write("Press START to", ALIGN_CENTER, 50, 100);
        font_write("return to the main menu", ALIGN_CENTER, 60, 100);
        char nums[64];
        sprintf(nums, "Kills %d Deaths %d", player_get_kill_count(), player_get_death_count());
        font_write(nums, ALIGN_CENTER, 140, 180);
        sprintf(nums, "Trophies %d of 3", player_get_trophy_count());
        font_write(nums, ALIGN_CENTER, 180, 180);
    }
    return game_state;
}

void gui_draw_all()
{
    game_sprite_t *player = game_sprite_get_type(TYPE_PLAYER, true);
    if (player != NULL && player->visible)
    {
        player_data_t *player_state = (player_data_t *)player->user_data;
        char ammo[6];
        sprintf(ammo, "%d", player_state->ammo);
        font_write(ammo, 40, 35, 150);
        if (weapons[WEAPON_PISTOL])
            weapons[WEAPON_PISTOL]->visible = false;
        if (weapons[WEAPON_PISTOL])
            weapons[WEAPON_SHOTGUN]->visible = false;
        if (weapons[WEAPON_PISTOL])
            weapons[WEAPON_AUTOMATIC]->visible = false;
        if (weapons[player_state->current_weapon])
            weapons[player_state->current_weapon]->visible = true;

        for (int i = 0; i < PLAYER_MAX_HEALTH; i++)
        {
            if (!life_bars[i])
                break;

            if (player_state->health > i)
            {
                life_bars[i]->visible = true;
            }
            else
            {
                life_bars[i]->visible = false;
            }
        }
    }
    font_draw();
}
