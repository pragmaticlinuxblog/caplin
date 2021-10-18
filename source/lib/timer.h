/************************************************************************************//**
* \file         timer.h
* \brief        Generic timer driver header file.
*
****************************************************************************************/
#ifndef TIMER_H
#define TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************************
* Type definitions
****************************************************************************************/
/** \brief Timer handle type. */
typedef void * tTimer;

/** \brief Function type for the timer event callback handler. */
typedef void (* tTimerEventCallback)(void);


/****************************************************************************************
* Function prototypes
****************************************************************************************/
void   TimerInit(void);
void   TimerTerminate(void);
tTimer TimerCreate(tTimerEventCallback callbackFcn);
void   TimerDelete(tTimer timer);
void   TimerStart(tTimer timer, uint32_t period);
void   TimerRestart(tTimer timer);
void   TimerStop(tTimer timer);


#ifdef __cplusplus
}
#endif

#endif /* TIMER_H */
/*********************************** end of timer.h ************************************/
