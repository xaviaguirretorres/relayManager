/**********************************************************************************************************************
 * RelayManager
 * @brief:   This is a short program to interact with the KMTronic Relays board through Serial Virtual Port Com
             connection.
 * @author:  Xavier Aguirre Torres @ The MicroBoard Order
 * @version: V1.0
 * @date:    December 2019
 *
 *********************************************************************************************************************/
/* Includes ----------------------------------------------------------------------------------------------------------*/
#include <windows.h> // MAX_PATH, HANDLE, DCB, COMMTIMEOUTS, CreateFile()
#include <stdio.h>   // fprintf(), stderr
#include <string.h>  // strcmp(), strcpy(), strlen()
#include <stdlib.h>  // atoi(), strtol()
#include <stdint.h>  // uint8_t
#include <ctype.h>   //
#include <time.h>    //

#include "main.h"
#include "virtualComPort.h"



/* Private defines ---------------------------------------------------------------------------------------------------*/
#define _FRAME_SOH            0xff  // First byte of the frame
#define _FRAME_RELAY_ON       0x01  // Value to set a relay ON
#define _FRAME_RELAY_OFF      0x00  // Value to set a relay OFFLINE_STATUS_INCOMPLETE
#define _FRAME_LENGTH         3     // Length of the frame

#define _SLEEP_TIME           5
#define _MAX_OPEN_VCP_TRIES   50    // Max number of retries to open the COM port in case it fails
#define _MAX_CLOSE_VCP_TRIES  50    // Max number of retries to close the COM port in case it fails

#define _MAX_RELAY_RANGE      7



/* Private variables -------------------------------------------------------------------------------------------------*/
// Virtual COM port settings
static int  _baudrate         = BAUD_RATE_DEFAULT; // Variable to set the baudrate of the UART connection
static int  _comPortNumber    = COM_PORT_DEFAULT;  // Variable to set the COM port of the UART connection

// Relay settings
static uint8_t  _relayBegin   = 0;                 // The first relay in a range of relays
static uint8_t  _relayEnd     = 0;                 // The last relay in a range of relays
static uint8_t  _numOfRelays  = 0;                 // Number of relays affected into the query. MAX 120!!!

// State & time settings
static uint16_t _openTime     = 0;                 // Time the relay must be open
static uint8_t  _impulses     = 1;                 // Number of impulses to give
static char     _relayState[_FRAME_LENGTH+1];      // Only 2 states valid "on" or "off"

// Main arguments flags
static bool _singleRelayFlag = false;              // When true means a single relay was choosen (Ex. 4)
static bool _rangeRelayFlag  = false;              // When true means a range of relays was choosem (Ex. 2:8)
static bool _multiRelayFlag  = false;              // When true means a group of different relays was choosen (Ex. 2,6,8)
static bool _stateFlag       = false;              // When true '-state' argument was called
static bool _openTimeFlag    = false;              // When true '-openTime' argument was called
static bool _impulsesFlag    = false;              // When true '-impulses' argument was called

// Dynamic allocate
uint8_t* _relays        = NULL; // Pointer to set dynamic array depending on the number of relays affected
char*    _rs485OpenMsg  = NULL; // Pointer to allocate dynamic array to send the instructions to open relays
char*    _rs485CloseMsg = NULL; // Pointer to allocate dynamic array to send the instructions to open relays


/* Private typedefs --------------------------------------------------------------------------------------------------*/
typedef enum eRelayModality_type {
   _errorRelay  = -1,
   _singleRelay = 1,
   _rangeRelays = 2,
   _groupRelays = 3
} relayModality_t;


/* Private functions declaration -------------------------------------------------------------------------------------*/
static int parseArgs( int argc, char *argv[] );
static relayModality_t relayModality( char* relayArgument, uint8_t* numberOfRelays );
static void _fillRelaysGroup( char* relayArgument, uint8_t* array, const uint8_t size );


