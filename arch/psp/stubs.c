/*
 * ui.c - PSP stub/mostly empty functions.
 *
 * Written by
 *  Akop Karapetyan <dev@psp.akop.org>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"
#include "ui.h"
#include "cmdline.h"
#include "resources.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

static const cmdline_option_t cmdline_options[] = {
    { NULL },
};

static const resource_int_t resources_int[] = {
    { NULL }
};

void ui_display_volume(int vol)
{
}

/* Display a mesage without interrupting emulation */
void ui_display_statustext(const char *text, int fade_out)
{
}

void ui_pause_emulation(int flag)
{
}

int ui_emulation_is_paused(void)
{
  return 0;
}

void ui_display_drive_current_image(unsigned int drive_number, const char *image)
{
}

void ui_display_tape_control_status(int control)
{
}

void ui_display_tape_counter(int counter)
{
}

void ui_display_tape_current_image(const char *image)
{
}

void ui_display_playback(int playback_status, char *version)
{
}

void ui_display_recording(int recording_status)
{
}

void ui_display_drive_track(unsigned int drive_number,
                                   unsigned int drive_base,
                                   unsigned int half_track_number)
{
}

void ui_enable_drive_status(ui_drive_enable_t state,
                                   int *drive_led_color)
{
}

/* tape-related ui, dummies so far */
void ui_set_tape_status(int tape_status)
{
}

/* Update all the menus according to the current settings.  */
void ui_update_menus(void)
{
}

int ui_extend_image_dialog()
{
  return 0;
}

void ui_dispatch_events()
{
}

void ui_resources_shutdown()
{
}

int video_init_cmdline_options(void)
{
    return cmdline_register_options(cmdline_options);
}

int ui_cmdline_options_init(void)
{
  return cmdline_register_options(cmdline_options);
}

int ui_init_finish()
{
  return 0;
}

int ui_init_finalize()
{
  return 0;
}

char* ui_get_file(const char *format,...)
{
    return NULL;
}

void ui_display_joyport(BYTE *joyport)
{
  /* needed */
}

void ui_display_event_time(unsigned int current, unsigned int total)
{
  /* needed */
}

int ui_resources_init(void)
{
  return resources_register_int(resources_int);
}

/* Report an error to the user.  */
void ui_error(const char *format,...)
{
#ifdef PSP_DEBUG
  FILE *error_log = fopen("errorlog.txt", "a");

  va_list ap;
  va_start (ap, format);
  vfprintf (error_log, format, ap);
  va_end (ap);

  fclose(error_log);
#endif
}

void fullscreen_capability()
{
}

