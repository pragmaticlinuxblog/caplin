/************************************************************************************//**
* \file         ex3.c
* \brief        SocketCAN example application.
*
****************************************************************************************/

/****************************************************************************************
* Include files
****************************************************************************************/
#include "caplin.h"                         /* Caplin functionality                    */


/****************************************************************************************
* Function prototypes
****************************************************************************************/
void OnTimer500ms(void);


/****************************************************************************************
* Data declarations
****************************************************************************************/
tTimer timer500ms;


/************************************************************************************//**
** \brief     Application callback that gets called upon startup.
**
****************************************************************************************/
void OnStart(void)
{
  printf("------------------------------------------------------------\n");
  printf("Example 3 - Periodic CAN message transmission:\n");
  printf("\n");
  printf("* Press the 'e' key to start the periodic CAN message\n");
  printf("  transmission.\n");
  printf("* Press the 'd' key to stop it.\n");
  printf("* A 500 millisecond timer handles the transmission.\n");
  printf("* The CAN message has an 29-bit (ext) ID 3F1h and two data\n");
  printf("  bytes containing an incrementing and decrementing counter.\n");
  printf("------------------------------------------------------------\n");

  /* Create the timer and set its callback function. */
  timer500ms = TimerCreate(OnTimer500ms);
} /*** end of OnStart ***/


/************************************************************************************//**
** \brief     Application callback that gets called upon keyboard key pressed event.
** \param     key ASCII code of the pressed key.
**
****************************************************************************************/
void OnKey(char key)
{
  /* Was the 'e' key pressed to enable the periodic transmission? */
  if (key == 'e')
  {
    /* Start the timer with a 500 millisecond period. */
    TimerStart(timer500ms, 500);
  }

  /* Was the 'd' key pressed to disable the periodic transmission? */
  if (key == 'd')
  {
    /* Stop the timer. */
    TimerStop(timer500ms);
  }
} /*** end of OnKey ***/


/************************************************************************************//**
** \brief     Timer callback that gets called each time the timer's period elapsed.
**
****************************************************************************************/
void OnTimer500ms(void)
{
  /* Note the static keyword to 'remember' the state of local variable between function
   * calls.
   */
  static tCanMsg txMsg = 
  {
    .id = 0x3F1,             /* CAN identifier. */
    .ext = true,             /* 29-bit identifier. */
    .len = 2,                /* 2 data bytes. */
    .data = { 0x00, 0xFF }   /* Data byte values. */
  };

  /* Transmit the CAN message. */
  CanTransmit(&txMsg);
  /* Increment the value of the first data byte. */
  txMsg.data[0]++;
  /* Decrement the value of the second data byte. */
  txMsg.data[1]--;

  /* Restart the timer. */
  TimerRestart(timer500ms);
} /*** end of OnTimer500ms ***/


/*********************************** end of ex3.c **************************************/
