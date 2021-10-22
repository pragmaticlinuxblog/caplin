/************************************************************************************//**
* \file         ex1.c
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
  printf("Example 1 - Ping Pong:\n");
  printf("\n");
  printf("* Echo all received CAN messages back with RX ID + 1\n");
  printf("------------------------------------------------------------\n");
} /*** end of OnStart ***/


/************************************************************************************//**
** \brief     Application callback that gets called upon reception of a CAN message.
** \param     msg Pointer to the received CAN message.
**
****************************************************************************************/
void OnMessage(tCanMsg const * msg)
{
  tCanMsg txMsg;

  /* Copy the received message (ping). */
  txMsg = *msg;
  /* Add 1 to the CAN identifier. */
  txMsg.id += 1;
  /* Transmit the message on the CAN bus (pong). */
  CanTransmit(&txMsg);
} /*** end of OnMessage ***/


/*********************************** end of ex1.c **************************************/
