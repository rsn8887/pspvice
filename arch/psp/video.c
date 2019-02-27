#include "lib/video.h"
#include "lib/ctrl.h"
#include "lib/pl_gfx.h"
#include "lib/pl_vk.h"
#include "lib/font.h"

#include "vice.h"
#include "interrupt.h"
#include "cmdline.h"
#include "video.h"
#include "videoarch.h"
#include "palette.h"
#include "viewport.h"
#include "keyboard.h"
#include "joystick.h"
#include "lib.h"
#include "log.h"
#include "ui.h"
#include "vsync.h"
#include "raster.h"
#include "sound.h"
#include "machine.h"
#include "resources.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DEBUG_VIDEO
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

PspImage *Screen = NULL;
static struct video_canvas_s *activeCanvas = NULL;
static float last_framerate = 0;
static float last_percent = 0;
static int screen_x;
static int screen_y;
static int screen_w;
static int screen_h;
static int clear_screen;
static int line_height;
static int psp_first_time = 1;
static int drive_led_on = 0, tape_led_on = 0;
static int disk_icon_offset, tape_icon_offset;
static int c64_screen_w, c64_screen_h;

static int show_kybd_held;
static int keyboard_visible;
static int ntsc_mode = 0;
static pl_vk_layout psp_keyboard;

static inline void psp_keyboard_toggle(unsigned int code, int on);

static void video_psp_display_menu();
static void pause_trap(WORD unused_addr, void *data);

typedef struct psp_ctrl_mask_to_index_map
{
  uint64_t mask;
  uint8_t index;
} psp_ctrl_mask_to_index_map_t;

static const psp_ctrl_mask_to_index_map_t physical_to_emulated_button_map[] =
    {
        /* These are shift-based (e.g. L/R are not unset when a button pressed) */
        {PSP_CTRL_LTRIGGER | PSP_CTRL_SELECT, 18},
        {PSP_CTRL_RTRIGGER | PSP_CTRL_SELECT, 19},
        {PSP_CTRL_LTRIGGER | PSP_CTRL_SQUARE, 20},
        {PSP_CTRL_LTRIGGER | PSP_CTRL_CROSS, 21},
        {PSP_CTRL_LTRIGGER | PSP_CTRL_CIRCLE, 22},
        {PSP_CTRL_LTRIGGER | PSP_CTRL_TRIANGLE, 23},
        {PSP_CTRL_RTRIGGER | PSP_CTRL_SQUARE, 24},
        {PSP_CTRL_RTRIGGER | PSP_CTRL_CROSS, 25},
        {PSP_CTRL_RTRIGGER | PSP_CTRL_CIRCLE, 26},
        {PSP_CTRL_RTRIGGER | PSP_CTRL_TRIANGLE, 27},

        /* These are normal */
        {PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER, 16},
        {PSP_CTRL_START | PSP_CTRL_SELECT, 17},
        {PSP_CTRL_ANALUP, 0},
        {PSP_CTRL_ANALDOWN, 1},
        {PSP_CTRL_ANALLEFT, 2},
        {PSP_CTRL_ANALRIGHT, 3},
        {PSP_CTRL_UP, 4},
        {PSP_CTRL_DOWN, 5},
        {PSP_CTRL_LEFT, 6},
        {PSP_CTRL_RIGHT, 7},
        {PSP_CTRL_SQUARE, 8},
        {PSP_CTRL_CROSS, 9},
        {PSP_CTRL_CIRCLE, 10},
        {PSP_CTRL_TRIANGLE, 11},
        {PSP_CTRL_LTRIGGER, 12},
        {PSP_CTRL_RTRIGGER, 13},
        {PSP_CTRL_SELECT, 14},
        {PSP_CTRL_START, 15},
        {0, -1}};

void video_canvas_resize(struct video_canvas_s *canvas, char resize_canvas)
{
  c64_screen_w = canvas->draw_buffer->visible_width;
  c64_screen_h = canvas->draw_buffer->visible_height;
  DBG(("video_canvas_resize %d x %d", c64_screen_w, c64_screen_h));

  // Needed for palette
  if (!canvas->videoconfig->color_tables.updated)
  { /* update colors as necessary */
    video_color_update_palette(canvas);
  }
}

