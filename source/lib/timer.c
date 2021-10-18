/************************************************************************************//**
* \file         timer.c
* \brief        Generic timer driver source file.
*
****************************************************************************************/

/****************************************************************************************
* Include files
****************************************************************************************/
#include <assert.h>                         /* for assertions                          */
#include <stdint.h>                         /* for standard integer types              */
#include <stddef.h>                         /* for NULL declaration                    */
#include <stdbool.h>                        /* for boolean type                        */
#include <stdlib.h>                         /* for standard library                    */
#include <threads.h>                        /* Multithreading                          */
#include <stdatomic.h>                      /* Atomic operations                       */
#include "util.h"                           /* Utility functions                       */
#include "timer.h"                          /* timer driver                            */


/****************************************************************************************
* Type definitions
****************************************************************************************/
typedef struct t_timer_node
{
  /** \brief Callback function pointer to call upon timeout. */
  tTimerEventCallback callbackFcn;  
  /** \brief Boolean flag to indicate if the timer is active or not. */
  bool running;
  /** \brief Timestamp of when the timer was started. */
  uint64_t startTime;
  /** \brief Period of the timer in microseconds. */
  uint32_t period_us;
  /** \brief Pointer to the next node in the list or NULL if it is the list end. */
  struct t_timer_node volatile * nextNode;
} tTimerNode;


/****************************************************************************************
* Local data declarations
****************************************************************************************/
/** \brief Identifier of the polling thread. */
static thrd_t timerPollingThreadId;

/** \brief Boolean flag that indicates if the polling thread is running or not. */
static bool timerPollingThreadRunning;

/** \brief Atomic boolean that is used to inform the polling thread to stop running. */
static atomic_bool timerStopPollingThread;

/** \brief Mutex for mutual exlusive access to the timer linked list. */
static mtx_t timerListMutex;

/** \brief Timer linked list. */
static tTimerNode volatile * timerList;


/****************************************************************************************
* Function prototypes
****************************************************************************************/
static int TimerPollingThread(void * param);


/************************************************************************************//**
** \brief     Initializes the timer driver. Think of it as the constructor, if this
**            driver was a C++ class.
**
****************************************************************************************/
void TimerInit(void)
{
  /* Initialize locals. */
  timerPollingThreadId = 0;
  timerPollingThreadRunning = false;
  atomic_init(&timerStopPollingThread, false);
  timerList = NULL;

  /* Initialize the mutex. */
  if (mtx_init(&timerListMutex, mtx_plain) != thrd_success)
  {
    assert(false);
  }

  /* Start the polling thread for processing timer related events. */
  if (thrd_create(&timerPollingThreadId, (thrd_start_t)TimerPollingThread, NULL) 
      == thrd_success)
  {
    /* Set flag. */
    timerPollingThreadRunning = true;
  }
} /*** end of TimerInit ***/


/************************************************************************************//**
** \brief     Terminates the timer driver. Think of it as the destructor if this driver
**            was a C++ class.
**
****************************************************************************************/
void TimerTerminate(void)
{
  tTimerNode volatile * aTimer;  
  tTimerNode volatile * delTimer;  

  /* Stop the timer's polling thread. */
  if (timerPollingThreadRunning)
  {
    /* Set atomic boolean flag to request the thread to stop. */
    atomic_store(&timerStopPollingThread, true);
    /* Wait until the thread terminated. */
    thrd_join(timerPollingThreadId, NULL);
  }

  /* Obtain mutual exclusion to the timer linked list. */
  mtx_lock(&timerListMutex);   
  /* Begin at the start of the list. */
  aTimer = timerList;
  /* Iterate over the entire list. */
  while (aTimer != NULL)
  {
    /* Store the one to delete. */
    delTimer = aTimer;
    /* Move to the next one, before actually deleting it. */
    aTimer = aTimer->nextNode;
    /* Release memory of the one that is to be deleted. */
    free((void *)delTimer);
  }
  /* Set list to empty now that all nodes have been released. */
  timerList = NULL;
  /* Release mutual exclusion to the timer linked list. */
  mtx_unlock(&timerListMutex);   

  /* Destroy the mutex. */
  mtx_destroy(&timerListMutex);

  /* Reset locals. */
  atomic_init(&timerStopPollingThread, false);
  timerPollingThreadRunning = false;
  timerPollingThreadId = 0;
} /*** end of TimerTerminate ***/


