/************************************************************************************//**
* \file         keys.h
* \brief        Input key detection driver header file.
*
****************************************************************************************/
#ifndef KEYS_H
#define KEYS_H

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************************
* Type definitions
****************************************************************************************/
/** \brief Function type for the key pressed event callback handler. */
typedef void (* tKeysEventCallback)(char key);


/****************************************************************************************
* Function prototypes
****************************************************************************************/
void KeysInit(tKeysEventCallback callbackFcn);
void KeysTerminate(void);


#ifdef __cplusplus
}
#endif

#endif /* KEYS_H */
/*********************************** end of keys.h *************************************/
