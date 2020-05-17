/***********************************************************************************************************************
 * virtualComPort.c
 * @brief:  Library to manage comPorts objects
 * @author: Xavier Aguirre Torres @ The microBoard Order
 * @date:   December 2019
 *
 **********************************************************************************************************************/
/* Includes ----------------------------------------------------------------------------------------------------------*/
#include <stdio.h>   //
#include <stdbool.h> // bool
#include <windows.h> // MAX_PATH, HANDLE, DCB, COMMTIMEOUTS, CreateFile(), DWORD
#include "main.h"
#include "virtualComPort.h"


/* Private objects/variables -----------------------------------------------------------------------------------------*/
DCB _dcbSerialParams = {0};     // DCB by default
COMMTIMEOUTS _timeouts = {0};   // COMMTIMEOUTS by default

/* Private functions declaration -------------------------------------------------------------------------------------*/
static int _setConnectionParameters( vcp_t* vcp );


/* Functions definition ----------------------------------------------------------------------------------------------*/
// PUBLIC FUNCTIONS ////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                    //
//   vcp_t     f_createVCP( int comPortNumber )                                                                       //
//   bool      f_openVCP( vcp_t vcp )                                                                                 //
//   bool      f_closeVCP( vcp_t vcp )                                                                                //
//   bool      f_tryOpenVCP( vcp_t _vcp, uint8_t maxNtries )                                                          //
//   bool      f_tryCloseVCP( vcp_t _vcp, uint8_t maxNtries )                                                         //
//                                                                                                                    //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/***********************************************************************************************************************
 * f_createVCP( .. )
 * @brief:  Function to create the COM port number to establish the communication with the relays boards
 * @param1: <int> portNum : Number of the highest probable COM port
 * @return: <vcp_t> The VirtualComPort object
 **********************************************************************************************************************/
vcp_t createVCP( const int portNum )
{
   vcp_t _VCP;                    // Object to return
   uint8_t tries = MAX_TRIES_TO_CREATE_VCP;

   // Loop to search for the device
   while( tries >= 0 )
   {
      // Set _VCP.name
      sprintf( _VCP.name, "\\\\.\\COM%d", portNum );
      // Set _VCP.handle
      _VCP.hSerial = CreateFile( _VCP.name,        // File Name
                                 GENERIC_WRITE,    // Access Mode
                                 0,                // Share Mode (Serial ports can't be shared)
                                 NULL,             // Security Attributes
                                 OPEN_EXISTING,    // Creation Disposition //OPEN_ALWAYS
                                 0,                // Flags and Attributes (Non Overlapped IO)
                                 NULL );           // Template File
      // If it was not possible to open the COM port try again
      if( _VCP.hSerial == INVALID_HANDLE_VALUE )
      {
         fprintf( stderr, "%s %s()::Error in opening serial port %s\n" , LOG_ERROR, __func__, _VCP.name );
         Sleep(50);
         tries--;
      }
      // Otherwise we've found it
      else
      {
         // Set _VCP object
         _VCP.number = portNum;
         _VCP.dcbSerialParams = _dcbSerialParams;
         _VCP.timeouts = _timeouts;
         _setConnectionParameters( &_VCP );
         fprintf( stdout, "%s %s()::Successfully VCP created in port: %s\n" , LOG_INFO, __func__, _VCP.name );
         break;
      }
   }
   if( tries < 0 )
   {
      fprintf( stderr, "%s %s()::Impossible to create a VCP in port %s. Ending program...\n" , LOG_ERROR, __func__, _VCP.name );
      exit(0); // Kill the program
   }
   // UART Connection parameters are ready. Close it by the time.
   //closeVCP( &_VCP );
   CloseHandle( _VCP.hSerial );
   // Return object
   return _VCP;
}
// END f_createVCP( .. ) ...


/***********************************************************************************************************************
 * f_openVCP( .. )
 * @brief:  Function to open the VCP
 * @param1: <vcp_t*> vcp: The Virtual Com Port we pretend to open
 * @return: <bool> TRUE if success FALSE if not
 **********************************************************************************************************************/
bool openVCP( vcp_t* vcp  )
{
   bool retValue = true;
   vcp->hSerial = CreateFile( vcp->name,        // File Name
                              GENERIC_WRITE,    // Access Mode
                              0,                // Share Mode (Serial ports can't be shared)
                              NULL,             // Security Attributes
                              OPEN_EXISTING,    // Creation Disposition //OPEN_ALWAYS
                              0,                // Flags and Attributes (Non Overlapped IO)
                              NULL );           // Template File

   if( vcp->hSerial == INVALID_HANDLE_VALUE )
   {
      //fprintf( stderr, "%s %s()::Unable to open port %s\n" , LOG_ERROR, __func__, vcp->name );
      retValue = false;
   }
   else
   {
      //fprintf( stdout, "%s %s()::Successfully opened %s\n" , LOG_INFO, __func__, vcp->name );
   }
   return retValue;
}
// END f_openVCP( .. ) ...


/***********************************************************************************************************************
 * f_closeVCP( .. )
 * @brief:  Function to close the VCP
 * @param1: <vcp_t*> vcp: The Virtual Com Port we pretend to open
 * @return: <bool> TRUE if success FALSE if not
 **********************************************************************************************************************/
