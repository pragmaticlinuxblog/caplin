/************************************************************************************//**
* \file         util.c
* \brief        Generic utilities source file.
*
****************************************************************************************/

/****************************************************************************************
* Include files
****************************************************************************************/
#include <assert.h>                         /* for assertions                          */
#include <stdint.h>                         /* for standard integer types              */
#include <stddef.h>                         /* for NULL declaration                    */
#include <stdbool.h>                        /* for boolean type                        */
#include <time.h>                           /* Date and time utilities                 */
#include <threads.h>                        /* Multithreading                          */
#include "util.h"                           /* Utility functions                       */


/************************************************************************************//**
** \brief     Sleeps the current thread for the specified amount of microseconds.
** \param     micros Amount of microseconds to sleep.
**
****************************************************************************************/
void UtilSleep(uint32_t micros)
{
  struct timespec sleepTime = { 0 };

  /* No need to sleep if a zero parameter value was passed. */
  if (micros > 0)
  {
    /* Convert milliseconds to timespec. */
    sleepTime.tv_sec = micros / (1000 * 1000);
    sleepTime.tv_nsec = (micros % (1000 * 1000)) * 1000;
    /* Sleep the current thread. */
    thrd_sleep(&sleepTime, NULL); 
  }
} /*** end of UtilSleep ***/


/************************************************************************************//**
** \brief     Gets the current system time in microseconds.
** \return    System time in microseconds.
**
****************************************************************************************/
uint64_t UtilSystemTime(void)
{
  uint64_t result = 0;
  struct timespec now;

  /* Obtain the current time. */
  if (timespec_get(&now, TIME_UTC) != 0)
  {
    /* Convert to microseconds. */
    result = ((int64_t)now.tv_sec * 1000 * 1000) + ((int64_t)now.tv_nsec / 1000);
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of UtilSystemTime ***/


/*********************************** end of util.c *************************************/
