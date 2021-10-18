/************************************************************************************//**
* \file         keys.c
* \brief        Input key detection driver source file.
*
****************************************************************************************/

/****************************************************************************************
* Include files
****************************************************************************************/
#include <assert.h>                         /* for assertions                          */
#include <stdint.h>                         /* for standard integer types              */
#include <stddef.h>                         /* for NULL declaration                    */
#include <stdbool.h>                        /* for boolean type                        */
#include <stdio.h>                          /* for standard input/output functions     */
#include <stdlib.h>                         /* for standard library                    */
#include <termios.h>                        /* Terminal I/O functions                  */
#include <unistd.h>                         /* UNIX standard functions                 */
#include <threads.h>                        /* Multithreading                          */
#include <stdatomic.h>                      /* Atomic operations                       */
#include "util.h"                           /* Utility functions                       */
#include "keys.h"                           /* Input key detection driver              */


/****************************************************************************************
* Local data declarations
****************************************************************************************/
/** \brief Function pointer for the key pressed event callback handler. Volatile because
 *  it is shared with the event thread.
 */
static volatile tKeysEventCallback keysEventCallback;

/** \brief Identifier of the key pressed detection event thread. */
static thrd_t keysEventThreadId;

/** \brief Boolean flag that indicates if the key pressed detection event thread is
 *  running or not.
 */
static bool keysEventThreadRunning;

/** \brief Atomic boolean that is used to inform the event thread to stop running. */
static atomic_bool keysStopEventThread;


/****************************************************************************************
* Function prototypes
****************************************************************************************/
static int KeysEventThread(void * param);


/************************************************************************************//**
** \brief     Initializes the input key detection driver. Think of it as the constructor,
**            if this driver was a C++ class.
** \param     callbackFnc Callback function pointer.
**
****************************************************************************************/
void KeysInit(tKeysEventCallback callbackFcn)
{
  /* Initialize locals. */
  keysEventCallback = NULL;
  keysEventThreadId = 0;
  keysEventThreadRunning = false;
  atomic_init(&keysStopEventThread, false);

  /* Verify parameter. */
  assert(callbackFcn != NULL);

  /* Only continue with valid parameter. */
  if (callbackFcn != NULL)
  {
    /* Set the message received callback handler. */
    keysEventCallback = callbackFcn;
  }

  /* Start the key pressed detection thread. */
  if (thrd_create(&keysEventThreadId, (thrd_start_t)KeysEventThread, NULL) 
      == thrd_success)
  {
    /* Set flag. */
    keysEventThreadRunning = true;
  }
} /*** end of KeysInit ***/


/************************************************************************************//**
** \brief     Terminates the input key detection driver. Think of it as the destructor if
**            this driver was a C++ class.
**
****************************************************************************************/
void KeysTerminate(void)
{
  /* Stop the key pressed detection thread. */
  if (keysEventThreadRunning)
  {
    /* Set atomic boolean flag to request the thread to stop. */
    atomic_store(&keysStopEventThread, true);
    /* Wait until the thread terminated. */
    thrd_join(keysEventThreadId, NULL);
  }

  /* Reset locals. */
  atomic_init(&keysStopEventThread, false);
  keysEventThreadRunning = false;
  keysEventThreadId = 0;
  keysEventCallback = NULL;
} /*** end of KeysTerminate ***/


/************************************************************************************//**
** \brief     Event thread that handles the asynchronous reception of data from the CAN
**            interface.
** \param     arg Pointer to thread parameters.
** \return    Thread return value.
**
****************************************************************************************/
static int KeysEventThread(void * param)
{
  struct termios termiosDefault, termiosRaw;
  struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };
  fd_set fs;

  /* Obtain current default standard input parameters. */
  tcgetattr(STDIN_FILENO, &termiosDefault);

  /* Initialize standard input parameters for raw (non-canonical) mode and disabled 
   * echo.
   */
  termiosRaw = termiosDefault;
  termiosRaw.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE);

  /* Configure standard input for raw mode. */
  tcsetattr(STDIN_FILENO, TCSANOW, &termiosRaw);

  /* Enter the thread's loop and run it, until a stop is requested. */
  while (!atomic_load(&keysStopEventThread))
  {
    /* Initialize the standard input file descriptor set. */
    FD_ZERO(&fs);
    FD_SET(STDIN_FILENO, &fs);
    /* Check for activity on the standard input without timeout, so non-blocking. */
    select(STDIN_FILENO + 1, &fs, 0, 0, &tv);
    /* Was activity detected? */
    if (FD_ISSET(STDIN_FILENO, &fs))
    {
      /* Read the input character from the standard input. */
      int c = getchar();
      /* Was it a valid character? */
      if (c != EOF)
      {
        /* Call the key pressed callback. */
        if (keysEventCallback != NULL)
        {
          keysEventCallback((char)c);
        }
      }
    }

    /* Wait a little to not starve the CPU. */
    UtilSleep(5 * 1000);
  }

  /* Restore the original standard input parameters. */
  tcsetattr(STDIN_FILENO, TCSANOW, &termiosDefault);

  /* Shut down the thread. */
  thrd_exit(EXIT_SUCCESS);
} /*** end of KeysEventThread ***/


/*********************************** end of keys.c *************************************/
