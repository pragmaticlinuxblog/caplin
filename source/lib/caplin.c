/************************************************************************************//**
* \file         caplin.c
* \brief        CAN application programming for Linux source file.
*
****************************************************************************************/

/****************************************************************************************
* Include files
****************************************************************************************/
#include <stdint.h>                         /* for standard integer types              */
#include <stdbool.h>                        /* for boolean type                        */
#include <stdio.h>                          /* for standard input/output functions     */
#include <stdlib.h>                         /* for standard library                    */
#include <stdatomic.h>                      /* Atomic operations                       */
#include <signal.h>                         /* Signal handling                         */
#include "util.h"                           /* Utility functions                       */
#include "timer.h"                          /* Timer driver                            */
#include "keys.h"                           /* Input key detection driver              */
#include "can.h"                            /* CAN driver                              */


/****************************************************************************************
* Local constant declarations
****************************************************************************************/
/** \brief Name of the SocketCAN device to use. Run 'ip addr | grep "can" for a list of
 *  all available CAN network interfaces and update this value to whichever CAN network
 *  interface you want to use.
 */
static const char * canDevice = "vcan0";


/****************************************************************************************
* Local data declarations
****************************************************************************************/
/** \brief Atomic boolean that is used to request a program exit. */
static atomic_bool appExitProgram;


/****************************************************************************************
* External function prototypes
****************************************************************************************/
/* OnPreStart is called upon program startup, before connecting to the CAN network. */
extern void OnPreStart(void);
/* OnStart is called upon program startup, after connecting to the CAN network. */
extern void OnStart(void);
/* OnPreStop is called upon program exit, before disconnecting from the CAN network. */
extern void OnPreStop(void);
/* OnStop is called upon program exit, after disconnecting from the CAN network. */
extern void OnStop(void);
/* OnMessage is called each time a new CAN message was received. */
extern void OnMessage(tCanMsg const * msg);
/* OnKey is called each time a key was pressed on the keyboard. */
extern void OnKey(char key);


/****************************************************************************************
* Function prototypes
****************************************************************************************/
static void AppKeyPressedCallback(char key);
static void AppInterruptSignalHandler(int signum);


/************************************************************************************//**
** \brief     This is the program entry point.
** \param     argc Number of program arguments.
** \param     argv Array with program arguments.
** \return    Program return code. 0 for success, error code otherwise.
**
****************************************************************************************/
int main(void)
{
  /* Initialize locals. */
  atomic_init(&appExitProgram, false);

  /* Initialize the timer driver. */
  TimerInit();
  /* Initialize the input key detection driver. */
  KeysInit(AppKeyPressedCallback);
  /* Initialization the CAN driver. */
  CanInit(OnMessage, NULL);

  /* Register interrupt signal handler for when CTRL+C was pressed. */
  signal(SIGINT,AppInterruptSignalHandler);
  /* Call the OnPreStart callback. */
  OnPreStart();
  /* Connect to the CAN bus. */
  if (!CanConnect(canDevice))
  {
    printf("Could not connect to CAN network interface %s.\n", canDevice);
  }
  /* Call the OnStart callback. */
  OnStart();

  /* Enter the program loop until an exit is requested. */
  while (!atomic_load(&appExitProgram))
  {
    /* Noting to do here, because the user's CAN application is event driven. Just 
     * delay a little to not starve the CPU. 
     */
    UtilSleep(50 * 1000);
  }

  /* Call the OnPreStop callback. */
  OnPreStop();
  /* Disconnect from the CAN bus. */
  CanDisconnect();
  /* Call the OnStop callback. */
  OnStop();

  /* Terminate the timer driver. */
  TimerTerminate();
  /* Terminate the CAN driver. */
  CanTerminate();
  /* Terminate the input key detection driver. */
  KeysTerminate();

  /* Exit the program. */
  return EXIT_SUCCESS;
} /*** end of main ***/


/************************************************************************************//**
** \brief     Application callback that gets called upon keyboard key pressed event.
** \param     key ASCII code of the pressed key.
**
****************************************************************************************/
static void AppKeyPressedCallback(char key)
{
  /* -------------------------- ESC key pressed? --------------------------------------*/
  if (key == 27)
  {
    /* Set request flag to exit the program when the ESC key was pressed. */
    atomic_store(&appExitProgram, true);
  }
  /* All other keys can be processed by the user's CAN application. */
  else
  {
    /* Call the OnKey callback. */
    OnKey(key);
  }
} /*** end of AppKeyPressedCallback ***/


/************************************************************************************//**
** \brief     Application callback that gets called when CTRL+C was pressed to quit the
**            program.
** \param     signum Signal number (not used)
**
****************************************************************************************/
static void AppInterruptSignalHandler(int signum)
{
  /* Set request flag to exit the program when the CTRL+C key combo was pressed. */
  atomic_store(&appExitProgram, true);
} /*** end of AppInterruptSignalHandler ***/


/************************************************************************************//**
** \brief     Default callback that gets called upon startup, before connecting to the
**            CAN network.
**
****************************************************************************************/
__attribute__((weak)) void OnPreStart(void)
{
  /* Do not implement your application functionality here. Instead copy this function
   * to your application's source file and exluce the __attribute__((weak)) part. That
   * way the version you implement in your application overrides this one.
   */
} /*** end of OnPreStart ***/


/************************************************************************************//**
** \brief     Default callback that gets called upon startup.
**
****************************************************************************************/
__attribute__((weak)) void OnStart(void)
{
  /* Do not implement your application functionality here. Instead copy this function
   * to your application's source file and exluce the __attribute__((weak)) part. That
   * way the version you implement in your application overrides this one.
   */
} /*** end of OnStart ***/


/************************************************************************************//**
** \brief     Default callback that gets called upon exit, before disconnecting from the
**            CAN network.
**
****************************************************************************************/
__attribute__((weak)) void OnPreStop(void)
{
  /* Do not implement your application functionality here. Instead copy this function
   * to your application's source file and exluce the __attribute__((weak)) part. That
   * way the version you implement in your application overrides this one.
   */
} /*** end of OnPreStop ***/


/************************************************************************************//**
** \brief     Default callback that gets called upon exit.
**
****************************************************************************************/
__attribute__((weak)) void OnStop(void)
{
  /* Do not implement your application functionality here. Instead copy this function
   * to your application's source file and exluce the __attribute__((weak)) part. That
   * way the version you implement in your application overrides this one.
   */
} /*** end of OnStop ***/


/************************************************************************************//**
** \brief     Default callback that gets called upon reception of a CAN message.
** \param     msg Pointer to the received CAN message.
**
****************************************************************************************/
__attribute__((weak)) void OnMessage(tCanMsg const * msg)
{
  /* Do not implement your application functionality here. Instead copy this function
   * to your application's source file and exluce the __attribute__((weak)) part. That
   * way the version you implement in your application overrides this one.
   */
} /*** end of OnMessage ***/


/************************************************************************************//**
** \brief     Default callback that gets called upon keyboard key pressed event.
** \param     key ASCII code of the pressed key.
**
****************************************************************************************/
__attribute__((weak)) void OnKey(char key)
{
  /* Do not implement your application functionality here. Instead copy this function
   * to your application's source file and exluce the __attribute__((weak)) part. That
   * way the version you implement in your application overrides this one.
   */
} /*** end of OnKey ***/


/*********************************** end of caplin.c ***********************************/
