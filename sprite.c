// SPDX-License-Identifier: MIT
#include "common.h"

//All game sprites are tracked in a linked list
static game_sprite_t *head = NULL;
static game_sprite_t *last = NULL;
//All png sprites are cached in this array.
static simple_sprite_t sprite_cache[MAX_SPRITES];

//I also keep track of the sprites that are on screen for collision detection purposes.
static game_sprite_t *on_screen_sprites[MAX_SPRITES];
static int on_screen_sprites_last = 0;

static inline uint32_t __rdp_round_to_power(uint32_t number)
{
    if (number <= 4)
    {
        return 4;
    }
    if (number <= 8)
    {
        return 8;
    }
    if (number <= 16)
    {
        return 16;
    }
    if (number <= 32)
    {
        return 32;
    }
    if (number <= 64)
    {
        return 64;
    }
    if (number <= 128)
    {
        return 128;
    }

    /* Any thing bigger than 256 not supported */
    return 256;
}

int sprite_load_from_png(const char *name)
{
    //Make sure we havent already loaded this sprite
    for (int i = 0; i < MAX_SPRITES; i++)
    {
        if (strcmp(name, sprite_cache[i].name) == 0)
        {
            return i;
        }
    }

    int w, h, n;
    unsigned char *data = stbi_load(name, &w, &h, &n, 4);
    assertf(data != NULL, "Loading %s image failed %s\n", name, stbi_failure_reason());

#ifdef N64
    assertf(w * __rdp_round_to_power(h) * sizeof(uint16_t) <= 4096, "Sprite bytes exceed TMEM limit (4096bytes)");
    assertf(w == 4 || w == 8 || w == 16 || w == 32 || w == 64 || w == 128 || w == 256 || w == 512,
            "Width invalid %s, must be power of 2\n", name);
#endif

    //Find a free spot in the sprite cache and add this sprite.
    for (int i = 0; i < MAX_SPRITES; i++)
    {
        if (sprite_cache[i].data == NULL)
        {
            //We have the data in RGBA8888 format, actually store the data in RGB5551 format to reduce RAM use
            //and  RGB5551 is compatible with the N64 VI and RDP
            sprite_cache[i].data = memalign(16, w * __rdp_round_to_power(h) * sizeof(uint16_t));
            memset(sprite_cache[i].data, 0x00, w * __rdp_round_to_power(h) * sizeof(uint16_t));
            assert(sprite_cache[i].data != NULL);
            uint32_t *rgba = (uint32_t *)data;
            for (int d = 0; d < w * h; d++)
            {
                uint16_t r, g, b, a;
                r = rgba[d] >> 27 & 0x1F;
                g = rgba[d] >> 19 & 0x1F;
                b = rgba[d] >> 11 & 0x1F;
                a = rgba[d] >> 7 & 0x01;
                sprite_cache[i].data[d] = a | (b << 1) | (g << 6) | (r << 11);
            };
            strcpy(sprite_cache[i].name, name);
            sprite_cache[i].width = w;
            sprite_cache[i].height = __rdp_round_to_power(h);
            sprite_cache[i].bits_per_pixel = 16;
            //debugf("%s %d %d at %d\n", name, sprite_cache[i].width, sprite_cache[i].height, i);
#ifdef N64
            data_cache_hit_writeback_invalidate(sprite_cache[i].data, w * __rdp_round_to_power(h) * sizeof(uint16_t));
#endif
            //We're done with the PNG image. Free it now
            stbi_image_free(data);
            //debugf("%s at index %d\n", name, i);
            return i;
        }
    }
    assertf(0, "Could not load sprite, increase MAX_SPRITES\n");
    return -1;
}

void sprite_free(int index)
{
    if (index < 0)
        return;

    assert(index < MAX_SPRITES);
    if (sprite_cache[index].data)
    {
        free(sprite_cache[index].data);
    }
    memset(&sprite_cache[index], 0x00, sizeof(simple_sprite_t));
}

void sprite_free_all()
{
    for (int i = 0; i < MAX_SPRITES; i++)
    {
        sprite_free(i);
    }
}