/* Main function -----------------------------------------------------------------------------------------------------*/
int main( int argc, char *argv[] )
{
   // Write header
   fprintf( stdout, RELAYS_MANAGER_HEADER_MSG );


   // Set frames for open/close messages
   char openRelayMess[_FRAME_LENGTH+1];            // Create an array to hold a single openRelay message  [0xFF,0xXX,0x01]
   memset( openRelayMess, 0, _FRAME_LENGTH + 1 );  // Fill it with 0 values
   char closeRelayMess[_FRAME_LENGTH+1];           // Create an array to hold a single closeRelay message [0xFF,0xXX,0x00]
   memset( closeRelayMess, 0, _FRAME_LENGTH + 1 ); // Fill it with 0 values

   // Parse command line arguments
   if( parseArgs( argc, argv ) < 4 )
   {
      if( argc == 1 )
      {
         return -1;
      }
      fprintf( stderr, "%s %s()::Number of parameters can not be less than 4.\r\n", LOG_ERROR, __func__ );
      fprintf( stdout, "%s %s()::Closing %s.\r\n", LOG_INFO, __func__, __FILE__ );
      return -1;
   }

   // Create a pointer and allocate dynamic memory for the Virtual Com Port object
   vcp_t* vcp;
   vcp = (vcp_t*)malloc( sizeof( vcp_t ) );

   fprintf( stdout, "%s %s()::Creating VCP...\n" , LOG_INFO, __func__ );
   // Find and set the Virtual COM Port for Serial communication
   *vcp = createVCP( _comPortNumber );

   // Build Open/Close messages and set size
   sprintf( openRelayMess, "%c%c%c", 0xff, 0x00, _FRAME_RELAY_ON );
   sprintf( closeRelayMess, "%c%c%c", 0xff, 0x00, _FRAME_RELAY_OFF );


   if( _openTimeFlag || ( _stateFlag && ( strcmp( _relayState, "on" ) == 0 ) ) )
   {
      _rs485OpenMsg = malloc( sizeof(char) * ( _FRAME_LENGTH * _numOfRelays )  );
      memset( _rs485OpenMsg, 0, _FRAME_LENGTH * _numOfRelays );
      fprintf( stdout, "%s %s()::OpenRelaysMessage: [ " , LOG_INFO, __func__ );
      for( uint8_t i = 0; i < _numOfRelays; i++ )
      {
         openRelayMess[1] = _relays[i];
         fprintf( stdout, "0x%.2x 0x%.2x 0x%.2x " , (uint8_t)openRelayMess[0], (uint8_t)openRelayMess[1], (uint8_t)openRelayMess[2] );
         strncpy( ( _rs485OpenMsg + ( i * _FRAME_LENGTH ) ) , openRelayMess, _FRAME_LENGTH );
      }
      fprintf( stdout, "]\n" );
   }

   if( _openTimeFlag || ( _stateFlag && ( strcmp( _relayState, "off" ) == 0 ) ) )
   {
      _rs485CloseMsg = malloc( sizeof(char) * ( _FRAME_LENGTH * _numOfRelays )  );
      memset( _rs485CloseMsg, 0, ( _FRAME_LENGTH * _numOfRelays ) );
      fprintf( stdout, "%s %s()::CloseRelaysMessage: [ " , LOG_INFO, __func__ );
      for( uint8_t i = 0; i < _numOfRelays; i++ )
      {
         closeRelayMess[1] = _relays[i];
         fprintf( stdout, "0x%.2x 0x%.2x 0x%.2x " , (uint8_t)closeRelayMess[0], (uint8_t)closeRelayMess[1], (uint8_t)closeRelayMess[2] );
         strncpy( ( _rs485CloseMsg + ( i * _FRAME_LENGTH ) ) , closeRelayMess, _FRAME_LENGTH );
      }
      fprintf( stdout, "]\n" );
   }

   clock_t startTime;

   while( _impulses > 0 )
   {
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
       // OPEN RELAY/S //////////////////////////////////////////////////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // OPEN COM PORT
      if( _openTimeFlag || ( _stateFlag && ( strcmp( _relayState, "on" ) == 0 ) ) )
      {
         if( !tryOpenVCP( vcp, _MAX_OPEN_VCP_TRIES ) )
         {
            fprintf( stdout, "%s Could not open Port BEFORE send OPEN relay message\n", LOG_ERROR );
            free( vcp );   // Free allocated memory
            return -1;
         }
         //fprintf( stdout, "%s %s()::Sending message to open relays\n", LOG_INFO, __func__ );
         sendFrameVCP( vcp, _rs485OpenMsg, _numOfRelays * 3 );
         startTime = clock();

         // CLOSE COM PORT
         if( !tryCloseVCP( vcp, _MAX_CLOSE_VCP_TRIES ) )
         {
            fprintf( stdout, "%s Could not close Port AFTER send OPEN relay message\n", LOG_ERROR );
            free( vcp );   // Free allocated memory
            return -1;
         }
      }

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // WAIT UNTIL OPENING TIME HAS PASSED (SLEEP) ////////////////////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      if( _openTimeFlag )
      {
         while( clock() - startTime < _openTime )
         {
            Sleep( _SLEEP_TIME );   // Allow other processes to resume, avoids to colapse the CPU
         }
      }

      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // CLOSE RELAY/S /////////////////////////////////////////////////////////////////////////////////////////////////
      //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // OPEN COM PORT
      if( _openTimeFlag || ( _stateFlag && ( strcmp( _relayState, "off" ) == 0 ) ) )
      {

         if( !tryOpenVCP( vcp, _MAX_OPEN_VCP_TRIES ) )
         {
            fprintf( stdout, "%s Could not open Port BEFORE send CLOSE relay message\n", LOG_ERROR );
            free( vcp );   // Free allocated memory
            return -1;
         }
         //fprintf( stdout, "%s %s()::Sending message to close relays\n", LOG_INFO, __func__ );
         sendFrameVCP( vcp, _rs485CloseMsg, ( _numOfRelays * 3 ) );

         // CLOSE COM PORT
         if( !tryCloseVCP( vcp, _MAX_CLOSE_VCP_TRIES ) )
         {
            fprintf( stdout, "%s Could not close Port AFTER send CLOSE relay message\n", LOG_ERROR );
            free( vcp );   // Free allocated memory
            return -1;
         }
      }

      _impulses--;
   }
   free( _relays );
   free( _rs485OpenMsg );
   free( _rs485CloseMsg );
   free( vcp );   // Free allocated memory
   return 0;      // Everything right
}
// END main( .. ) ...