/************************************************************************************//**
** \brief     Create a new timer. It returns the handle of the newly created timer, which
**            is needed for other timer related API functions.
** \param     callbackFcn Callback function pointer of the function that should be called
**            upon each expiration event.
** \return    Timer handle if successful, NULL otherwise.
**
****************************************************************************************/
tTimer TimerCreate(tTimerEventCallback callbackFcn)
{
  tTimer result = NULL;
  tTimerNode volatile * newTimer;  

  /* Verify parameter. */
  assert(callbackFcn != NULL);

  /* Only continue with valid parameter. */
  if (callbackFcn != NULL)  
  {
    /* Allocate memory for the new timer. */
    newTimer = (tTimerNode volatile *)malloc(sizeof(tTimerNode));

    /* Verify that memory could be allocated. */
    assert(newTimer != NULL);

    /* Only continue when memory was allocated. */
    if (newTimer != NULL)
    {
      /* Initialize the timer. */
      newTimer->running = false;
      newTimer->startTime = 0;
      newTimer->period_us = 0;
      newTimer->callbackFcn = callbackFcn;
      newTimer->nextNode = NULL;

      /* Obtain mutual exclusion to the timer linked list. */
      mtx_lock(&timerListMutex);  
      /* Add the new node at the start of the linked list. */
      newTimer->nextNode = timerList;
      timerList = newTimer;
      /* Release mutual exclusion to the timer linked list. */
      mtx_unlock(&timerListMutex);   

      /* Update the result. */
      result = (tTimer)newTimer;
    }
  }

  /* Give the result back to the caller. */
  return result;
} /*** end of TimerCreate ***/


/************************************************************************************//**
** \brief     Deletes a previously created timer. 
** \param     timer Handle of the timer to delete.
**
****************************************************************************************/
void TimerDelete(tTimer timer)
{
  tTimerNode volatile * delTimer = (tTimerNode volatile *)timer;  
  tTimerNode volatile * prevTimer;  
  tTimerNode volatile * aTimer;  

  /* Verify parameter. */
  assert(timer != NULL);

  /* Only continue with valid parameter. */
  if (timer != NULL)  
  {
    /* Obtain mutual exclusion to the timer linked list. */
    mtx_lock(&timerListMutex);
    /* Can only delete a node from the list if the list is not empty. */
    if (timerList != NULL)
    {
      /* Is the node to be deleted right at the start of the list? */
      if (timerList == delTimer)
      {
        /* Remove it from the list and free its allocated memory. */
        timerList = delTimer->nextNode;
        free((void *)delTimer);
      }
      /* Not at the start of the list, so we need to search for it. */
      else
      {
        /* Begin at the start of the list. */
        aTimer = timerList;
        /* Loop through the list to find the to be deleted node. */
        while (aTimer->nextNode != NULL)
        {
          /* Is the next node the one to delete? */
          if (aTimer->nextNode == delTimer)
          {
            /* Remove it from the list and free its allocated memory. */
            aTimer->nextNode = delTimer->nextNode;
            free((void *)delTimer);
            /* All done, so no point in continuing the loop. */
            break;
          }
          /* Continue with the next node. */
          aTimer = aTimer->nextNode;
        }
      }
    }
    /* Release mutual exclusion to the timer linked list. */
    mtx_unlock(&timerListMutex);   
  }
} /*** end of TimerDelete ***/


/************************************************************************************//**
** \brief     Starts the timer by scheduling a timer event to occur in time specified by
**            the period parameter.
** \param     timer Handle of the timer to start.
** \param     period Number of milliseconds after which the timer event should occur.
**
****************************************************************************************/
void TimerStart(tTimer timer, uint32_t period)
{
  tTimerNode volatile * aTimer = (tTimerNode volatile *)timer;  

  /* Verify parameter. */
  assert(timer != NULL);

  /* Only continue with valid parameter. */
  if (timer != NULL)  
  {
    /* Obtain mutual exclusion to the timer linked list. */
    mtx_lock(&timerListMutex);  
    /* Set the start time, store the period and start the timer. 
     */
    aTimer->startTime = UtilSystemTime();
    aTimer->period_us = period * 1000;
    aTimer->running = true;
    /* Release mutual exclusion to the timer linked list. */
    mtx_unlock(&timerListMutex);   
  }
} /*** end of TimerStart ***/