bool closeVCP( const vcp_t* vcp )
{
   bool retValue = true;
   FlushFileBuffers( vcp->hSerial );
   if( !CloseHandle( vcp->hSerial ) )
   {
      //fprintf( stderr, "%s %s()::Unable to close port %s\n", LOG_ERROR, __func__, vcp->name );
      retValue = false;
   }
   else
   {
      //fprintf( stdout, "%s %s()::Successfully closed %s\n" , LOG_INFO, __func__, vcp->name );
   }
   return retValue;
}
// END f_closeVCP( .. ) ...


/***********************************************************************************************************************
 * f_sendFrameVCP( .. )
 * @brief:  Function to send a frame to the
 * @param1: <const vcp_t*> vcp: the Virtual COM port used
 * @param2: <const char *> message: The message to send
 * @param3: <size_t> frameLength: The message length
 * @return: <bool> TRUE if success FALSE if not
 **********************************************************************************************************************/
bool sendFrameVCP( const vcp_t* vcp, const char * message, size_t frameLength )
{
   DWORD bytesWritten = 0,
         totalBytesWritten = 0;

   while( totalBytesWritten < frameLength )
   {
      if( !WriteFile( vcp->hSerial, message + totalBytesWritten, frameLength - totalBytesWritten, &bytesWritten, NULL ) )
      {
         fprintf( stderr, "%s Error writing text to %s\n", LOG_ERROR, vcp->name );
      }
      else
      {
         totalBytesWritten += bytesWritten;
      }
   }
   if( bytesWritten != frameLength )
   {
      fprintf( stderr, "%s Incomplete message written\n", LOG_ERROR );
      return false;
   }
   return true;
}
// END f_sendFrameVCP( .. ) ...


/**********************************************************************************************************************
 * f_tryOpenVCP( .. )
 * @brief: Function to stablish a maximum number of tries to Open the COM Port
 * @param1 <vcp_t*> _vcp : the Virtual Com Port
 * @param2 <uint8_t> maxNtries : Maximum number of tries
 * @return: <bool> TRUE if succeed FALSE if not
 *********************************************************************************************************************/
bool tryOpenVCP( vcp_t* _vcp, uint8_t maxNtries )
{
   uint8_t triesToOpen = 0;

   while( !openVCP( _vcp ) )
   {
      fprintf( stderr, "%s %s()::Try %d: Unable to open port %s\n" , LOG_ERROR, __func__, triesToOpen, _vcp->name );
      Sleep( 50 );
      triesToOpen++;
      if( triesToOpen >= maxNtries )
      {
         return false;
      }
   }
   return true;
}
// END f_tryOpenVCP( .. ) ...


/**********************************************************************************************************************
 * tryCloseVCP( .. )
 * @brief: Function to stablish a maximum number of tries to Close the COM Port
 * @param1 <vcp_t*> _vcp : the Virtual Com Port
 * @param2 <uint8_t> maxNtries : Maximum number of tries
 * @return: <bool> TRUE if succeed FALSE if not
 *********************************************************************************************************************/
bool tryCloseVCP( const vcp_t* _vcp, uint8_t maxNtries )
{
   uint8_t triesToClose = 0;

   while( !closeVCP( _vcp ) )
   {
      fprintf( stderr, "%s %s()::Try %d: Unable to close port %s\n" , LOG_ERROR, __func__, triesToClose, _vcp->name );
      Sleep( 50 );
      triesToClose++;
      if( triesToClose >= maxNtries )
      {
         return false;
      }
   }
   return true;
}
// END f_tryCloseVCP( .. ) ...



// PRIVATE FUNCTIONS ///////////////////////////////////////////////////////////////////////////////////////////////////
/***********************************************************************************************************************
 * f_setConnectionParameters( .. )
 * @brief:  Function to setUp the UART connection
 * @param1: <vcp_t *> vcp: The Virtual COM port to set up connection
 * @return: <int> 0 if Success 1 if not
 **********************************************************************************************************************/
static int _setConnectionParameters( vcp_t* vcp )
{
   vcp->dcbSerialParams.DCBlength = sizeof( vcp->dcbSerialParams );

   if( GetCommState( vcp->hSerial, &( vcp->dcbSerialParams ) ) == 0 )
   {
     fprintf( stderr, "%s Error getting device state\n", LOG_ERROR );
     CloseHandle( vcp->hSerial );
     return 1;
   }

   vcp->dcbSerialParams.BaudRate = BAUD_RATE_DEFAULT;
   vcp->dcbSerialParams.ByteSize = 8;
   vcp->dcbSerialParams.StopBits = ONESTOPBIT;
   vcp->dcbSerialParams.Parity = NOPARITY;
   if( SetCommState( vcp->hSerial, &( vcp->dcbSerialParams ) ) == 0 )
   {
     fprintf( stderr, "%s Error setting device parameters\n", LOG_ERROR );
     CloseHandle( vcp->hSerial );
     return 1;
   }
   // Set COM port timeout settings
   vcp->timeouts.ReadIntervalTimeout = 50;
   vcp->timeouts.ReadTotalTimeoutConstant = 50;
   vcp->timeouts.ReadTotalTimeoutMultiplier = 10;
   vcp->timeouts.WriteTotalTimeoutConstant = 50;
   vcp->timeouts.WriteTotalTimeoutMultiplier = 10;
   if( SetCommTimeouts( vcp->hSerial, &( vcp->timeouts ) ) == 0 )
   {
     fprintf( stderr, "%s Error setting timeouts\n", LOG_ERROR );
     closeVCP( vcp );
     return 1;
   }
   return 0;
}
// END f_setConnectionParameters( .. ) ...