// PRIVATE FUNCTIONS ///////////////////////////////////////////////////////////////////////////////////////////////////
/***********************************************************************************************************************
 * parseArgs( .. )
 * @brief: Function to parse main function arguments. The only arguments requiered are "-relay" and "-openTime"
 * @param1 <int> argc : Total number of arguments
 * @param2 <char[] *> argv : Array containing the arguments
 * @return: Number of parameters read
 **********************************************************************************************************************/
static int parseArgs( int argc, char *argv[] )
{
   int argn = 1;  // Initial number of arguments is always one "The name of the executable"

   // No arguments will show help command wich explains how to use the software
   if( argn == argc )
   {
      fprintf( stdout, "\n\'RelayManager.exe\' is waiting for parameters:\n" );
      fprintf( stdout, "It is mandatory to pass '-relay' argument in one of the following modalities:\n" );
      fprintf( stdout, " [%s n]      (s=Single number). Ex: -relay 2\n", ARG_RELAY_NUM );
      fprintf( stdout, " [%s n:n]    (s=Two numbers \':\' separated) Indicates a range of relays. Ex: -relay 4:10\n",
               ARG_RELAY_NUM );
      fprintf( stdout, " [%s n,n...] (s=Seveal numbers \',\' separated) Indicates a group of relays. Ex: -relay 2,7,11\n",
               ARG_RELAY_NUM );
      fprintf( stdout, "It is also mandatory to pass '-openTime' or '-state' but not both at the same time:\n" );
      fprintf( stdout, " [%s m]   (m=number of milliseconds)\n", ARG_OPEN_TIME );
      fprintf( stdout, " [%s b]      (b=State \"on\" \"off\". It is set \"%s\" by default)\n\n", ARG_RELAY_STATE, RELAY_STATE_DEFAULT );
      fprintf( stdout, "There is another aditional argument that can be used with '-openTime':\n" );
      fprintf( stdout, " [%s n]   (OPTIONAL, n=number of impulses. 1 by default.)\n\n", ARG_IMPULSES );
      fprintf( stdout, "There are other optional arguments related to the virtual UART communication port:\n" );
      fprintf( stdout, " [%s x]   (OPTIONAL, x=Baudrate for uart communication. It is set %d by default)\n", ARG_BAUD_RATE, BAUD_RATE_DEFAULT );
      fprintf( stdout, " [%s n]    (OPTIONAL, n=COM port number. It is set %d by default)\n\n", ARG_COM_PORT, COM_PORT_DEFAULT );
      return 1;
   }

   // Loop to parse arguments
   while( argn < argc )
   {
      // RELAY argument found ( ARGUMENT REQUIERED )
      if( strcmp( argv[argn], ARG_RELAY_NUM ) == 0 )
      {
         // Parse relay number we pretend to use
         if( ++argn < argc )
         {
            int8_t modality = relayModality( argv[argn], &_numOfRelays );
            switch( modality )
            {
               case _errorRelay:
                  return -1;
                  break;
               case _singleRelay:
                  _singleRelayFlag = true;
                  _relayEnd = _relayBegin;
                  _relays = malloc( sizeof( uint8_t ) * _numOfRelays );
                  _relays[0] = _relayEnd;
                  break;
               case _rangeRelays:
                  if( _relayEnd <= _relayBegin )
                  {
                     fprintf( stderr, "Wrong range order, final relay number (%d) must higher than beginner relay (%d)\n",
                                      _relayEnd, _relayBegin );
                     return -1;
                  }
                  else
                  {
                     _numOfRelays =  _relayEnd - _relayBegin + 1 ;
                     _relays = malloc( sizeof( uint8_t ) * _numOfRelays );
                     for( uint8_t i = _relayBegin; i <= _relayEnd; i++ )
                     {
                        _relays[i-_relayBegin] = i;
                     }
                  }
                  break;
               case _groupRelays:
                  _relays = malloc( sizeof( uint8_t ) * _numOfRelays );
                  _fillRelaysGroup( argv[argn], _relays, _numOfRelays );
            }
         }
         else
         {
            fprintf( stderr, "%s Relay number error\n", LOG_ERROR );
            return 1;   // Get out of main function
         }
      }
      // OPEN TIME argument found ( ARGUMENT REQUIERED )
      else if( strcmp( argv[argn], ARG_OPEN_TIME ) == 0 )
      {
         // Check if '-state' argument has been passed
         if( _stateFlag )
         {
            fprintf( stderr, "%s Can't use \'-openTime\' if  \'-state\' argument has been passed before\n",
                     LOG_ERROR );
            return -1;   // Get out of main function
         }
         // Parse time we pretend to open selected relay
         if( ++argn < argc )
         {
            _openTime = atoi( argv[argn] );
            fprintf( stdout, "%s Relay asked to be opened %d milliseconds\n", LOG_INFO, _openTime );
            _openTimeFlag = true;
         }
         else
         {
            fprintf( stderr, "%s Time unknown\n", LOG_ERROR );
            return -1;   // Get out of main function
         }
      }
      // IMPULSES argument found ( ARGUMENT OPTINAL [ 1 by DEFAULT ] )
      else if( strcmp( argv[argn], ARG_IMPULSES ) == 0 )
      {
         // Parse number of times we pretend to open selected relay
         if( ++argn < argc )
         {
            _impulses = atoi( argv[argn] );
            if( _impulses < 0 )
            {
               fprintf( stderr, "%s Not accepted a negative number of impulses\n", LOG_ERROR );
               return 1;   // Get out of main function
            }
            _impulsesFlag = true;
            fprintf( stdout, "%s Give %d impulses \n", LOG_INFO, _impulses );
         }
         else
         {
            fprintf( stderr, "%s Time unknown\n", LOG_ERROR );
            return 1;   // Get out of main function
         }
      }
      // BAUD RATE argument found ( NOT REQUIERED, THERE'S A DEFAULT BAUDRATE OF 9600 )
      else if( strcmp( argv[argn], ARG_BAUD_RATE ) == 0)
      {
         // Parse baud rate
         if ( ++argn < argc && ( ( _baudrate = atoi( argv[argn] ) ) > 0) )
         {
            fprintf( stdout, "%s %d baud rate specified\n", LOG_INFO, _baudrate );
         }
         else
         {
            fprintf( stderr, "%s Baud rate error\n", LOG_WARNING );
            return 1;
         }
         // TODO: function that found if _baudrate is an acceptable value
      }
      // COM PORT argument found ( NOT REQUIERED, BETTER TO HARDCODE IT TO AVOID CONFUSIONS )
      else if( strcmp( argv[argn], ARG_COM_PORT ) == 0 )
      {
         // Parse COM port number.
         // SerialSend actually just begins searching at this number and continues working down to zero.
         if( ++argn < argc )
         {
            _comPortNumber = atoi( argv[argn] );
            fprintf( stdout, "%s Virtual port COM%d specified\n", LOG_INFO, _comPortNumber );
         }
         else
         {
            fprintf( stderr, "%s Device number error\n", LOG_WARNING );
            return 1;   // Get out of main function
         }
      }
      // RELAY STATE argument found
      else if( strcmp( argv[argn], ARG_RELAY_STATE ) == 0 )
      {
         // Parse RELAY STATE value.
         if( ++argn < argc )
         {
            // Check if '-openTime' argument hasn't been passed
            if( !_openTimeFlag )
            {
               strcpy( _relayState, argv[argn] );
               if( ( strcmp( _relayState, "on" ) == 0 ) || ( strcmp( _relayState, "off" ) == 0 ) )
               {
                  fprintf( stdout, "%s Relay state set to \"%s\"\n", LOG_INFO, _relayState );
                  _stateFlag = true;
               }
               else
               {
                  fprintf( stderr, "%s \'-state\' only valid values are \"on\" or \"off\"\n", LOG_ERROR );
                  return -1;
               }

            }
            else
            {
               fprintf( stderr, "%s Can't use \'-state\' if  \'-openTime\' argument has been passed before\n",
                        LOG_ERROR );
               return -1;
            }
         }
         else
         {
            fprintf( stderr, "%s Device number error\n", LOG_WARNING );
            return -1;   // Get out of main function
         }
      }
      // UNKNOWN ARGUMENT
      else
      {
         fprintf( stderr, "%s Unknown argument \'%s\'\n", LOG_ERROR, argv[argn] );
         return -1;
      }
      // Next command line argument
      argn++;
   }

   if( _impulsesFlag && !_openTimeFlag )
   {
      fprintf( stderr, "%s \'-impulses\' only works with \'-openTime\'\n", LOG_ERROR );
      return -1;
   }

   fprintf( stdout, "%s Number of arguments: %d\n", LOG_INFO, ( argn - 1 ) );
   return ( argn - 1 );
}
// END parseArgs( .. ) ...