/************************************************************************************//**
** \brief     Restarts the timer using the same period as the last event. Typically 
**            called from the timer's event callback to create a pure cyclical timer.
** \param     timer Handle of the timer to restart.
**
****************************************************************************************/
void TimerRestart(tTimer timer)
{
  tTimerNode volatile * aTimer = (tTimerNode volatile *)timer;  
  uint64_t now = UtilSystemTime();

  /* Verify parameter. */
  assert(timer != NULL);

  /* Only continue with valid parameter. */
  if (timer != NULL)  
  {
    /* Obtain mutual exclusion to the timer linked list. */
    mtx_lock(&timerListMutex);  
    /* Add period to the start time to restart it. */
    aTimer->startTime += aTimer->period_us;
    /* Did the timer already overrun? */
    if ((now - aTimer->startTime) > aTimer->period_us)
    {
      /* Schedule the timer to trigger the timeout event right away. */
      aTimer->startTime = now - aTimer->period_us;
    }
    aTimer->running = true;
    /* Release mutual exclusion to the timer linked list. */
    mtx_unlock(&timerListMutex);   
  }
} /*** end of TimerRestart ***/


/************************************************************************************//**
** \brief     Stops the timer that was previously started.
** \param     timer Handle of the timer to delete.
**
****************************************************************************************/
void TimerStop(tTimer timer)
{
  tTimerNode volatile * aTimer = (tTimerNode volatile *)timer;  

  /* Verify parameter. */
  assert(timer != NULL);

  /* Only continue with valid parameter. */
  if (timer != NULL)  
  {
    /* Obtain mutual exclusion to the timer linked list. */
    mtx_lock(&timerListMutex);   
    /* Stop the timer. */
    aTimer->running = false;
    /* Release mutual exclusion to the timer linked list. */
    mtx_unlock(&timerListMutex);   
  }
} /*** end of TimerStop ***/


/************************************************************************************//**
** \brief     Polling thread that handles detection and processing of timer related
**            events.
** \param     arg Pointer to thread parameters.
** \return    Thread return value.
**
****************************************************************************************/
static int TimerPollingThread(void * param)
{
  tTimerNode volatile * aTimer;
  uint64_t now;
  tTimerEventCallback callbackFcnCopy;

  /* Enter the thread's loop and run it, until a stop is requested. */
  while (!atomic_load(&timerStopPollingThread))
  {
    /* Get current system time. */
    now = UtilSystemTime();
    /* Obtain mutual exclusion to the timer linked list. */
    mtx_lock(&timerListMutex);   
    /* Begin at the start of the list. */
    aTimer = timerList;
    /* Iterate over the entire list. */
    while (aTimer != NULL)
    {
      /* Is this timer running? */
      if (aTimer->running)
      {
        /* Did this timer timeout? */
        if ((now - aTimer->startTime) > aTimer->period_us)
        {
          /* Make a copy of its callback function pointer. */
          callbackFcnCopy = aTimer->callbackFcn;
          /* Invoke the callback function, but make sure to do it outside of the mutex
           * lock, because the user might call other timer API functions inside the
           * callback, which would result in a deadlock.
           */
          if (callbackFcnCopy != NULL)
          {
            /* Release mutual exclusion to the timer linked list. */
            mtx_unlock(&timerListMutex);   
            /* Invoke the callback. */
            callbackFcnCopy();
            /* Obtain mutual exclusion to the timer linked list. */
            mtx_lock(&timerListMutex);   
          }
        }
      }
      /* Continue with the next timer. */
      aTimer = aTimer->nextNode;
    }
    /* Release mutual exclusion to the timer linked list. */
    mtx_unlock(&timerListMutex);   

    /* Wait a little to not starve the CPU. Note that one millisecond is the smallest 
     * timer interval. Yet the thread executation takes a litle time to, so we should
     * wait a bit less than one millisecond, for example 500 microseconds.
     */
    UtilSleep(500);
  }

  /* Shut down the thread. */
  thrd_exit(EXIT_SUCCESS);
} /*** end of TimerPollingThread ***/


/*********************************** end of timer.c ************************************/
