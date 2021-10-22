/************************************************************************************//**
* \file         ex4.c
* \brief        SocketCAN example application.
*
****************************************************************************************/

/****************************************************************************************
* Include files
****************************************************************************************/
#include "caplin.h"                         /* Caplin functionality                    */


/************************************************************************************//**
** \brief     Application callback that gets called upon startup.
**
****************************************************************************************/
void OnStart(void)
{
  printf("------------------------------------------------------------\n");
  printf("Example 4 - CAN message logger:\n");
  printf("\n");
  printf("* Displays all received CAN messages on the standard output.\n");
  printf("------------------------------------------------------------\n");
} /*** end of OnStart ***/


/************************************************************************************//**
** \brief     Application callback that gets called upon reception of a CAN message.
** \param     msg Pointer to the received CAN message.
**
****************************************************************************************/
void OnMessage(tCanMsg const * msg)
{
  /* Display the CAN message in a formatted manner on the standard output. */
  CanPrintMessage(msg);
} /*** end of OnMessage ***/


/*********************************** end of ex4.c **************************************/
