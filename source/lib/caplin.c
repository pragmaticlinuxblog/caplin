/************************************************************************************//**
* \file         caplin.c
* \brief        CAN application programming for Linux source file.
*
****************************************************************************************/

/****************************************************************************************
* Include files
****************************************************************************************/
#include <assert.h>                         /* for assertions                          */
#include <stdint.h>                         /* for standard integer types              */
#include <stdbool.h>                        /* for boolean type                        */
#include <stdio.h>                          /* for standard input/output functions     */
#include <stdlib.h>                         /* for standard library                    */
#include <stdatomic.h>                      /* Atomic operations                       */
#include <signal.h>                         /* Signal handling                         */
#include <string.h>                         /* for string library                      */
#include <getopt.h>                         /* Command line parsing                    */
#include <unistd.h>                         /* UNIX standard functions                 */
#include <net/if.h>                         /* Network interfaces                      */
#include <linux/if_arp.h>                   /* ARP definitions                         */
#include <linux/can.h>                      /* CAN kernel definitions                  */
#include <linux/can/raw.h>                  /* CAN raw sockets                         */
#include <sys/ioctl.h>                      /* I/O control operations                  */
#include <ifaddrs.h>                        /* Listing network interfaces.             */
#include "util.h"                           /* Utility functions                       */
#include "timer.h"                          /* Timer driver                            */
#include "keys.h"                           /* Input key detection driver              */
#include "can.h"                            /* CAN driver                              */


/****************************************************************************************
* Global data declarations
****************************************************************************************/
/** \brief Name of the SocketCAN device to use. Run 'ip addr | grep "can" for a list of
 *  all available CAN network interfaces and update this value to whichever CAN network
 *  interface you want to use.
 */
char canDevice[IFNAMSIZ] = "vcan0";


/****************************************************************************************
* Local data declarations
****************************************************************************************/
/** \brief Atomic boolean that is used to request a program exit. */
static atomic_bool appExitProgram;

/** \brief Boolean flag to determine if the help info should be displayed. */
static bool appArgHelp;


/****************************************************************************************
* External function prototypes
****************************************************************************************/
/* OnPreStart is called upon program startup, before connecting to the CAN network. */
extern void OnPreStart(void);
/* OnStart is called upon program startup, after connecting to the CAN network. */
extern void OnStart(void);
/* OnStop is called upon program exit, before disconnecting from the CAN network. */
extern void OnStop(void);
/* OnPostStop is called upon program exit, after disconnecting from the CAN network. */
extern void OnPostStop(void);
/* OnMessage is called each time a new CAN message was received. */
extern void OnMessage(tCanMsg const * msg);
/* OnKey is called each time a key was pressed on the keyboard. */
extern void OnKey(char key);


/****************************************************************************************
* Function prototypes
****************************************************************************************/
static void AppParseArguments(int argc, char *argv[]);
static void AppDisplayHelp(char const * appName);
static void AppFindFirstCanInterface(void);
static bool AppIsCanInterface(char const * name);
static void AppKeyPressedCallback(char key);
static void AppInterruptSignalHandler(int signum);


/************************************************************************************//**
** \brief     This is the program entry point.
** \param     argc Number of program arguments.
** \param     argv Array with program arguments.
** \return    Program return code. 0 for success, error code otherwise.
**
****************************************************************************************/
int main(int argc, char *argv[])
{
  bool canConnected = false;

  /* Initialize locals. */
  atomic_init(&appExitProgram, false);
  appArgHelp = false;

  /* Attempt to locate and use the first SocketCAN interface known on the system. */
  AppFindFirstCanInterface();
  /* Parse the command line arguments. This allows an override of the SocketCAN name. */
  AppParseArguments(argc, argv);

  /* Should program usage be displayed? */
  if (appArgHelp)
  {
    /* Display usage information. */
    AppDisplayHelp(argv[0]);
    /* Exit the program. */
    return EXIT_SUCCESS;
  }

  /* Initialize the timer driver. */
  TimerInit();
  /* Initialize the input key detection driver. */
  KeysInit(AppKeyPressedCallback);
  /* Initialization the CAN driver. */
  CanInit(OnMessage, NULL);

  /* Register interrupt signal handler for when CTRL+C was pressed. */
  signal(SIGINT,AppInterruptSignalHandler);
  /* Call the OnPreStart callback. */
  OnPreStart();
  /* Connect to the CAN bus. */
  canConnected = CanConnect(canDevice);

  /* Only run the actual CAN application if connected. */
  if (!canConnected)
  {
    /* Display usage information. */
    AppDisplayHelp(argv[0]);
    /* Display error message. */
    printf("ERROR: Could not connect to SocketCAN network interface \"%s\".\n", canDevice);
  }
  else
  {
    /* Call the OnStart callback. */
    OnStart();

    /* Enter the program loop until an exit is requested. */
    while (!atomic_load(&appExitProgram))
    {
      /* Noting to do here, because the user's CAN application is event driven. Just 
       * delay a little to not starve the CPU. 
      */
      UtilSleep(50 * 1000);
    }

    /* Call the OnStop callback. */
    OnStop();

    /* Disconnect from the CAN bus. */
    CanDisconnect();
  }

  /* Call the OnPostStop callback. */
  OnPostStop();

  /* Terminate the timer driver. */
  TimerTerminate();
  /* Terminate the CAN driver. */
  CanTerminate();
  /* Terminate the input key detection driver. */
  KeysTerminate();

  /* Exit the program. */
  return EXIT_SUCCESS;
} /*** end of main ***/


