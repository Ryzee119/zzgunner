// SPDX-License-Identifier: MIT
#ifndef _SPRITE_H
#define _SPRITE_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef MAX_SPRITES
#define MAX_SPRITES 128
#endif

#ifndef MAX_GAME_SPRITES
#define MAX_GAME_SPRITES 150
#endif

#ifndef MAX_FRAMES_PER_ANIMATION
#define MAX_FRAMES_PER_ANIMATION 12
#endif

#ifndef MAX_ANIMATIONS_PER_SPRITE
#define MAX_ANIMATIONS_PER_SPRITE 4
#endif

typedef enum sprite_layer
{
    BACKGROUND_LAYER,
    MIDDLE_LAYER,
    FOREGROUND_LAYER
} sprite_layer_t;

typedef struct
{
    char name[64];
    int width;
    int height;
    int bits_per_pixel;
    uint16_t *data;
    uint16_t tileset_id;
} simple_sprite_t;

typedef struct
{
    uint8_t sprite_indexes[MAX_FRAMES_PER_ANIMATION];
    int total_frames;
    int current_frame;
    int ms_per_frame;
    uint32_t last_frame_tick;
    bool loop;
} sprite_animation_t;

typedef struct
{
    int x; //Center of box
    int y; //Center of box
    int w; //Center of box
    int h; //Center of box
} collision_box_t;

enum 
{
    TYPE_NONE,
    TYPE_PLAYER,
    TYPE_ENEMY,
    TYPE_ITEM,
    TYPE_PLAYER_BULLET,
    TYPE_ENEMY_BULLET,
    TYPE_MAPOBJECT,
    TYPE_GUI,
};

typedef struct game_sprite_t
{
    int type;
    sprite_layer_t layer;
    sprite_animation_t animations[MAX_ANIMATIONS_PER_SPRITE];
    collision_box_t collision_box;
    void (*collision_callback)(struct game_sprite_t *this_game_sprite, struct game_sprite_t *other_game_sprite);
    int current_animation;
    bool blocking;
    float x;
    float y;
    int scale_x;
    int scale_y;
    bool mirror_x;
    bool mirror_y;
    bool visible;
    bool fixed_on_screen;
    void *user_data;
    bool destroy;
    struct game_sprite_t *prev;
    struct game_sprite_t *next;
} game_sprite_t;



//Load a sprite from a given file (on N64 this should be in the form "rom:/myimage.png")
//This returns in index in the sprite cache. If you try open the same sprite again it will return the
//same index.
int sprite_load_from_png(const char *filename);

//Free a given sprite index from the sprite cache.
void sprite_free(int index);

//Free all sprites in the sprite cache
void sprite_free_all();
void game_sprite_free_all();

//Draw a sprite from a given index and x, y coords relative to the screen. scalex/scaley are percent scale values
//i.e scale=100 is no scaling.
void sprite_draw(int index, int x, int y, int scalex, int scaley, bool mirrorx, bool mirrory);

//Create a new gamesprite which is allocated on the heap internally. Gamesprites are a wrapper around a simple sprite which stores
//various parameters related to it useful to monitoring game state. You can also link a custom user struct.
game_sprite_t *game_sprite_create(sprite_layer_t layer, int type, int x, int y, int scale_x, int scale_y, bool mirror_x, bool mirror_y);

//Create a new gamesprite which is allocated on the heap internally. Gamesprites are a wrapper around a simple sprite which stores
//Is just creating a static sprite with no animations, you can use this. Load a sprite with sprite_load_from_png to get an index.
game_sprite_t *game_sprite_create_static(uint8_t sprite_index, int type, int x, int y, int scale_x, int scale_y, bool mirror_x, bool mirror_y);

game_sprite_t *game_sprite_copy(game_sprite_t *src, int x, int y, int scale_x, int scale_y, bool mirror_x, bool mirror_y);

//Add an animation to a previously created game_sprite. This returns the index of the animation. This starts at 0 and increases
//for each animation added. The max is set by MAX_ANIMATIONS_PER_SPRITE.
//The number of frames in the animation is set by MAX_FRAMES_PER_ANIMATION
//ms_per_frame sets how many milliseconds to elasped before using the next frame. This animation will loop after all frames are done.
//sprite_indexes is an array to the sprite index created with sprite_load_from_png.
//example to create two animations with 3 frames. One cycles every second, the other every 100ms.
//game_sprite_t *my_animated_sprite = game_sprite_create(0, 50, 50, 100, 100, false, false);
//uint8_t my_animation[3];
//my_animation[0]=sprite_load_from_png("rom:/frame1.png");
//my_animation[1]=sprite_load_from_png("rom:/frame2.png");
//my_animation[2]=sprite_load_from_png("rom:/frame3.png");
//int animation_index1 = game_sprite_add_animation(my_animated_sprite, my_animation, sizeof(my_animation), 1000);
//int animation_index2 = game_sprite_add_animation(my_animated_sprite, my_animation, sizeof(my_animation), 100);
//game_sprite_set_animation(my_animated_sprite, animation_index1); //or animation_index2
int game_sprite_add_animation(game_sprite_t *game_sprite, uint8_t *sprite_indexes, int frames, int ms_per_frame, bool loop);

//Set what animation index to use. These should be registered with game_sprite_add_animation.
//The max number is set by MAX_ANIMATIONS_PER_SPRITE
void game_sprite_set_animation(game_sprite_t *game_sprite, uint8_t anim);

//Change the speed of the currently set animation. See game_sprite_set_animation
void game_sprite_set_animation_speed(game_sprite_t *game_sprite, int new_ms_per_frame);

void game_sprite_set_pos(game_sprite_t *game_sprite, int x, int y);

void game_sprite_get_collision_box(game_sprite_t *game_sprite, collision_box_t *box);

//Draw all the gamesprites that have been created with game_sprite_create or added to the internal list with game_sprite_add
void game_sprite_draw_all();

void game_sprite_handle_collisions();

//Free a gramesprite created with game_sprite_create
void game_sprite_free(game_sprite_t *game_sprite);

//Get the width/height of the current sprite in the gamesprite
int game_sprite_get_width(game_sprite_t *game_sprite);
int game_sprite_get_height(game_sprite_t *game_sprite);

game_sprite_t *game_sprite_get_type(int type, bool reset);

#ifdef __cplusplus
}
#endif

#endif