/***********************************************************************************************************************
 * f_relayModality( .. )
 * @brief: Function parse the relay modality choosen
 * @param1: <char*> relayArgument : Value for main argument '-relay'
 * @param2: <uint8_t*> argv : Array containing the arguments
 * @return: <relayModality_t> The modality selected 'singleRelay', 'range of relays' or 'group of relays'
 **********************************************************************************************************************/
static relayModality_t relayModality( char* relayArgument, uint8_t* numberOfRelays )
{
   relayModality_t returnValue = _singleRelay;
   uint32_t numberFound = 0;
   bool     newNumberFlag = false;
   while( *relayArgument != '\0' )
   {
      if( !( isdigit( *relayArgument ) || *relayArgument == ':' || *relayArgument == ',' ) )
      {
         fprintf( stderr, "%s Can't use \'-state\' if  \'-openTime\' argument has been passed before\n",
                  LOG_ERROR );
         return _errorRelay;
      }
      // If a colon is detected can be a range of relays
      else if( *relayArgument == ':' )
      {
         if( *numberOfRelays == 0 )
         {
            fprintf( stderr, "%s A range of relays must start with a number.\n", LOG_ERROR );
            return _errorRelay;
         }
         else if( _rangeRelayFlag )
         {
            fprintf( stderr, "%s A range of relays can only be composed of two numbers begin and end.\n",
                     LOG_ERROR );
            return _errorRelay;
         }
         else if( _multiRelayFlag )
         {
            fprintf( stderr, "%s A group of relays is only separated by \',\'.\n",
                     LOG_ERROR );
            return _errorRelay;
         }
         else
         {
            newNumberFlag   = false;
            _rangeRelayFlag = true;
            returnValue = _rangeRelays;
         }
      }
      // If a comma is detected can be a group of specific relays
      else if( *relayArgument == ',' )
      {
         if( numberOfRelays == 0 )
         {
            fprintf( stderr, "%s A group of relays must start with a number.\n", LOG_ERROR );
            return _errorRelay;
         }
         else if( _rangeRelayFlag )
         {
            fprintf( stderr, "%s A range of relays can only be composed of two numbers begin and end.\n",
                     LOG_ERROR );
            return _errorRelay;
         }
         else
         {
            newNumberFlag = false;
            if( !_multiRelayFlag )
            {
               _multiRelayFlag = true;
               returnValue = _groupRelays;
            }
         }
      }
      else
      {
         if( !newNumberFlag )
         {
            (*numberOfRelays)++;
            newNumberFlag = true;
            numberFound = 0;
         }
         numberFound = 10 * numberFound + (*relayArgument - '0' );

         if( *numberOfRelays == 1 ) _relayBegin = numberFound;
         else _relayEnd = numberFound;
      }
      relayArgument++;
   }
   return returnValue;
}
// END f_relayModality( .. ) ...


