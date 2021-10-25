// SPDX-License-Identifier: MIT
#ifndef _SOUND_H
#define _SOUND_H

#ifdef __cplusplus
extern "C" {
#endif

#define SFX_PISTOL_SHOT 0
#define SFX_SHOTGUN_SHOT 1
#define SFX_AUTO_SHOT 2
#define SFX_PICKUP_ITEM 3
#define SFX_UNUSED 4
#define SFX_CONTROLPAD 5
#define SFX_EXPLODE 6
#define SFX_OUCH 7
#define SFX_ALARM 8

unsigned long long get_milli_tick();

void sound_init();
void sound_task();

void music_set_track(int track);
void music_stop();

void sfx_play(int sfx_number);

#ifdef __cplusplus
}
#endif

#endif