/************************************************************************************//**
* \file         can.c
* \brief        Generic SocketCAN driver source file.
*
****************************************************************************************/

/****************************************************************************************
* Include files
****************************************************************************************/
#include <assert.h>                         /* for assertions                          */
#include <stdint.h>                         /* for standard integer types              */
#include <stddef.h>                         /* for NULL declaration                    */
#include <stdbool.h>                        /* for boolean type                        */
#include <stdio.h>                          /* for standard input/output functions     */
#include <string.h>                         /* for string library                      */
#include <stdlib.h>                         /* for standard library                    */
#include <fcntl.h>                          /* File control operations                 */
#include <unistd.h>                         /* UNIX standard functions                 */
#include <net/if.h>                         /* network interfaces                      */
#include <linux/can.h>                      /* CAN kernel definitions                  */
#include <linux/can/raw.h>                  /* CAN raw sockets                         */
#include <sys/ioctl.h>                      /* I/O control operations                  */
#include <threads.h>                        /* Multithreading                          */
#include <stdatomic.h>                      /* Atomic operations                       */
#include "util.h"                           /* Utility functions                       */
#include "can.h"                            /* CAN driver                              */


/****************************************************************************************
* Macro definitions
****************************************************************************************/
/** \brief Value of an invalid socket. */
#define CAN_INVALID_SOCKET             (-1)


/****************************************************************************************
* Local data declarations
****************************************************************************************/
/** \brief Function pointer for the message received callback handler. Volatile because
 *  it is shared with the event thread.
 */
static volatile tCanReceivedCallback canReceivedCallback;

/** \brief Function pointer for the message transmitted callback handler. Volatile
 *  because it is shared with the event thread.
 */
static volatile tCanTransmittedCallback canTransmittedCallback;

/** \brief CAN raw socket. Volatile because it is shared with the event thread. No need
 *  to make it atomic, because its value is only written before the event thread starts.
 */
static volatile int32_t canSocket;

/** \brief Mutex for mutual exlusive access to the canSocket. */
static mtx_t canSocketMutex;

/** \brief Identifier of the CAN event thread. */
static thrd_t canEventThreadId;

/** \brief Boolean flag that indicates if the CAN event thread is running or not. */
static bool canEventThreadRunning;

/** \brief Atomic boolean that is used to inform the event thread to stop running. */
static atomic_bool canStopEventThread;

/** \brief System time at which this CAN driver connected to the CAN bus. Volatile
 *  because it is shared with the event thread.
 */
static volatile uint64_t canStartTime;

/** \brief Boolean flag to keep track of the CAN connection state. Volatile because
 *  it is used in CanTransmit, which could be called from the reception callback.
 */
static volatile bool canConnected;


/****************************************************************************************
* Function prototypes
****************************************************************************************/
static int CanEventThread(void * param);


/************************************************************************************//**
** \brief     Initializes the CAN driver and sets the callback function to call, each
**            time a CAN message was received. Think of it as the constructor, if this
**            driver was a C++ class.
** \param     rxCallbackFnc CAN message received callback function pointer.
** \param     txCallbackFnc CAN message transmitted callback function pointer.
**
****************************************************************************************/
void CanInit(tCanReceivedCallback rxCallbackFcn, tCanTransmittedCallback txCallbackFcn)
{
  /* Initialize locals. */
  canReceivedCallback = NULL;
  canTransmittedCallback = NULL;
  canSocket = CAN_INVALID_SOCKET;
  canEventThreadId = 0;
  canEventThreadRunning = false;
  atomic_init(&canStopEventThread, false);
  canStartTime = 0;
  canConnected = false;

  /* Initialize the mutex. */
  if (mtx_init(&canSocketMutex, mtx_plain) != thrd_success)
  {
    assert(false);
  }

  /* Set the callback handlers. Note that it is okay to specify NULL for these
   * parameters, in case you have no need for the event handler.
   */
  canReceivedCallback = rxCallbackFcn;
  canTransmittedCallback = txCallbackFcn;
} /*** end of CanInit ***/


