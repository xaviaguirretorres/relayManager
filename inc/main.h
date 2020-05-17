/**********************************************************************************************************************
 * RelayManager
 * @brief:   This is a short program to interact with the KMTronic Relays board through Serial Virtual Port Com
             connection.
 * @author:  Xavier Aguirre Torres @ The MicroBoard Order
 * @version: V1.0
 * @date:    December 2019
 *
 **********************************************************************************************************************/
#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED



/* Includes ----------------------------------------------------------------------------------------------------------*/



/* Public/Global defines ---------------------------------------------------------------------------------------------*/
#ifndef MAX_PATH                    // MAX_PATH is normally defined by the system
   #define MAX_PATH                 256
#endif

#define RELAYS_MANAGER_HEADER_MSG   "------------------------------------------------------\n" \
                                    "| RELAY MANAGER                                      |\n" \
                                    "------------------------------------------------------\n"

#define MAX_RELAYS_PER_BOARD			8
#define MAX_BOARDS_IN_RS485_CHAIN	15
#define MAX_RELAYS_IN_RS485_CHAIN	( MAX_RELAYS_PER_BOARD * MAX_BOARDS_IN_RS485_CHAIN )
#define MIN_RELAY_NUMBER            1

// Log defines
#define LOG_WARNING                 "[WARN]"
#define LOG_ERROR                   "[ERR ]"
#define LOG_INFO                    "[INFO]"
#define LOG_DEBUG                   "[DBUG]"

// Default values
#define COM_PORT_DEFAULT            7
#define RELAY_STATE_DEFAULT         "off"

// Main Program arguments
#define ARG_RELAY_NUM               "-relay"
#define ARG_OPEN_TIME               "-openTime"
#define ARG_IMPULSES                "-impulses"
#define ARG_RELAY_STATE             "-state"

#define ARG_BAUD_RATE               "-baudRate"
#define ARG_COM_PORT                "-comPort"


#endif // MAIN_H_INCLUDED
