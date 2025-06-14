/**
  Generated Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This is the main file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  Description:
    This header file provides implementations for driver APIs for all modules selected in the GUI.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.81.8
        Device            :  PIC18LF26K40
        Driver Version    :  2.00
*/

/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip software and any
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party
    license terms applicable to your use of third party software (including open source software) that
    may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
*/
#define print_adc 0
#include <time.h>

#include "mcc_generated_files/TCPIPLibrary/tcpv4.h"
#include "stdint.h"

// keep remote socket information

#include "mcc_generated_files/TCPIPLibrary/ip_database.h"
#include "mcc_generated_files/TCPIPLibrary/tcpip_config.h"
#include "mcc_generated_files/mcc.h"

/*
                         Main application
 */

extern struct time_t timev;
// keep remote IP address/Port Number for the TCP Client Demo
sockaddr_in4_t remoteSocket = {
    .addr.s_addr = MAKE_IPV4_ADDRESS(192, 168, 1, 102),
    .port = 8888};

unsigned long ip, old_ip = 0;
char *ip_str;

// Variables to store ADC readings
adc_result_t adc_value0 = 0;
adc_result_t adc_value1 = 0;
adc_result_t adc_value2 = 0;

uint8_t adc_flag = 0;

// Function to read ADC channels in polling mode
void ReadADCChannels(void) {
    // Read channel 0
    ADCC_StartConversion(channel_ANA0);
    while (!ADCC_IsConversionDone());  // Polling - wait until conversion is complete
    adc_value0 = ADCC_GetConversionResult();

    // Read channel 1
    ADCC_StartConversion(channel_ANA1);
    while (!ADCC_IsConversionDone());  // Polling - wait until conversion is complete
    adc_value1 = ADCC_GetConversionResult();

    // Read channel 2
    ADCC_StartConversion(channel_ANA2);
    while (!ADCC_IsConversionDone());  // Polling - wait until conversion is complete
    adc_value2 = ADCC_GetConversionResult();

    // Print ADC values - use this if EUSART is set up for printf
    printf("ADC0: %u, ADC1: %u, ADC2: %u\r\n", adc_value0, adc_value1, adc_value2);
}

void my_tmr0_callback(void) {
    printf("TMR0 Interrupt occurred!\n\r");
    // Read ADC channels
    adc_flag = 1;  // Set flag to indicate ADC read is needed
}

void TCP_Client(void) {
    unsigned char errCode;
    static time_t t_client;
    static time_t socketTimeout;

    // Create the socket for the TCP Client port Transmission Control Block
    static tcpTCB_t portTCB;

    // Create the TX and RX Client's buffers
    static uint8_t rxdata[50];
    static uint8_t txdata[50];

    // Used to check the socket state
    socketState_t socketState;

    // Other communication related variables
    uint16_t rx_len = 0;

    time(&t_client);

    socketState = TCP_SocketPoll(&portTCB);

    switch (socketState) {
        case NOT_A_SOCKET:
            // Initializing and store the socket connection information
            TCP_SocketInit(&portTCB);
            break;
        case SOCKET_CLOSED:
            // If the socket is closed we will try to connect again
            if (t_client >= socketTimeout) {
                // Try to connect once at 2 seconds
                socketTimeout = t_client + 2;
                TCP_InsertRxBuffer(&portTCB, rxdata, sizeof(rxdata));
                TCP_Connect(&portTCB, &remoteSocket);
            }
            break;

        case SOCKET_IN_PROGRESS:
            // close the socket
            if (t_client >= socketTimeout) {
                TCP_Close(&portTCB);
            }
            break;

        case SOCKET_CONNECTED:
            // Check for incoming bytes
            rx_len = TCP_GetRxLength(&portTCB);
            if (rx_len > 0) {
                // Get available incoming data
                rx_len = TCP_GetReceivedData(&portTCB);  // lê os dados recebidos

                // Reuse the RX buffer
                TCP_InsertRxBuffer(&portTCB, rxdata, sizeof(rxdata));

                // Take whatever we receive through the socket, and send it
                // through the USART
                for (int i = 0; i < rx_len; i++)
                    EUSART1_Write(rxdata[i]);
            }

            if (t_client >= socketTimeout) {
                // Send a message every couple of seconds
                if (TCP_SendDone(&portTCB)) {
                    socketTimeout = t_client + 2;

                    sprintf(txdata, "{\"adc0\":%u,\"adc1\":%u,\"adc2\":%u}\n",
                            adc_value0, adc_value1, adc_value2);
                    printf("%s", txdata);

                    errCode = TCP_Send(&portTCB, txdata, sizeof(txdata));
                }
            }
            break;

        case SOCKET_CLOSING:

            TCP_SocketRemove(&portTCB);
            break;

        default:
            // This should not happen
            break;
    }
}

void main(void) {
    // Initialize the device
    SYSTEM_Initialize();

    // If using interrupts in PIC18 High/Low Priority Mode you need to enable the Global High and Low Interrupts
    // If using interrupts in PIC Mid-Range Compatibility Mode you need to enable the Global and Peripheral Interrupts
    // Use the following macros to:

    // Enable the Global Interrupts
    INTERRUPT_GlobalInterruptEnable();

    // Disable the Global Interrupts
    // INTERRUPT_GlobalInterruptDisable();

    // Enable the Peripheral Interrupts
    INTERRUPT_PeripheralInterruptEnable();

    // Disable the Peripheral Interrupts
    // INTERRUPT_PeripheralInterruptDisable();

    TMR0_SetInterruptHandler(my_tmr0_callback);
    TMR0_StartTimer();

    ReadADCChannels();
    printf("ola\n\r");

    while (1) {
        // Add your application code

        Network_Manage();
        ip = ipdb_getAddress();
        if ((ip == 0) || (old_ip != ip)) {
            old_ip = ip;
            ip_str = makeIpv4AddresstoStr(ip);
        }
        for (int i = 0; i < 16; i++) {
            EUSART1_Write(ip_str[i]);
        }

        EUSART1_Write('\n');
        EUSART1_Write('\r');

        if (ip != 0) {
            TCP_Client();  // chamada da função cliente
        }
        // end while(1)

#if print_adc
        ADCC_StartConversion(channel_ANA0);
        while (!ADCC_IsConversionDone());  // Polling - wait until conversion is complete
        adc_value0 = ADCC_GetConversionResult();

        // Read channel 1
        ADCC_StartConversion(channel_ANA1);
        while (!ADCC_IsConversionDone());  // Polling - wait until conversion is complete
        adc_value1 = ADCC_GetConversionResult();

        // Read channel 2
        ADCC_StartConversion(channel_ANA2);
        while (!ADCC_IsConversionDone());  // Polling - wait until conversion is complete
        adc_value2 = ADCC_GetConversionResult();

        // Print ADC values - use this if EUSART is set up for printf
        printf("ADC0: %u, ADC1: %u, ADC2: %u\r\n", adc_value0, adc_value1, adc_value2);
#endif
    }
}
/**
 End of File
*/