video_canvas_t *video_canvas_create(video_canvas_t *canvas, unsigned int *width, unsigned int *height, int mapped)
{
  *width = 480;
  *height = 272;
  canvas->depth = Screen->Depth;
  video_canvas_set_palette(canvas, canvas->palette);
  activeCanvas = canvas;
  return canvas;
}

void video_canvas_destroy(struct video_canvas_s *canvas)
{
  if (Screen)
  {
    pspImageDestroy(Screen);
    Screen = NULL;
  }
  lib_free(canvas->video_draw_buffer_callback);
}

static int video_frame_buffer_alloc(video_canvas_t *canvas,
                                    BYTE **draw_buffer,
                                    unsigned int fb_width,
                                    unsigned int fb_height,
                                    unsigned int *fb_pitch);
static void video_frame_buffer_free(video_canvas_t *canvas,
                                    BYTE *draw_buffer);
static void video_frame_buffer_clear(video_canvas_t *canvas,
                                     BYTE *draw_buffer,
                                     BYTE value,
                                     unsigned int fb_width,
                                     unsigned int fb_height,
                                     unsigned int fb_pitch);

void video_arch_canvas_init(struct video_canvas_s *canvas)
{
  canvas->video_draw_buffer_callback 
        = lib_malloc(sizeof(video_draw_buffer_callback_t));
  canvas->video_draw_buffer_callback->draw_buffer_alloc
        = video_frame_buffer_alloc;
  canvas->video_draw_buffer_callback->draw_buffer_free
        = video_frame_buffer_free;
  canvas->video_draw_buffer_callback->draw_buffer_clear
        = video_frame_buffer_clear;
}

static int video_frame_buffer_alloc(video_canvas_t *canvas,
                                    BYTE **draw_buffer,
                                    unsigned int fb_width,
                                    unsigned int fb_height,
                                    unsigned int *fb_pitch)
{
  DBG(("video_frame_buffer_alloc %d x %d", fb_width, fb_height));
  if (!Screen)
  {
    if (!(Screen = pspImageCreateVram(512, 512, PSP_IMAGE_INDEXED)))
      return -1;
  }

  *fb_pitch = (Screen->Depth / 8) * Screen->Width;
  *draw_buffer = Screen->Pixels;

  return 0;
}

static void video_frame_buffer_free(video_canvas_t *canvas, BYTE *draw_buffer)
{
}

static void video_frame_buffer_clear(video_canvas_t *canvas,
                                     BYTE *draw_buffer,
                                     BYTE value,
                                     unsigned int fb_width,
                                     unsigned int fb_height,
                                     unsigned int fb_pitch)
{
  memset(draw_buffer, value, fb_pitch * fb_height);
}

int video_canvas_set_palette(struct video_canvas_s *canvas,
                             struct palette_s *palette)
{
  if (palette == NULL)
  {
    return 0;
  }

  canvas->palette = palette;
  DBG(("video_canvas_set_palette, size:%d", palette->num_entries));
  unsigned int i;
  for (i = 0; i < palette->num_entries; i++)
  {
    Screen->Palette[i] = RGB(
        palette->entries[i].red,
        palette->entries[i].green,
        palette->entries[i].blue);
    DBG(("video_canvas_set_palette r:%d, g:%d, b:%d", palette->entries[i].red, palette->entries[i].green, palette->entries[i].blue));
  }
  Screen->PalSize = palette->num_entries;
  return 0;
}

void psp_reset_viewport(PspViewport *port, int show_border)
{
  DBG(("psp_reset_viewport geometry screen size %d x %d, gfx size %d x %d, gfx pos %d, %d", activeCanvas->geometry->screen_size.width, activeCanvas->geometry->screen_size.height,
    activeCanvas->geometry->gfx_size.width, activeCanvas->geometry->gfx_size.height, activeCanvas->geometry->gfx_position.x, activeCanvas->geometry->gfx_position.y));
  DBG(("psp_reset_viewport first_x:%d, first_line:%d, last_line:%d", activeCanvas->viewport->first_x, activeCanvas->viewport->first_line, activeCanvas->viewport->last_line));
  /* Initialize viewport */
  port->X = activeCanvas->viewport->first_x + activeCanvas->geometry->extra_offscreen_border_left - activeCanvas->viewport->x_offset;
  port->Y = activeCanvas->viewport->first_line - activeCanvas->viewport->y_offset;

  if (show_border)
  {
    /* Show normal size */
    port->Width = c64_screen_w;
    port->Height = c64_screen_h;
  }
  else
  {
    /* Shrink & reposition the viewport to show screen w/o border */
    port->Width = activeCanvas->geometry->gfx_size.width;
    port->Height = activeCanvas->geometry->gfx_size.height;
    port->X += -activeCanvas->viewport->first_x + activeCanvas->geometry->gfx_position.x;
    port->Y += -activeCanvas->viewport->first_line + activeCanvas->geometry->gfx_position.y;
  }
  DBG(("psp_reset_viewport show border %d, x:%d, y:%d, w:%d, h:%d", show_border, port->X, port->Y, port->Width, port->Height));
}

