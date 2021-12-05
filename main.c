#define CUTE_TILED_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_THREAD_LOCALS
#define STBI_ONLY_PNG

#include "common.h"

static game_state_t game_state = INIT_MAIN_MENU;
static int current_level = 0;

#ifdef SHOW_FPS
static int perf_fps = 0;
static int frame_cnt = 0;
static bool update_frame = true;
static void fps_calc(int ovfl)
{
    static int old_frame_cnt = 0;
    perf_fps = frame_cnt - old_frame_cnt;
    old_frame_cnt = frame_cnt;
    update_frame = true;
}
#endif

void set_level(int new_level)
{
    if (new_level == 6)
        game_state = INIT_END_GAME;
    else
    {
        game_state = LOADING_LEVEL;
    }
    
    current_level = new_level;
}

int main(void)
{
#ifdef N64
    controller_init();
    //Uncomment for IS-VIEWER debug output.
    //debug_init(DEBUG_FEATURE_LOG_ISVIEWER);
    dfs_init(DFS_DEFAULT_LOCATION);
    timer_init();
#ifdef SHOW_FPS
    new_timer(TIMER_TICKS(1000000), TF_CONTINUOUS, fps_calc);
#endif
#endif

    srand((unsigned int)646464);
    video_init();
    sound_init();
    map_startup();
    game_sprite_t *player_sprite = NULL;
    font_init("rom:/font/font.png");
    int paused = false;
    while (1)
    {
        controller_scan();

        struct controller_data keys_down = get_keys_down();
        if (keys_down.c[0].start && (game_state == PLAYING_LEVEL || game_state == PAUSE))
        {
            paused ^= 1;
            game_state = (paused) ? PAUSE : PLAYING_LEVEL;
        }

        switch (game_state)
        {
        case INIT_MAIN_MENU:
            if (player_sprite == NULL)
                player_sprite = player_init();
            video_wait_complete();
            gui_handle(game_state);
            game_state = MAIN_MENU;
            music_set_track(current_level);
            break;
        case MAIN_MENU:
            video_wait_complete();
            game_state = gui_handle(game_state);
            break;
        case INIT_STORY_SCREEN:
            video_wait_complete();
            gui_clear();
            gui_handle(game_state);
            game_state = STORY_SCREEN;
            break;
        case STORY_SCREEN:
            video_wait_complete();
            gui_handle(game_state);
            if (keys_down.c[0].start || keys_down.c[0].A)
            {
                game_state = LOADING_LEVEL;
                current_level = 1;
                player_set_kill_count(0);
                player_set_death_count(0);
                player_set_trophy_count(0);
            }
            break;
        case LOADING_LEVEL:
            video_wait_complete();
            gui_handle(game_state);
            game_state = PLAYING_LEVEL;
            music_set_track(current_level);
            map_init(current_level);
            weapon_init();
            break;
        case PLAYING_LEVEL:
            if (current_level == 6)
            {
                game_state = INIT_END_GAME;
                break;
            }
            player_handle_logic(player_sprite);
            weapon_handle_logic();
            game_sprite_handle_collisions();
            enemy_handle_logic();
            break;
        case INIT_END_GAME:
            video_wait_complete();
            map_deinit();
            game_sprite_free_all();
            player_sprite = NULL;
            gui_handle(game_state);
            music_set_track(current_level);
            game_state = END_GAME;
            break;
        case END_GAME:
            gui_handle(game_state);
            if (keys_down.c[0].start)
            {
                game_state = INIT_MAIN_MENU;
                current_level = 0;
            }
            break;
        case PAUSE:
            break;
        }
#ifdef SHOW_FPS
        static char _fps[10];
        if (update_frame)
        {
            sprintf(_fps, "fps:%d", perf_fps);
            update_frame = false;
        }
        font_write(_fps, video_get_width() - (8 * 8), video_get_height() - 45, 100);
        frame_cnt++;
#endif

        //Sound
        sound_task();

        //Video Rendering
        video_begin_frame();
        if (game_state == END_GAME)
        {
            video_draw_rect(0x10, 0x10, 0x10, 0xFF, 0, 0, video_get_width(), video_get_height());
        }
        map_draw();
        game_sprite_draw_all();
        gui_draw_all();
        
        //Draw paused screen
        if (paused)
        {
            font_write("PAUSED", ALIGN_CENTER, ALIGN_CENTER, 200);
        }
        video_end_frame();
        //malloc_stats();
    }
    map_deinit();
    game_sprite_free_all();
    sprite_free_all();
    return 0;
}
