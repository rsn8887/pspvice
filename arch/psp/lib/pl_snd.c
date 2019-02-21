/* psplib/pl_snd.c: Simple sound playback library
   Copyright (C) 2007-2009 Akop Karapetyan

   $Id$

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Author contact information: 
     Email: dev@psp.akop.org
*/

#include "pl_snd.h"

#include <stdio.h>
#include <pspaudio.h>
#include <pspthreadman.h>
#include <string.h>
#include <malloc.h>

#define DEFAULT_SAMPLES 512

static volatile int sound_stop;

typedef struct {
  short *sample_buffer[2];
  unsigned int samples[2];
  pl_snd_callback callback;
  int sound_ch_handle;
  int thread_handle;
  int paused;
} pl_snd_channel_info;

static pl_snd_channel_info sound_stream;

static int channel_thread(int args, void *argp);
static void free_buffers();

int pl_snd_init(int sample_count)
{
  int j, failed = 0;
  sound_stop = 0;

  /* Check sample count */
  if (sample_count <= 0) 
    sample_count = DEFAULT_SAMPLES;
  sample_count = PSP_AUDIO_SAMPLE_ALIGN(sample_count);

  sound_stream.sound_ch_handle = -1;
  sound_stream.thread_handle = -1;
  sound_stream.callback = NULL;
  sound_stream.paused = 1;

  for (j = 0; j < 2; j++)
  {
    sound_stream.sample_buffer[j] = NULL;
    sound_stream.samples[j] = 0;
  }

  /* Initialize buffers */
  sound_stream.sample_buffer[0] = memalign(64, 2 * sample_count * sizeof(pl_snd_sample)); // Alloc for both buffers!
  if (!sound_stream.sample_buffer[0])
  {
    return 0;
  }
  memset(sound_stream.sample_buffer[0], 0, 2 * sample_count * sizeof(pl_snd_sample));	// clean the buffer 
  sound_stream.sample_buffer[1] = sound_stream.sample_buffer[0] + sample_count;
  sound_stream.samples[0] = sample_count;
  sound_stream.samples[1] = sample_count;

  /* Initialize channels */
  sound_stream.sound_ch_handle = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, sample_count, PSP_AUDIO_FORMAT_MONO);
  if (sound_stream.sound_ch_handle < 0)
  { 
    if (sound_stream.sound_ch_handle != -1)
    {
      sceAudioChRelease(sound_stream.sound_ch_handle);
      sound_stream.sound_ch_handle = -1;
    }

    free_buffers();
    return 0;
  }

  char label[16];
  strcpy(label, "audiot0");
  sound_stream.thread_handle = sceKernelCreateThread(label, (void*)&channel_thread, 0x12, 0x10000, 0, NULL);
  if (sound_stream.thread_handle >= 0)
  {
    if (sceKernelStartThread(sound_stream.thread_handle, 0, NULL) < 0)
    {
      failed = 1;
    }
  }
  else
  {
    sound_stream.thread_handle = -1;
    failed = 1;
  }

  if (failed)
  {
    sound_stop = 1;
    if (sound_stream.thread_handle != -1)
    {
      sceKernelDeleteThread(sound_stream.thread_handle);
    }

    sound_stream.thread_handle = -1;

    free_buffers();
    return 0;
  }

  return sample_count;
}

void pl_snd_shutdown()
{
  sound_stop = 1;

  if (sound_stream.thread_handle != -1)
  {
    sceKernelDeleteThread(sound_stream.thread_handle);
  }

  sound_stream.thread_handle = -1;

  if (sound_stream.sound_ch_handle != -1)
  {
    sceAudioChRelease(sound_stream.sound_ch_handle);
    sound_stream.sound_ch_handle = -1;
  }

  free_buffers();
}

static int channel_thread(int args, void *argp)
{
  int bufidx = 0;
  short* bufptr = NULL;
  
  while (!sound_stop)
  {
    if (sound_stream.paused)
		{
			do
			{
				sceKernelDelayThread(100000);
			} while (sound_stream.paused);
		} 

    bufptr = sound_stream.sample_buffer[bufidx];

    if (sound_stream.callback)
    {
      /* Use callback to fill buffer */
      sound_stream.callback(bufptr, sound_stream.samples[bufidx]);
    }
    else
    {
      memset(bufptr, 0, sound_stream.samples[bufidx] * sizeof(pl_snd_sample));
    }
    
    /* Play sound */
	  sceAudioOutputBlocking(sound_stream.sound_ch_handle, PSP_AUDIO_VOLUME_MAX, bufptr);
	
    /* Switch active buffer */
    bufidx ^= 1;
  }

  sceKernelExitThread(0);
  return 0;
}

static void free_buffers()
{
  if (sound_stream.sample_buffer[0])
  {
    free(sound_stream.sample_buffer[0]);
    sound_stream.sample_buffer[0] = NULL;
    sound_stream.sample_buffer[1] = NULL;
  }
 }

int pl_snd_set_callback(pl_snd_callback callback)
{
  sound_stream.callback = callback;
  return 1;
}

int pl_snd_pause()
{
  sound_stream.paused = 1;
  return 1;
}

int pl_snd_resume()
{
  sound_stream.paused = 0;
  return 1;
}
