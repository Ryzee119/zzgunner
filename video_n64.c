// SPDX-License-Identifier: MIT
#include "common.h"

static display_context_t disp = 0;
static ugfx_buffer_t *render_commands[NUM_BUFFERS];
static void *tmem_data = NULL;
static int display_width;
static int display_height;
static uint64_t current_render_mode;
static bool screen_shaker_loop = false;
static int screen_shaker_frames = 0;
static int8_t screen_shaker_x[128] = {0};
static int8_t screen_shaker_y[128] = {0};
static int screen_shaker_offset = 0;

static volatile uint32_t wait_intr = 1;
static void __rdp_interrupt()
{
    /* Flag that the interrupt happened */
    wait_intr = 1;
}

void video_init()
{
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, NUM_BUFFERS, GAMMA_NONE, ANTIALIAS_OFF);
    ugfx_init(UGFX_MIN_RDP_BUFFER_SIZE * 16);
    render_commands[0] = ugfx_buffer_new(2048);
    render_commands[1] = ugfx_buffer_new(2048);
    display_width = 320;
    display_height = 240;
    register_DP_handler(__rdp_interrupt);
    set_DP_interrupt(1);
}

int video_get_width()
{
    return display_width;
}

int video_get_height()
{
    return display_height;
}

ugfx_buffer_t *video_get_display_list()
{
    assert(disp != 0);
    return render_commands[disp - 1];
}

void video_wait_complete()
{
    rsp_wait();
    while( !wait_intr ) { ; }
}

void display_show_force(display_context_t disp);
void video_begin_frame()
{
    //Wait for the RDP and show the last from on VSYNC
    video_wait_complete();

    if (disp)
        display_show(disp);

    //Get the back buffer
    while (!(disp = display_lock()))
        ;

    tmem_data = NULL;
    ugfx_buffer_t *command_list = video_get_display_list();
    ugfx_buffer_clear(command_list);
    ugfx_buffer_push(command_list, ugfx_sync_pipe());
    ugfx_buffer_push(command_list, ugfx_set_display(disp));
    ugfx_buffer_push(command_list, ugfx_set_scissor(0, 0, (display_width - 1) << 2, (display_height - 1) << 2, UGFX_SCISSOR_DEFAULT));
    ugfx_buffer_push(command_list, ugfx_set_other_modes(UGFX_CYCLE_FILL));
    ugfx_buffer_push(command_list, ugfx_set_fill_color(PACK_RGBA16x2(0x10, 0x10, 0x10, 0xFF)));
    ugfx_buffer_push(command_list, ugfx_fill_rectangle(0, 0, display_width << 2, display_height << 2));
    current_render_mode = UGFX_CYCLE_FILL;
}

void video_end_frame()
{
    ugfx_buffer_t *command_list = video_get_display_list();
    ugfx_buffer_push(command_list, ugfx_sync_pipe());
    ugfx_buffer_push(command_list, ugfx_sync_full());
    ugfx_buffer_push(command_list, ugfx_finalize());
    data_cache_hit_writeback(ugfx_buffer_data(command_list), ugfx_buffer_length(command_list) * sizeof(ugfx_command_t));
    ugfx_load(ugfx_buffer_data(command_list), ugfx_buffer_length(command_list));
    wait_intr = 0;
    rsp_run_async();
    if (screen_shaker_frames > 0)
    {
        if (screen_shaker_loop == false)
        {
            screen_shaker_x[screen_shaker_offset] = 0;
            screen_shaker_y[screen_shaker_offset] = 0;
        }
        screen_shaker_offset = (screen_shaker_offset + 1) % screen_shaker_frames;
    }
}

//RGBA8888
void video_draw_rect(uint8_t r, uint8_t g, uint8_t b, uint8_t a, int x, int y, int w, int h)
{
    ugfx_buffer_t *command_list = video_get_display_list();
    if (current_render_mode != UGFX_CYCLE_FILL)
    {
        ugfx_buffer_push(command_list, ugfx_set_other_modes(UGFX_CYCLE_FILL));
        current_render_mode = UGFX_CYCLE_FILL;
    }
    ugfx_buffer_push(command_list, ugfx_sync_pipe());
    ugfx_buffer_push(command_list, ugfx_set_fill_color(PACK_RGBA16x2(r, g, b, a)));
    ugfx_buffer_push(command_list, ugfx_fill_rectangle(x << 2, y << 2, (x + w - 1) << 2, (x + h - 1) << 2));
}