/************************************************************************************//**
** \brief     Terminates the CAN driver. Think of it as the destructor if this driver was
**            a C++ class.
**
****************************************************************************************/
void CanTerminate(void)
{
  /* Disconnect from the CAN bus. */
  CanDisconnect();

  /* Destroy the mutex. */
  mtx_destroy(&canSocketMutex);

  /* Reset locals that are not yet reset by CanDisconnect. */
  canTransmittedCallback = NULL;
  canReceivedCallback = NULL;
} /*** end of CanTerminate ***/


/************************************************************************************//**
** \brief     Connects to the specified SocketCAN device. Note that you can run command
**            "ip addr" in the terminal to determine the SocketCAN device name known to
**            your Linux system.
** \param     device Null terminated string with the SocketCAN device name, e.g. "can0".
** \return    True if successfully connected to the SocketCAN device, false otherwise.
**
****************************************************************************************/
bool CanConnect(char const * device)
{
  bool result = false;
  struct sockaddr_can addr;
  struct ifreq ifr;
  int32_t flags;

  /* Verify parameter. */
  assert(device != NULL);

  /* Only continue with valid parameter. */
  if (device != NULL)
  {
    /* Set positive result at this point and negate upon error detected. */
    result = true;

    /* Make sure we are in the disconnected state. */
    CanDisconnect();

    /* Store the current time to be able to have timestamps relative to when the CAN
     * device was connected.
    */
   canStartTime = UtilSystemTime();

    /* Create an ifreq structure for passing data in and out of ioctl. */
    strncpy(ifr.ifr_name, device, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    /* Get open socket descriptor */
    if ((canSocket = socket(PF_CAN, (int)SOCK_RAW, CAN_RAW)) < 0)
    {
      result = false;
    }

    if (result)
    {
      /* Obtain interface index. */
      if (ioctl(canSocket, SIOCGIFINDEX, &ifr) < 0)
      {
        close(canSocket);
        result = false;
      }
    }

    if (result)
    {
      /* Configure socket to work in non-blocking mode. */
      flags = fcntl(canSocket, F_GETFL, 0);
      if (flags == -1)
      {
        flags = 0;
      }
      if (fcntl(canSocket, F_SETFL, flags | O_NONBLOCK) == -1)
      {
        close(canSocket);
        result = false;
      }
    }

    if (result)
    {
      /* Set the address info. */
      addr.can_family = AF_CAN;
      addr.can_ifindex = ifr.ifr_ifindex;

      /* Bind the socket. */
      if (bind(canSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0)
      {
        close(canSocket);
        result = false;
      }
    }

    if (result)
    {
      /* Start the event thread. */
      if (thrd_create(&canEventThreadId, (thrd_start_t)CanEventThread, NULL) 
          != thrd_success)
      {
        close(canSocket);
        result = false;
      }
      else
      {
        /* Set flag. */
        canEventThreadRunning = true;
      }
    }
  }

  /* Update the connection state flag. */
  canConnected = result;

  /* Give the result back to the caller. */
  return result;
} /*** end of CanConnect ***/


/************************************************************************************//**
** \brief     Disconnects to the SocketCAN device.
**
****************************************************************************************/
void CanDisconnect(void)
{
  /* Reset the connection state flag. */
  canConnected = false;

  /* Stop the reception thread. */
  if (canEventThreadRunning)
  {
    /* Set atomic boolean flag to request the thread to stop. */
    atomic_store(&canStopEventThread, true);
    /* Wait until the thread terminated. */
    thrd_join(canEventThreadId, NULL);
  }

  /* Close the socket. */
  if (canSocket != CAN_INVALID_SOCKET)
  {
    close(canSocket);
  }

  /* Reset locals. */
  canStartTime = 0;
  atomic_init(&canStopEventThread, false);
  canEventThreadRunning = false;
  canEventThreadId = 0;
  canSocket = CAN_INVALID_SOCKET;
} /*** end of CanDisconnect ***/


/************************************************************************************//**
** \brief     Submits a CAN message for transmission.
** \param     msg Pointer to the CAN message to transmit.
** \return    True if the message could be submitted for transmission, false otherwise.
**
****************************************************************************************/
bool CanTransmit(tCanMsg const * msg)
{
  bool result = false;
  struct can_frame canTxFrame;
  tCanMsg txMsg;

  /* Verify parameter. */
  assert(msg != NULL);

  /* Only continue with valid parameter and when connected. */
  if ( (msg != NULL) && (canConnected) )
  {
    /* Copy the message so we can set the timestamp later on. */
    txMsg = *msg;

    /* Construct the message frame. */
    canTxFrame.can_id = msg->id;
    if (msg->ext)
    {
      canTxFrame.can_id |= CAN_EFF_FLAG;
    }
    canTxFrame.can_dlc = ((msg->len <= CAN_DATA_LEN_MAX) ? msg->len : CAN_DATA_LEN_MAX);
    for (uint8_t idx = 0; idx < canTxFrame.can_dlc; idx++)
    {
      canTxFrame.data[idx] = msg->data[idx];
    }

    /* Submit the message for transmission. */
    mtx_lock(&canSocketMutex);   
    /* Set the timestamp. */
    txMsg.timestamp = UtilSystemTime() - canStartTime;
    if (write(canSocket, &canTxFrame, sizeof(struct can_frame)) == 
        (ssize_t)sizeof(struct can_frame))
    {
      /* Message successfully submitted for transmission. Update the result accordingly. */
      result = true;
    }
    mtx_unlock(&canSocketMutex);
  }

  /* Call message transmitted callback. */
  if (result)
  {
    if (canTransmittedCallback != NULL)
    {
      canTransmittedCallback(&txMsg);
    }
  }
  
  /* Give the result back to the caller. */
  return result;
} /*** end of CanTransmit ***/


/************************************************************************************//**
** \brief     Prints the CAN message in a human readable format on the standard output.
** \param     msg Pointer to the CAN message to print.
**
****************************************************************************************/
void CanPrintMessage(tCanMsg const * msg)
{
  /* Print timestamp. */
  printf("(%.6f)", (float)msg->timestamp/(1000 * 1000));
  /* Print identifier. */
  printf(" %x", msg->id);
  msg->ext ? printf("x") : printf(" ");
  /* Print payload length. */
  printf(" [%d]", msg->len);
  /* Print data bytes. */
  for (uint8_t idx = 0; idx < msg->len; idx++)
  {
    printf(" %02x", msg->data[idx]);
  }
  /* Add line ending. */
  printf("\n");
} /*** end of CanPrintMessage ***/


/************************************************************************************//**
** \brief     Event thread that handles the asynchronous reception of data from the CAN
**            interface.
** \param     arg Pointer to thread parameters.
** \return    Thread return value.
**
****************************************************************************************/
static int CanEventThread(void * param)
{
  struct can_frame canRxFrame;
  tCanMsg rxMsg;
  bool msgReceived;

  /* Enter the thread's loop and run it, until a stop is requested. */
  while (!atomic_load(&canStopEventThread))
  {
    /* Empty out the CAN event queue. */
    do
    {
      /* Attempt to get the next CAN event from the queue. */
      msgReceived = false;
      mtx_lock(&canSocketMutex);   
      if (read(canSocket, &canRxFrame, sizeof(struct can_frame)) == 
           (ssize_t)sizeof(struct can_frame))
      {
        msgReceived = true;        
      }
      mtx_unlock(&canSocketMutex);

      /* Only process the message, if one was actually received. */
      if (msgReceived)
      {
        /* Set the message's timestamp. */
        rxMsg.timestamp = UtilSystemTime() - canStartTime;

        /* Ignore remote frames and error information. */
        if (!(canRxFrame.can_id & (CAN_RTR_FLAG | CAN_ERR_FLAG)))
        {
          /* Copy the CAN message. */
          if (canRxFrame.can_id & CAN_EFF_FLAG)
          {
            rxMsg.ext = true;
          }
          else
          {
            rxMsg.ext = false;
          }
          rxMsg.id = canRxFrame.can_id & ~CAN_EFF_FLAG;
          rxMsg.len = canRxFrame.can_dlc;
          for (uint8_t idx = 0; idx < canRxFrame.can_dlc; idx++)
          {
            rxMsg.data[idx] = canRxFrame.data[idx];
          }

          /* Call message reception callback. */
          if (canReceivedCallback != NULL)
          {
            canReceivedCallback(&rxMsg);
          }
        }
      }
    }
    while (msgReceived);

    /* Sleep for 500us to not starve the CPU. */
    UtilSleep(500);
  }

  /* Shut down the thread. */
  thrd_exit(EXIT_SUCCESS);
} /*** end of CanEventThread ***/


/*********************************** end of can.c **************************************/
