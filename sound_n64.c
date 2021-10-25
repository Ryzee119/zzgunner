// SPDX-License-Identifier: MIT
#include "common.h"

#define MAX_CHANNELS 32
#define MUSIC_CHANNEL 0
#define NUM_SONGS 7
#define NUM_SFX 9

static xm64player_t xm;
static wav64_t sfx[NUM_SFX];

static const char *music_files[NUM_SONGS] =
{
    "rom:/music/accel_menu.xm64",
    "rom:/music/a_cast.xm64",
    "rom:/music/a_cast.xm64",
    "rom:/music/a_cast.xm64",
    "rom:/music/a_cast.xm64",
    "rom:/music/a_cast.xm64",
    "rom:/music/81978-happy.xm64",
};

//84692-im_the_freak 30 channels
//accel_menu 18
//_moon 10
//a_cast 4
//84692-im_the_freak 30
//a-depres 24

static const char *sfx_files[NUM_SFX] =
{
    "sfx/sfx1.wav64", //Pistol shot
    "sfx/sfx2.wav64", //Shotgun shot
    "sfx/sfx3.wav64", //Auto shot
    "sfx/sfx4.wav64", //Pick up item
    "sfx/sfx5.wav64", //Unused?
    "sfx/sfx6.wav64", //Activate control pad
    "sfx/sfx7.wav64", //Enemy exploding
    "sfx/sfx8.wav64", //Took damage
    "sfx/sfx9.wav64", //Alarm
};

unsigned long long get_milli_tick()
{
    return (timer_ticks() / (long long)TICKS_FROM_MS(1));
}

void sound_init()
{
    audio_init(44100, 4);
    mixer_init(32);
    for (int i = 0; i < NUM_SFX; i++)
    {
        wav64_open(&sfx[i], sfx_files[i]);
        wav64_set_loop(&sfx[i], false);
    }
}

void music_set_track(int track)
{
    assertf(track < NUM_SONGS, "Track number invalid\n");
    music_stop();
    xm64player_open(&xm, music_files[track]);
    xm64player_play(&xm, MUSIC_CHANNEL);
    assertf(xm64player_num_channels(&xm) < 25, "%s Too many music channels %d\n", music_files[track], xm64player_num_channels(&xm));
}

void music_stop()
{
    if (xm.playing || xm.waves)
        xm64player_close(&xm);

    for (int channel = 0; channel < MAX_CHANNELS; channel++)
    {
        if (mixer_ch_playing(channel))
        {
            mixer_ch_stop(channel);
        }
    }

    memset(&xm, 0, sizeof(xm64player_t ));
}

void sfx_play(int sfx_number)
{
    assertf(sfx_number < NUM_SFX, "Sfx number invalid\n");
    for (int channel = 25; channel < MAX_CHANNELS; channel++)
    {
        if (!mixer_ch_playing(channel))
        {
            mixer_ch_play(channel, &sfx[sfx_number].wave);
            return;
        }
        else
        {
            channel += sfx[sfx_number].wave.channels;
        }
    }
    //assertf(0, "Not enough SFX channels, increase MAX_CHANNELS\n");
}

void sound_task()
{
    int buffers = 4;
    while(audio_can_write() && buffers > 0)
    {
        rsp_wait();
        int16_t *out = audio_write_begin();
        mixer_poll(out, audio_get_buffer_length());
        audio_write_end();
        buffers--;
    }
}
