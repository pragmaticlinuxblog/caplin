/************************************************************************************//**
* \file         canapp.c
* \brief        SocketCAN application C template.
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
  printf(">>> CANAPP SocketCAN node application <<<<\n");
  printf("- 'ESC'-key quits the application\n\n");
} /*** end of OnStart ***/


/************************************************************************************//**
** \brief     Application callback that gets called upon exit.
**
****************************************************************************************/
void OnStop(void)
{

} /*** end of OnStop ***/


/************************************************************************************//**
** \brief     Application callback that gets called upon reception of a CAN message.
** \param     msg Pointer to the received CAN message.
**
****************************************************************************************/
void OnMessage(tCanMsg const * msg)
{

} /*** end of OnMessage ***/


/************************************************************************************//**
** \brief     Application callback that gets called upon keyboard key pressed event.
** \param     key ASCII code of the pressed key.
**
****************************************************************************************/
void OnKey(char key)
{

} /*** end of OnKey ***/


/*********************************** end of caplin.c ***********************************/
