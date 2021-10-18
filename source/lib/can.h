/************************************************************************************//**
* \file         can.h
* \brief        Generic SocketCAN driver header file.
*
****************************************************************************************/
#ifndef CAN_H
#define CAN_H

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************************
* Macro definitions
****************************************************************************************/
/** \brief Maximum number of bytes in a CAN message. */
#define CAN_DATA_LEN_MAX     (8U)


/****************************************************************************************
* Type definitions
****************************************************************************************/
typedef struct
{
  /** \brief CAN message identifier. */
  uint32_t id;
  /** \brief True for a 29-bit CAN identifier, false for 11-bit. */
  bool     ext;
  /** \brief CAN message data length [0..(CAN_DATA_LEN_MAX-1)]. */
  uint8_t  len;
  /** \brief Array with the data bytes of the CAN message. */
  uint8_t  data[CAN_DATA_LEN_MAX];
  /** \brief Timestamp in microseconds. */
  uint64_t timestamp;
} tCanMsg;

/** \brief Function type for the message received callback handler. */
typedef void (* tCanReceivedCallback)(tCanMsg const * msg);

/** \brief Function type for the message transmitted callback handler. */
typedef void (* tCanTransmittedCallback)(tCanMsg const * msg);


/****************************************************************************************
* Function prototypes
****************************************************************************************/
void CanInit(tCanReceivedCallback rxCallbackFcn, tCanTransmittedCallback txCallbackFcn);
void CanTerminate(void);
bool CanConnect(char const * device);
void CanDisconnect(void);
bool CanTransmit(tCanMsg const * msg);
void CanPrintMessage(tCanMsg const * msg);


#ifdef __cplusplus
}
#endif

#endif /* CAN_H */
/*********************************** end of can.h **************************************/