/************************************************************************************//**
** \brief     Parses the program's command line arguments.
** \param     argc Number of program arguments.
** \param     argv Array with program arguments.
**
****************************************************************************************/
static void AppParseArguments(int argc, char *argv[])
{
  int c;

  /* Process arguments one-by-one. */
  while (1) 
  {
    int option_index = 0;
    static struct option long_options[] = 
    {
      { "help", no_argument, NULL, 'h' },
      { NULL,   0,           NULL,  0  }
    };

    /* Get the next argument, */
    c = getopt_long(argc, argv, "-:h", long_options, &option_index);
    /* All done? */
    if (c == -1)
    {
      break;
    }

    /* Filter and process the argument. */
    switch (c) 
    {
      /* Regular option. Must be the preferred SocketCAN interface name. */
      case 1:
        /* Store the SocketCAN interface name. */
        strncpy(canDevice, optarg, sizeof(canDevice)/sizeof(canDevice[0]));
        break;

      /* Help requested. */
      case 'h':
        /* Set flag. */
        appArgHelp = true;
        break;

      default:
        break;
    }
  }
} /*** end of AppParseArguments ***/


/************************************************************************************//**
** \brief     Display program usage on the standard output.
** \param     appName Application name.
**
****************************************************************************************/
static void AppDisplayHelp(char const * appName)
{
  printf("Usage: %s [-h] [interface]\n", appName);
  printf("\n");
  printf("  Run the SocketCAN node application, using the INTERFACE SocketCAN\n");
  printf("  network interface.\n");
  printf("\n");
  printf("  The default INTERFACE is the first one found on the Linux system,\n");
  printf("  or \"vcan0\" if none are found.\n");
  printf("\n");
  printf("  Command 'ip addr | grep \"can\"' lists all available SocketCAN\n");
  printf("  network interfaces.\n");
  printf("\n");
  printf("  Press ESC or CTRL+C to exit.\n");
  printf("\n");
  printf("  Options:\n");
  printf("    -h, --help      Display this help information.\n");
  printf("\n");
} /*** end of AppDisplayHelp ***/


/************************************************************************************//**
** \brief     Interates over the list of all network interfaces known to the system, 
**            until it encounters the first one with "can" in the name. If found, this
**            one will be configured as the default SocketCAN interface to connect to.
**
****************************************************************************************/
static void AppFindFirstCanInterface(void)
{
  struct ifaddrs *ifaddr;

  /* Attempt to obtain access to the linked list with network interfaces. */
  if (getifaddrs(&ifaddr) == 0) 
  {
    /* Loop through linked list, while maintaining head pointer, needed to free the
     * list later on.
     */
    for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
      /* We are interested in the ifa_name element, so only process the node, when this
       * one is valid.
       */
      if (ifa->ifa_name != NULL)
      {
        /* Check if this network interface is actually a CAN interface. */
        if (AppIsCanInterface(ifa->ifa_name))
        {
          /* Store the SocketCAN interface name. */
          strncpy(canDevice, ifa->ifa_name, sizeof(canDevice)/sizeof(canDevice[0]));
          /* First SocketCAN device found and stored. No need to continue the loop. */
          break;
        }
      }
    }
    /* Free the list, now that we are done with it. */
    freeifaddrs(ifaddr);
  }
} /*** end of AppFindFirstCanInterface ***/