game_sprite_t *game_sprite_create(sprite_layer_t layer, int type, int x, int y, int scale_x, int scale_y, bool mirror_x, bool mirror_y)
{
    game_sprite_t *game_sprite = malloc(sizeof(game_sprite_t));
    assert(game_sprite != NULL);
    memset(game_sprite, 0x00, sizeof(game_sprite_t));
    if (head == NULL)
    {
        head = game_sprite;
        game_sprite->prev = NULL;
    }
    else
    {
        last->next = game_sprite;
        game_sprite->prev = last;
    }
    game_sprite->next = NULL;
    last = game_sprite;
    //debugf("Created sprite this %08lx, prev: %08lx next: %08lx head: %08lx: headnext: %08lx\n",
    //       (uint32_t)game_sprite,
    //       (uint32_t)game_sprite->prev,
    //       (uint32_t)game_sprite->next,
    //       (uint32_t)head,
    //       (uint32_t)head->next);

    game_sprite->type = type;
    game_sprite->layer = layer;
    game_sprite->x = x;
    game_sprite->y = y;
    game_sprite->scale_x = scale_x;
    game_sprite->scale_y = scale_y;
    game_sprite->mirror_x = mirror_x;
    game_sprite->mirror_y = mirror_y;
    game_sprite->visible = true;
    return game_sprite;
}

game_sprite_t *game_sprite_create_static(uint8_t sprite_index, int type, int x, int y, int scale_x, int scale_y, bool mirror_x, bool mirror_y)
{
    game_sprite_t *game_sprite = game_sprite_create(FOREGROUND_LAYER, type, x, y, scale_x, scale_y, mirror_x, mirror_y);
    game_sprite->fixed_on_screen = false;
    game_sprite_add_animation(game_sprite, &sprite_index, 1, 0, true);
    game_sprite->collision_box.x = game_sprite_get_width(game_sprite) / 2;
    game_sprite->collision_box.y = game_sprite_get_height(game_sprite) / 2;
    game_sprite->collision_box.w = game_sprite_get_width(game_sprite);
    game_sprite->collision_box.h = game_sprite_get_height(game_sprite);
    return game_sprite;
}

game_sprite_t *game_sprite_copy(game_sprite_t *src, int x, int y, int scale_x, int scale_y, bool mirror_x, bool mirror_y)
{
    game_sprite_t *game_sprite = game_sprite_create(FOREGROUND_LAYER, 0, x, y, scale_x, scale_y, mirror_x, mirror_y);
    game_sprite_t *next = game_sprite->next;
    game_sprite_t *prev = game_sprite->prev;
    memcpy(game_sprite, src, sizeof(game_sprite_t));
    game_sprite->x = x;
    game_sprite->y = y;
    game_sprite->scale_x = scale_x;
    game_sprite->scale_y = scale_y;
    game_sprite->mirror_x = mirror_x;
    game_sprite->mirror_y = mirror_y;
    game_sprite->next = next;
    game_sprite->prev = prev;
    game_sprite->user_data = NULL;
    return game_sprite;
}

void game_sprite_handle_collisions()
{
    for (int i = 0; i < on_screen_sprites_last; i++)
    {
        collision_box_t box_a;
        game_sprite_get_collision_box(on_screen_sprites[i], &box_a);
        int x1 = box_a.x - box_a.w / 2;
        int y1 = box_a.y - box_a.h / 2;
        int w1 = box_a.w;
        int h1 = box_a.h;
        for (int j = 0; j < on_screen_sprites_last; j++)
        {
            if (on_screen_sprites[i] == on_screen_sprites[j])
                continue;

            collision_box_t box_b;
            game_sprite_get_collision_box(on_screen_sprites[j], &box_b);
            int x2 = box_b.x - box_b.w / 2;
            int y2 = box_b.y - box_b.h / 2;
            int w2 = box_b.w;
            int h2 = box_b.h;
            if (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2)
            {
                //debugf("1: %d %d %d %d %d\n2: %d %d %d %d %d\n",
                //       camera_absolute_to_relative_x(x1),
                //       camera_absolute_to_relative_x(x2), w1, h2, a->type,
                //       camera_absolute_to_relative_y(y1),
                //       camera_absolute_to_relative_y(y2), w2, h2, b->type);
                if (on_screen_sprites[i]->collision_callback)
                    on_screen_sprites[i]->collision_callback(on_screen_sprites[i], on_screen_sprites[j]);
                if (on_screen_sprites[j]->collision_callback)
                    on_screen_sprites[j]->collision_callback(on_screen_sprites[j], on_screen_sprites[i]);
            }
        }
    }
}

