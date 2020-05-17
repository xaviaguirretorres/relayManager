/**********************************************************************************************************************
 * virtualComPort.h
 * @brief:  Library to manage comPorts objects
 * @author: Xavier Aguirre Torres @ The microBoard Order
 * @date:   December 2019
 *
 *********************************************************************************************************************/
#ifndef VIRTUAL_COM_PORT_H_INCLUDED
#define VIRTUAL_COM_PORT_H_INCLUDED

/* Includes ----------------------------------------------------------------------------------------------------------*/
#include <windows.h> // HANDLE
#include <stdbool.h> // bool
#include <stdint.h>  // uint8_t


/* Public/Global defines ---------------------------------------------------------------------------------------------*/
// Baud rates predefined macros
#define BAUD_RATE_110            110
#define BAUD_RATE_300            300
#define BAUD_RATE_600            600
#define BAUD_RATE_1200           1200
#define BAUD_RATE_2400           2400
#define BAUD_RATE_4800           4800
#define BAUD_RATE_9600           9600
#define BAUD_RATE_14400          14400
#define BAUD_RATE_19200          19200
#define BAUD_RATE_38400          38400
#define BAUD_RATE_57600          57600
#define BAUD_RATE_115200         115200
#define BAUD_RATE_128000         128000
#define BAUD_RATE_256000         256000

#define BAUD_RATE_DEFAULT        BAUD_RATE_9600

#define MAX_TRIES_TO_CREATE_VCP  50

/* Public typedefs ---------------------------------------------------------------------------------------------------*/
// Virtual Port COM Class
typedef struct virtualComPort_type vcp_t;
struct virtualComPort_type
{
   HANDLE       hSerial;            // Object's handler
   int          number;             // Port number
   char         name[MAX_PATH];     // Port name
   DCB          dcbSerialParams;    // Connection parameters
   COMMTIMEOUTS timeouts;
};



/* Public functions declaration --------------------------------------------------------------------------------------*/
vcp_t createVCP( const int /* num */ );
bool  openVCP( vcp_t* /* vcp */ );
bool  closeVCP( const vcp_t* /* vcp */ );
bool  sendFrameVCP( const vcp_t* /* vcp */, const char * /* message */, const size_t /* frameLength */ );

bool  tryOpenVCP( vcp_t* /* _vcp */, uint8_t /* maxNtries */ );
bool  tryCloseVCP( const vcp_t* /* _vcp */, uint8_t /* maxNtries */ );

#endif // VIRTUAL_COM_PORT_H_INCLUDED
