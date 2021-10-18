/************************************************************************************//**
* \file         caplin.h
* \brief        CAN application programming for Linux header file.
*
****************************************************************************************/
#ifndef CAPLIN_H
#define CAPLIN_H

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************************
* Include files
****************************************************************************************/
#include <assert.h>                         /* for assertions                          */
#include <stdint.h>                         /* for standard integer types              */
#include <stddef.h>                         /* for NULL declaration                    */
#include <stdbool.h>                        /* for boolean type                        */
#include <stdio.h>                          /* for standard input/output functions     */
#include <stdlib.h>                         /* for standard library                    */
#include "util.h"                           /* Utility functions                       */
#include "can.h"                            /* CAN driver                              */
#include "timer.h"                          /* Timer driver                            */
#include "keys.h"                           /* Input key detection driver              */


#ifdef __cplusplus
}
#endif

#endif /* CAPLIN_H */
/*********************************** end of caplin.h ***********************************/