/************************************************************************************//**
** \brief     Determines if the specified network interface name is a CAN device.
** \param     name Network interface name. For example obtained by getifaddrs().
** \return    True is the specified network interface name is a CAN device.
**
****************************************************************************************/
static bool AppIsCanInterface(char const * name)
{
  bool result = false;
  struct ifreq ifr;
  int canSocket;

  /* Verify parameter. */
  assert(name != NULL);

  /* Only continue with valid parameter and acceptable length of the interface name. */
  if ( (name != NULL) && (strlen(name) < IFNAMSIZ) )
  {
    /* Create an ifreq structure for passing data in and out of ioctl. */
    strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    /* Get open socket descriptor */
    if ((canSocket = socket(PF_CAN, (int)SOCK_RAW, CAN_RAW)) != -1)
    {
      /* Obtain the hardware address information. */
      if (ioctl(canSocket, SIOCGIFHWADDR, &ifr) != -1)
      {
        /* Is this a CAN device? */
        if (ifr.ifr_hwaddr.sa_family == ARPHRD_CAN)
        {
          /* Update the result accordingly. */
          result = true;
        }
      }
      /* Close the socket, now that we are done with it. */
      close(canSocket);
    }
  }
 
  /* Give the result back to the caller. */
  return result;
} /*** end of AppIsCanInterface ***/


/************************************************************************************//**
** \brief     Application callback that gets called upon keyboard key pressed event.
** \param     key ASCII code of the pressed key.
**
****************************************************************************************/
static void AppKeyPressedCallback(char key)
{
  /* -------------------------- ESC key pressed? --------------------------------------*/
  if (key == 27)
  {
    /* Set request flag to exit the program when the ESC key was pressed. */
    atomic_store(&appExitProgram, true);
  }
  /* All other keys can be processed by the user's CAN application. */
  else
  {
    /* Call the OnKey callback. */
    OnKey(key);
  }
} /*** end of AppKeyPressedCallback ***/


/************************************************************************************//**
** \brief     Application callback that gets called when CTRL+C was pressed to quit the
**            program.
** \param     signum Signal number (not used)
**
****************************************************************************************/
static void AppInterruptSignalHandler(int signum)
{
  /* Set request flag to exit the program when the CTRL+C key combo was pressed. */
  atomic_store(&appExitProgram, true);
} /*** end of AppInterruptSignalHandler ***/


/************************************************************************************//**
** \brief     Default callback that gets called upon startup, before connecting to the
**            CAN network.
**
****************************************************************************************/
__attribute__((weak)) void OnPreStart(void)
{
  /* Do not implement your application functionality here. Instead copy this function
   * to your application's source file and exluce the __attribute__((weak)) part. That
   * way the version you implement in your application overrides this one.
   */
} /*** end of OnPreStart ***/


/************************************************************************************//**
** \brief     Default callback that gets called upon startup.
**
****************************************************************************************/
__attribute__((weak)) void OnStart(void)
{
  /* Do not implement your application functionality here. Instead copy this function
   * to your application's source file and exluce the __attribute__((weak)) part. That
   * way the version you implement in your application overrides this one.
   */
} /*** end of OnStart ***/


/************************************************************************************//**
** \brief     Default callback that gets called upon exit.
**
****************************************************************************************/
__attribute__((weak)) void OnStop(void)
{
  /* Do not implement your application functionality here. Instead copy this function
   * to your application's source file and exluce the __attribute__((weak)) part. That
   * way the version you implement in your application overrides this one.
   */
} /*** end of OnStop ***/


/************************************************************************************//**
** \brief     Default callback that gets called upon exit, after disconnecting from the
**            CAN network.
**
****************************************************************************************/
__attribute__((weak)) void OnPostStop(void)
{
  /* Do not implement your application functionality here. Instead copy this function
   * to your application's source file and exluce the __attribute__((weak)) part. That
   * way the version you implement in your application overrides this one.
   */
} /*** end of OnPostStop ***/


/************************************************************************************//**
** \brief     Default callback that gets called upon reception of a CAN message.
** \param     msg Pointer to the received CAN message.
**
****************************************************************************************/
__attribute__((weak)) void OnMessage(tCanMsg const * msg)
{
  /* Do not implement your application functionality here. Instead copy this function
   * to your application's source file and exluce the __attribute__((weak)) part. That
   * way the version you implement in your application overrides this one.
   */
} /*** end of OnMessage ***/


/************************************************************************************//**
** \brief     Default callback that gets called upon keyboard key pressed event.
** \param     key ASCII code of the pressed key.
**
****************************************************************************************/
__attribute__((weak)) void OnKey(char key)
{
  /* Do not implement your application functionality here. Instead copy this function
   * to your application's source file and exluce the __attribute__((weak)) part. That
   * way the version you implement in your application overrides this one.
   */
} /*** end of OnKey ***/


/*********************************** end of caplin.c ***********************************/