void game_sprite_free(game_sprite_t *game_sprite)
{
    if (game_sprite == NULL)
    {
        return;
    }

    //Handle list head
    if (game_sprite == head)
    {
        head = game_sprite->next;
        if (head != NULL)
            head->prev = NULL;
    }

    //Handle list tail
    else if (game_sprite == last)
    {
        last = game_sprite->prev;
        if (last != NULL)
            last->next = NULL;
    }

    //Relink list
    if (game_sprite->prev != NULL)
    {
        game_sprite->prev->next = game_sprite->next;
    }
    if (game_sprite->next != NULL)
    {
        game_sprite->next->prev = game_sprite->prev;
    }

    /*
    if (game_sprite->prev && game_sprite->next)
    {
        debugf("Freed sprite this %08lx, prev: %08lx now linked to next: %08lx head: %08lx headnext: %08lx\n", (uint32_t)game_sprite,
                                        (uint32_t)game_sprite->prev->next,
                                        (uint32_t)game_sprite->next->prev,
                                        (uint32_t)head,
                                        (uint32_t)head->next
                                        );
    }
    else if (game_sprite->prev == NULL)
    {
        debugf("Freed head\n");
    }
    else if (game_sprite->next == NULL)
    {
        debugf("Freed tail\n");
    }
    */

    //Free sprite
    if (game_sprite->user_data)
    {
        free(game_sprite->user_data);
    }
    free(game_sprite);
}

void game_sprite_free_all()
{
    game_sprite_t *a = last;
    while (a != NULL)
    {
        game_sprite_t *prev = a->prev;
        if (a->user_data)
        {
            free(a->user_data);
        }
        free(a);
        a = prev;
    }
    head = NULL;
    last = NULL;
}

void game_sprite_set_animation_speed(game_sprite_t *game_sprite, int new_ms_per_frame)
{
    game_sprite->animations[game_sprite->current_animation].ms_per_frame = new_ms_per_frame;
}

int game_sprite_add_animation(game_sprite_t *game_sprite, uint8_t *sprite_indexes, int frames, int ms_per_frame, bool loop)
{
    sprite_animation_t *animation = NULL;
    int i = 0;
    for (i = 0; i < MAX_ANIMATIONS_PER_SPRITE; i++)
    {
        if (game_sprite->animations[i].total_frames == 0)
        {
            animation = &game_sprite->animations[i];
            break;
        }
    }
    assert(animation != NULL);
    animation->total_frames = frames;
    animation->ms_per_frame = ms_per_frame;
    animation->current_frame = 0;
    animation->last_frame_tick = 0;
    animation->loop = loop;
    memcpy(animation->sprite_indexes, sprite_indexes, frames);
    return i;
}

void game_sprite_set_animation(game_sprite_t *game_sprite, uint8_t anim)
{
    game_sprite->current_animation = anim;
}

void game_sprite_set_pos(game_sprite_t *game_sprite, int x, int y)
{
    game_sprite->x = x;
    game_sprite->y = y;
}

void game_sprite_get_collision_box(game_sprite_t *game_sprite, collision_box_t *box)
{
    box->x = game_sprite->x + game_sprite->collision_box.x;
    box->y = game_sprite->y + game_sprite->collision_box.y;
    box->w = game_sprite->collision_box.w;
    box->h = game_sprite->collision_box.h;
}