static void video_psp_display_menu()
{
  interrupt_maincpu_trigger_trap(pause_trap, NULL);
}

static void pause_trap(WORD unused_addr, void *data)
{
  DBG(("pause_trap"));
  psp_reset_viewport(&Screen->Viewport, 1);
  sound_suspend();

  psp_display_menu(); /* Display menu */

  psp_reset_viewport(&Screen->Viewport, psp_options.show_border);

  /* Set up viewing sizes */
  float ratio;
  switch (psp_options.display_mode)
  {
  default:
  case DISPLAY_MODE_UNSCALED:
    screen_w = Screen->Viewport.Width;
    screen_h = Screen->Viewport.Height;
    break;
  case DISPLAY_MODE_FIT_HEIGHT:
    ratio = (float)PL_GFX_SCREEN_HEIGHT / (float)Screen->Viewport.Height;
    screen_w = (float)Screen->Viewport.Width * ratio;
    screen_h = PL_GFX_SCREEN_HEIGHT;
    break;
  case DISPLAY_MODE_FILL_SCREEN:
    screen_w = PL_GFX_SCREEN_WIDTH;
    screen_h = PL_GFX_SCREEN_HEIGHT;
    break;
  }

  screen_x = (PL_GFX_SCREEN_WIDTH / 2) - (screen_w / 2);
  screen_y = (PL_GFX_SCREEN_HEIGHT / 2) - (screen_h / 2);

  /* Determine if NTSC mode is active */
  int vice_setting;
  resources_get_int("MachineVideoStandard", &vice_setting);
  ntsc_mode = (vice_setting == MACHINE_SYNC_NTSC);

  line_height = pspFontGetLineHeight(&PspStockFont);
  clear_screen = 1;
  show_kybd_held = 0;
  keyboard_visible = 0;

  disk_icon_offset = 0;
  tape_icon_offset = disk_icon_offset +
                     pspFontGetTextWidth(&PspStockFont, PSP_CHAR_FLOPPY);

  last_framerate = 0;
  last_percent = 0;

  keyboard_clear_keymatrix();
  sound_resume();
  vsync_suspend_speed_eval();
}

void psp_input_poll()
{
  if (psp_first_time)
  {
    video_psp_display_menu();
    psp_first_time = 0;
  }

  /* Reset joystick */
  joystick_value[psp_options.joyport] = 0;

  /* Parse input */
  static SceCtrlData pad;
  if (pspCtrlPollControls(&pad))
  {
    if (keyboard_visible)
      pl_vk_navigate(&psp_keyboard, &pad);

    const psp_ctrl_mask_to_index_map_t *current_mapping = physical_to_emulated_button_map;
    for (; current_mapping->mask; current_mapping++)
    {
      u32 code = current_map.button_map[current_mapping->index];
      u8 on = (pad.Buttons & current_mapping->mask) == current_mapping->mask;
      if (!keyboard_visible)
      {
        if (on)
        {
          if (current_mapping->index < MAP_SHIFT_START_POS)
            /* If a button set is pressed, unset it, so it */
            /* doesn't trigger any other combination presses. */
            pad.Buttons &= ~current_mapping->mask;
          else
            /* Shift mode: Don't unset the L/R; just the rest */
            pad.Buttons &= ~(current_mapping->mask &
                             ~(PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER));
        }

        if (code & KBD)
        {
          psp_keyboard_toggle(code, on);
          continue;
        }
        else if ((code & JOY) && on)
        {
          joystick_value[psp_options.joyport] |= CODE_MASK(code);
          continue;
        }
      }

      if (code & SPC)
      {
        switch (CODE_MASK(code))
        {
        case SPC_MENU:
          if (on)
          {
            video_psp_display_menu();
            return;
          }
          break;
        case SPC_KYBD:
          if (psp_options.toggle_vk)
          {
            if (show_kybd_held != on && on)
            {
              keyboard_visible = !keyboard_visible;
              keyboard_clear_keymatrix();

              if (keyboard_visible)
                pl_vk_reinit(&psp_keyboard);
              else
                clear_screen = 1;
            }
          }
          else
          {
            if (show_kybd_held != on)
            {
              keyboard_visible = on;
              if (on)
                pl_vk_reinit(&psp_keyboard);
              else
              {
                clear_screen = 1;
                keyboard_clear_keymatrix();
              }
            }
          }

          show_kybd_held = on;
          break;
        }
      }
    }
  }
}

