/*
 * soundpsp.c - Implementation of the PSP sound driver
 *              Depends on psplib (http://svn.akop.org/psp/trunk/libpsp)
 *              Uses FIFO by (c) 2000-2002, David Olofson (used orig. in Fuse)
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

#include <string.h>
#include <pspkernel.h>

#include "vice.h"
#include "log.h"
#include "vsync.h"

#include "sound.h"
#include "log.h"
#include "lib/pl_snd.h"
#include "lib.h"
#include "sfifo.h"

static sfifo_t sound_fifo;
static int sound_initted = 0;
static int sound_rendering = 0;

static void psp_sound_callback(pl_snd_sample* stream, unsigned int samples);

static int psp_sound_init(const char *param, int *speed,
		    int *fragsize, int *fragnr, int *channels)
{
  log_debug("psp_sound_init defaults speed:%d, fragsize:%d, fragnr:%d", *speed, *fragsize, *fragnr);
  *speed = 44100;
  *fragsize = SND_FRAG_SIZE;//(*speed)/50;
  *channels = 1;

  int buffer_size = (*fragnr) * (*channels) * (*fragsize) + 1;
  if (sfifo_init(&sound_fifo, buffer_size))
    return 1;

  log_debug("psp_sound_init speed:%d, fragsize:%d, fragnr:%d, buffer size:%d", *speed, *fragsize, *fragnr, buffer_size);

  pl_snd_set_callback(psp_sound_callback);
  pl_snd_resume();
  sound_rendering = 1;
  sound_initted = 1;

  return 0;
}

static int psp_sound_write(SWORD *pbuf, size_t nr)
{
  //log_debug("psp_sound_write bytes:%d", nr);
  int i = 0;

  /* Convert to bytes */
  BYTE *bytes = (BYTE*)pbuf;
  nr <<= 1;

  while (nr && sound_rendering)
  {
    if ((i = sfifo_write(&sound_fifo, bytes, nr)) < 0)
      break;
    else if (!i)
      sceKernelDelayThread(50);
/*
    {
      sfifo_flush(&sound_fifo);
      continue;
    }
*/

    bytes += i;
    nr -= i;
  }

  return 0;
}

static int psp_sound_suspend(void)
{
  if (!sound_initted) return 0;
  pl_snd_pause();
  sound_rendering = 0;
  return 0;
}

static int psp_sound_resume(void)
{
  if (!sound_initted) return 0;
  pl_snd_resume();
  sound_rendering = 1;
  return 0;
}

static void psp_sound_close(void)
{
  psp_sound_suspend();
  sfifo_flush(&sound_fifo);
  sfifo_close(&sound_fifo);
  sound_initted = 0;
  log_debug("psp_sound_close");
}

static sound_device_t psp_sound =
{
    "PSP",
    psp_sound_init,
    psp_sound_write,
    NULL,
    NULL,
    NULL,
    psp_sound_close,
    psp_sound_suspend,
    psp_sound_resume,
    0
};

int sound_init_psp_device(void)
{
    return sound_register_device(&psp_sound);
}

void sound_flush_psp(void)
{
  if (sound_initted)
  {
    sfifo_flush(&sound_fifo);
  }
}

static void psp_sound_callback(pl_snd_sample* stream, unsigned int samples)
{
  //log_debug("psp_sound_callback samples:%d", samples);
  unsigned int length = samples << 1; /* 2 bytes per sample */
  if (sfifo_used(&sound_fifo) <= 0)
  {
    /* Render silence if not enough sound data */
    memset(stream, 0, length);
  }
  else
  {
    sfifo_read(&sound_fifo, stream, length);
  }
}

