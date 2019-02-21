#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pspkernel.h>

#include "main.h"
#include "machine.h"

#include "lib/pl_snd.h"
#include "lib/video.h"
#include "lib/pl_psp.h"
#include "lib/ctrl.h"

PSP_MODULE_INFO(PSP_APP_NAME, 0, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER /*| PSP_THREAD_ATTR_VFPU*/);
PSP_HEAP_SIZE_KB(-256);

static void ExitCallback(void* arg)
{
  ExitPSP = 1;
}

int main(int argc, char *argv[])
{
  /* Initialize PSP */
  pl_psp_init(argv[0]);
  pl_snd_init(SND_FRAG_SIZE);
  pspCtrlInit();
  pspVideoInit();

  /* Initialize callbacks */
  pl_psp_register_callback(PSP_EXIT_CALLBACK, ExitCallback, NULL);
  pl_psp_start_callback_thread();

  main_program(argc, argv);
  main_exit();

  /* Release PSP resources */
  pl_snd_shutdown();
  pspVideoShutdown();
  pl_psp_shutdown();

  return(0);
}

void main_exit()
{
    machine_shutdown();
}