void video_canvas_refresh(struct video_canvas_s *canvas,
                          unsigned int xs, unsigned int ys,
                          unsigned int xi, unsigned int yi,
                          unsigned int w, unsigned int h)
{
  /* This is where the emulator usually performs screen refresh */
  /* Moved it to pre-sync routine, so that drawing is performed constantly */
}

void psp_refresh_screen()
{
  if (!Screen || !activeCanvas)
    return;

  /* Wait for video synch, if enabled */
  if (psp_options.vsync && ntsc_mode)
    pspVideoWaitVSync();

  /* Update the display */
  pspVideoBegin();

  /* Clear the buffer first, if necessary */
  if (clear_screen >= 0)
  {
    clear_screen--;
    pspVideoClearScreen();
  }
  else
  {
    if (psp_options.show_fps || psp_options.show_osi)
      pspVideoFillRect(0, 0, SCR_WIDTH, line_height, PSP_COLOR_BLACK);
  }

  // Needed for palette
  if (!activeCanvas->videoconfig->color_tables.updated)
  { /* update colors as necessary */
    video_color_update_palette(activeCanvas);
  }

  /* Draw the screen */
  pl_gfx_put_image(Screen, screen_x, screen_y, screen_w, screen_h);

  /* Draw keyboard */
  if (keyboard_visible)
    pl_vk_render(&psp_keyboard);

  if (psp_options.show_fps)
  {
    static char fps_display[64];
    sprintf(fps_display, " %3.02f (%.02f%%)", last_framerate, last_percent);

    int width = pspFontGetTextWidth(&PspStockFont, fps_display);

    pspVideoPrint(&PspStockFont, SCR_WIDTH - width, 0, fps_display, PSP_COLOR_WHITE);
  }

  /* Display any status indicators */
  if (psp_options.show_osi)
  {
    /* "Disk busy" indicator */
    if (drive_led_on)
      pspVideoPrint(&PspStockFont, disk_icon_offset, 0, PSP_CHAR_FLOPPY, PSP_COLOR_GREEN);
    if (tape_led_on)
      pspVideoPrint(&PspStockFont, tape_icon_offset, 0, PSP_CHAR_TAPE, PSP_COLOR_GREEN);
  }

  pspVideoEnd();

  /* Swap buffers */
  pspVideoSwapBuffers();
}

void ui_display_speed(float percent, float framerate, int warp_flag)
{
  last_framerate = framerate;
  last_percent = percent;
}

int video_init()
{
  return 0;
}

void video_shutdown()
{
}

int video_arch_cmdline_options_init(void)
{
  return 0;
}

int video_arch_resources_init()
{
  if (!pl_vk_load(&psp_keyboard, "c64.l2", "c64keys.png", NULL, psp_keyboard_toggle))
    return 1;
  return 0;
}

void video_arch_resources_shutdown()
{
  /* Destroy keyboard */
  pl_vk_destroy(&psp_keyboard);
}

static inline void psp_keyboard_toggle(unsigned int code, int on)
{
  if (CODE_MASK(code) == 0xff)
  {
    keyboard_set_keyarr(-3, 0, on); /* TODO: better */
    return;
  }
  keyboard_set_keyarr(CKROW(code), CKCOL(code), on);
}

void ui_display_drive_led(int drive_number, unsigned int led_pwm1,
                          unsigned int led_pwm2)
{
  drive_led_on = (led_pwm1 > 100);
}

void ui_display_tape_motor_status(int motor)
{
  tape_led_on = motor;
}

char video_canvas_can_resize(struct video_canvas_s *canvas)
{
  return 1;
}