void game_sprite_draw_all()
{
    game_sprite_t *a = head;
    game_sprite_t *p = NULL;
    on_screen_sprites_last = 0;
    while (a != NULL)
    {
        if (a->destroy)
        {
            game_sprite_t *temp = a->next;
            if (camera_is_sprite_onscreen(a))
            {
                on_screen_sprites_last--;
            }
            game_sprite_free(a);
            a = temp;
            continue;
        }

        bool on_screen = camera_is_sprite_onscreen(a);

        if (a->type == TYPE_NONE)
        {
            a = a->next;
            continue;
        }

        if (a->type == TYPE_PLAYER)
        {
            p = a;
        }

        if (on_screen && !a->fixed_on_screen)
        {
            assertf(on_screen_sprites_last < MAX_SPRITES, "Increase on screen sprite count\n");
            on_screen_sprites[on_screen_sprites_last] = a;
            on_screen_sprites_last++;
        }

        if (a->visible == false)
        {
            a = a->next;
            continue;
        }

        //Handle animations
        sprite_animation_t *a_anim = &a->animations[a->current_animation];
        if (a_anim->total_frames == 0)
        {
            a = a->next;
            continue;
        }

        if (a_anim->current_frame == a_anim->total_frames && a_anim->loop == false)
        {
            a->destroy = true;
            a = a->next;
            continue;
        }

        if (a_anim->ms_per_frame > 0 && (get_milli_tick() - a_anim->last_frame_tick) > a_anim->ms_per_frame)
        {
            if (a_anim->loop)
                a_anim->current_frame = (a_anim->current_frame + 1) % a_anim->total_frames;
            else
            {
                if (a_anim->current_frame < a_anim->total_frames)
                {
                    a_anim->current_frame++;
                }
                if (a_anim->current_frame == a_anim->total_frames)
                {
                    return;
                }
            }

            a_anim->last_frame_tick = get_milli_tick();
        }

        int _x, _y;
        if (a->fixed_on_screen)
        {
            _x = (a->x);
            _y = (a->y);
        }
        else
        {
            _x = camera_absolute_to_relative_x(a->x);
            _y = camera_absolute_to_relative_y(a->y);
        }

        if ((on_screen || a->fixed_on_screen))
        {
            sprite_draw(a_anim->sprite_indexes[a_anim->current_frame],
                    _x,
                    _y,
                    a->scale_x,
                    a->scale_y,
                    a->mirror_x,
                    a->mirror_y);
        }
        a = a->next;
    }

    if (p != NULL)
    {
        if (p->visible == false)
            return;
        sprite_animation_t *p_anim = &p->animations[p->current_animation];
        int _x = camera_absolute_to_relative_x(p->x);
        int _y = camera_absolute_to_relative_y(p->y);
        sprite_draw(p_anim->sprite_indexes[p_anim->current_frame],
                    _x,
                    _y,
                    p->scale_x,
                    p->scale_y,
                    p->mirror_x,
                    p->mirror_y);
    }
}

int game_sprite_get_width(game_sprite_t *game_sprite)
{
    sprite_animation_t *a = &game_sprite->animations[game_sprite->current_animation];
    simple_sprite_t *s = &sprite_cache[a->sprite_indexes[a->current_frame]];
    return (s->width * game_sprite->scale_x) / 100;
}

int game_sprite_get_height(game_sprite_t *game_sprite)
{
    sprite_animation_t *a = &game_sprite->animations[game_sprite->current_animation];
    simple_sprite_t *s = &sprite_cache[a->sprite_indexes[a->current_frame]];
    return (s->height * game_sprite->scale_y) / 100;
}

void sprite_draw(int index, int x, int y, int scalex, int scaley, bool mirrorx, bool mirrory)
{
    simple_sprite_t *sprite = &sprite_cache[index];

    if (index < 0)
        return;

    video_draw_tex_rect(sprite->data, sprite->bits_per_pixel,
                        x,
                        y,
                        sprite->width,
                        sprite->height,
                        scalex,
                        scaley,
                        mirrorx,
                        mirrory);
}

game_sprite_t *game_sprite_get_type(int type, bool reset)
{
    static int _type = -1;
    static game_sprite_t *_index = NULL;
    if (type != _type || reset)
    {
        _index = head;
        _type = type;
    }
    while (_index != NULL)
    {
        if (_index->type == type)
        {
            game_sprite_t *match = _index;
            _index = _index->next;
            return match;
        }
        _index = _index->next;
    }
    return NULL;
}
