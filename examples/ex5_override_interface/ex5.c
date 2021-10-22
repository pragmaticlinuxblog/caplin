/************************************************************************************//**
* \file         ex5.c
* \brief        SocketCAN example application.
*
****************************************************************************************/

/****************************************************************************************
* Include files
****************************************************************************************/
#include "caplin.h"                         /* Caplin functionality                    */


/************************************************************************************//**
** \brief     Application callback that gets called upon startup, before connecting to
**            the CAN network.
**
****************************************************************************************/
void OnPreStart(void)
{
  /* By default, the application connects to the first CAN network interface found on
   * the system. To use another one, you can specify its name as a command-line argument.
   * To programmatically override both these selections, you can store the name of the
   * CAN network interface in variable 'canDevice', as shown here:
   */
  strcpy(canDevice, "can0");
} /*** end of OnPreStart ***/


/************************************************************************************//**
** \brief     Application callback that gets called upon startup.
**
****************************************************************************************/
void OnStart(void)
{
  printf("------------------------------------------------------------\n");
  printf("Example 5 - CAN network interface override:\n");
  printf("\n");
  printf("* Programmatically overrides the CAN network interface.\n");
  printf("* It forces the program to always use 'can0'.\n");
  printf("------------------------------------------------------------\n");

  /* Display the name of the CAN network interface that we are connected to. */
  printf("Currently connected to CAN network interface: %s\n", canDevice);
} /*** end of OnStart ***/


/*********************************** end of ex5.c **************************************/