/***********************************************************************************************************************
 * f_fillRelaysGroup( .. )
 * @brief: Function parse the relay modality chosen
 * @param1: <char*> relayArgument: The main argument containing the relays
 * @param2: <uint8_t*> array: Dynamic array to contain the relays selected
 * @param3: <uint8_t> size: Size of the dynamic array
 * @return: <void> None
 **********************************************************************************************************************/
static void _fillRelaysGroup( char* relayArgument, uint8_t* array, const uint8_t size )
{
   uint8_t  arrayIndex = 0;
   uint32_t numberFound = 0;
   bool     newNumberFlag = false;

   while( *relayArgument != '\0' )
   {
      if( *relayArgument == ',' )
      {
         newNumberFlag = false;
      }
      else
      {
         if( !newNumberFlag )
         {
            arrayIndex++;
            newNumberFlag = true;
            numberFound = 0;
         }
         numberFound = 10 * numberFound + (*relayArgument - '0' );
         if( numberFound > MAX_RELAYS_IN_RS485_CHAIN || numberFound < MIN_RELAY_NUMBER )
         {
            fprintf( stderr, "%s Valid relay numbers must be between %d and %d both included\n",
                             LOG_ERROR,MIN_RELAY_NUMBER, MAX_RELAYS_IN_RS485_CHAIN );
         }
         array[arrayIndex-1] = numberFound;
      }
      relayArgument++;
   }
}
// END f_relayModality( .. ) ...
