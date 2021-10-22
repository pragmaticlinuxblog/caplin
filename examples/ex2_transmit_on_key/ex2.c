/************************************************************************************//**
* \file         ex2.c
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
  printf("Example 2 - Transmit CAN message on key press:\n");
  printf("\n");
  printf("* Transmit a CAN message with ID 201h each time the 't' key\n");
  printf("  is pressed on the keyboard.\n");
  printf("* The first data byte of the CAN message contains an\n");
  printf("  incrementing counter.\n");
  printf("------------------------------------------------------------\n");
} /*** end of OnStart ***/


/************************************************************************************//**
** \brief     Application callback that gets called upon keyboard key pressed event.
** \param     key ASCII code of the pressed key.
**
****************************************************************************************/
void OnKey(char key)
{
  /* Note the static keyword to 'remember' the state of local variable between function
   * calls.
   */
  static tCanMsg txMsg = 
  {
    .id = 0x201,    /* CAN identifier. */
    .ext = false,   /* 11-bit identifier. */
    .len = 1,       /* 1 data byte. */
    .data = { 0 }   /* Data byte value(s). */
  };

  /* Was the 't' key pressed? */
  if (key == 't')
  {
    /* Transmit the CAN message. */
    CanTransmit(&txMsg);
    /* Increment the value of the first data bytes. */
    txMsg.data[0]++;
  }
} /*** end of OnKey ***/


/*********************************** end of ex2.c **************************************/