//Draw a texture rect to the screen
//tex = pointer to the data (Only tested with RGBA5551)
//bpp = bits per pixel
//x,y = coords on the screen (0,0 is top left)
//w,h = width and height in pixels of the rect (unscaled)
//w_scale/h_scale, percent to scale the rect by (100 = no scale)
//mirror_x/mirror_y, whether to mirror the rect on the axis
void video_draw_tex_rect(void *tex, uint8_t bpp, int x, int y, int w, int h, int w_scale, int h_scale, bool mirror_x, bool mirror_y)
{
#define TMEM_ADDRESS 0x00000000
#define SPRITE_TILE 0
    int ds = 4 << 10, dt = 1 << 10;
    float s0 = 0, t0 = 0;
    float h_scale_ratio = (float)h_scale / 100;
    float w_scale_ratio = (float)w_scale / 100;
    float scaled_w = w * h_scale_ratio;
    float scaled_h = h * h_scale_ratio;

    assert(w_scale > 0);
    assert(h_scale > 0);

    //Handle screen shaking
    x += screen_shaker_x[screen_shaker_offset];
    y += screen_shaker_y[screen_shaker_offset];

    if (x + scaled_w - 1 < 0)
        return;
    if (x >= video_get_width())
        return;
    if (y + scaled_h - 1 < 0)
        return;
    if (y >= video_get_height())
        return;

    assertf(tex != NULL, "Trying to draw uncached sprite\n");

    uint64_t ugfx_bpp = bpp == 32 ? UGFX_PIXEL_SIZE_32B : bpp == 16 ? UGFX_PIXEL_SIZE_16B
                                                      : bpp == 8    ? UGFX_PIXEL_SIZE_8B
                                                      : bpp == 4    ? UGFX_PIXEL_SIZE_4B
                                                                    : UGFX_PIXEL_SIZE_32B;

    ugfx_buffer_t *command_list = video_get_display_list();

    //Load the texture from sprite:
    if (tmem_data != tex)
    {
        ugfx_buffer_push(command_list, ugfx_sync_pipe());
        ugfx_buffer_push(command_list, ugfx_set_texture_image((uint32_t)tex, UGFX_FORMAT_RGBA, ugfx_bpp, w - 1));
        ugfx_buffer_push(command_list, ugfx_set_tile(UGFX_FORMAT_RGBA, ugfx_bpp, (w * bpp) / 64, TMEM_ADDRESS, SPRITE_TILE, 0, 0, 0, 0, 0, 0, 0, 0, 0));
        ugfx_buffer_push(command_list, ugfx_sync_load());
        ugfx_buffer_push(command_list, ugfx_load_tile(0 << 2, 0 << 2, (w - 1) << 2, (h - 1) << 2, SPRITE_TILE));
        tmem_data = tex;
    }

    #define N64_MAX(a,b) (((a) > (b)) ? (a) : (b))
    //Handle edges
    if (x < 0)
    {
        s0 = -x / w_scale_ratio;;
        x = 0;
        scaled_w -= s0 * w_scale_ratio;
    }

    if (y < 0)
    {
        t0 = -y / h_scale_ratio;
        y = 0;
        scaled_h -= t0 * h_scale_ratio;
    }

    //Any scaling or mirroring needs to use 1CYCLE mode
    if (mirror_x || mirror_y || w_scale != 100 || h_scale != 100 || w <= 16 || h <= 16)
    {
        ds = 1024 * 100 / w_scale;
        dt = 1024 * 100 / h_scale;

        if (mirror_x)
        {
            s0 = (float)w - s0;
            ds = -ds;
        }
        if (mirror_y)
        {
            t0 = h - t0;
            dt = -dt;
        }
        if (current_render_mode != UGFX_CYCLE_1CYCLE)
        {
            ugfx_buffer_push(command_list, ugfx_sync_pipe());
            ugfx_buffer_push(command_list, ugfx_set_other_modes(
                                               UGFX_CYCLE_1CYCLE |
                                               ugfx_blend_1cycle(UGFX_BLEND_IN_RGB, UGFX_BLEND_IN_ALPHA, UGFX_BLEND_MEM_RGB, UGFX_BLEND_1_MINUS_A) |
                                               UGFX_SAMPLE_POINT | UGFX_BI_LERP_0 | UGFX_BI_LERP_1 | UGFX_ALPHA_DITHER_NONE | UGFX_RGB_DITHER_NONE |
                                               UGFX_IMAGE_READ | UGFX_ALPHA_COMPARE));

            ugfx_buffer_push(command_list, ugfx_set_combine_mode(0, 0, 0, 0, 0, 0, 0, 0, 6, 15, 1, 7, 1, 7, 0, 7));
            ugfx_buffer_push(command_list, ugfx_set_blend_color(0x00000001));
            current_render_mode = UGFX_CYCLE_1CYCLE;
        }
    }
    //No scaling or mirroring, use COPY mode. This is much faster
    else
    {
        if (current_render_mode != UGFX_CYCLE_COPY)
        {
            ugfx_buffer_push(command_list, ugfx_sync_pipe());
            ugfx_buffer_push(command_list, ugfx_set_other_modes(UGFX_CYCLE_COPY | UGFX_ALPHA_COMPARE));
            current_render_mode = UGFX_CYCLE_COPY;
        }
    }

    ugfx_buffer_push(command_list, ugfx_texture_rectangle(SPRITE_TILE, x << 2, y << 2, (int)((x + scaled_w - 1) * 4), (int)((y + scaled_h - 1) * 4)));
    ugfx_buffer_push(command_list, ugfx_texture_rectangle_tcoords((int)(s0 * 32), (int)(t0 * 32), ds, dt));
}

void video_shake(const int8_t *shake_array_x, const int8_t *shake_array_y, int frames, bool loop)
{
    assert(frames < sizeof(screen_shaker_x));

    if (shake_array_x)
    {
        memcpy(screen_shaker_x, shake_array_x, frames);
    }

    if (shake_array_y)
    {
        memcpy(screen_shaker_y, shake_array_y, frames);
    }

    screen_shaker_offset = 0;
    screen_shaker_loop = loop;
    screen_shaker_frames = frames;
}
