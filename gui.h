// SPDX-License-Identifier: MIT
#ifndef _GUI_H
#define _GUI_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAX_STRINGS
#define MAX_STRINGS 12
#endif

#ifndef MAX_STRING_LEN
#define MAX_STRING_LEN 64
#endif

#define CONTROL_SCHEME_MOVE 0
#define CONTROL_SCHEME_AIM 1

#define ALIGN_CENTER (-1)

typedef enum
{
    INIT_MAIN_MENU,
    MAIN_MENU,
    INIT_STORY_SCREEN,
    STORY_SCREEN,
    LOADING_LEVEL,
    PLAYING_LEVEL,
    INIT_END_GAME,
    END_GAME,
    PAUSE
} game_state_t;

void font_init();
void font_write(const char *str, int x, int y, int scale);

game_state_t gui_handle(game_state_t game_state);
void gui_draw_all();
void gui_clear();

#ifdef __cplusplus
}
#endif

#endif