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
            // Verificar se existem bytes recebidos
            rx_len = TCP_GetRxLength(&portTCB);
            if (rx_len > 0) {
                // Obter os dados recebidos disponíveis
                rx_len = TCP_GetReceivedData(&portTCB);  // lê os dados recebidos

                // Reutilizar o buffer RX
                TCP_InsertRxBuffer(&portTCB, rxdata, sizeof(rxdata));

                // Pegar tudo o que recebemos através do socket e enviar
                // através da USART
                for (int i = 0; i < rx_len; i++)
                    EUSART1_Write(rxdata[i]);
            }

            if (t_client >= socketTimeout) {
                // Enviar uma mensagem a cada dois segundos
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
            // Remove o socket quando está a fechar
            TCP_SocketRemove(&portTCB);
            break;

        default:
            // Isto não deveria acontecer
            break;
    }
}

void main(void) {
    // Inicializa o dispositivo
    SYSTEM_Initialize();

    // Ativar as Interrupções Globais
    INTERRUPT_GlobalInterruptEnable();

    // Desativar as Interrupções Globais
    // INTERRUPT_GlobalInterruptDisable();

    // Ativar as Interrupções Periféricas
    INTERRUPT_PeripheralInterruptEnable();

    // Desativar as Interrupções Periféricas
    // INTERRUPT_PeripheralInterruptDisable();

    TMR0_SetInterruptHandler(my_tmr0_callback);
    // Inicia o temporizador
    TMR0_StartTimer();

    // Lê os valores dos canais ADC
    ReadADCChannels();
    // Imprime mensagem de teste
    printf("ola\n\r");

    while (1) {
        // Código principal da aplicação

        // Gere a rede e mantém as ligações ativas
        Network_Manage();
        // Obtém o endereço IP atual
        ip = ipdb_getAddress();
        // Verifica se o IP mudou ou se não há IP atribuído
        if ((ip == 0) || (old_ip != ip)) {
            old_ip = ip;
            // Converte o IP para formato de string legível
            ip_str = makeIpv4AddresstoStr(ip);
        }
        // Envia cada caractere do endereço IP através da EUSART
        for (int i = 0; i < 16; i++) {
            EUSART1_Write(ip_str[i]);
        }

        // Envia caracteres de nova linha e retorno de carro
        EUSART1_Write('\n');
        EUSART1_Write('\r');

        // Se tivermos um IP válido, executa o cliente TCP
        if (ip != 0) {
            TCP_Client();  // Chamada da função cliente TCP
        }
        // Fim do ciclo while(1)

#if print_adc
        // Lê o canal ADC 0 em modo de sondagem
        ADCC_StartConversion(channel_ANA0);
        while (!ADCC_IsConversionDone());  // Sondagem - espera até que a conversão esteja completa
        adc_value0 = ADCC_GetConversionResult();

        // Lê o canal ADC 1
        ADCC_StartConversion(channel_ANA1);
        while (!ADCC_IsConversionDone());  // Sondagem - espera até que a conversão esteja completa
        adc_value1 = ADCC_GetConversionResult();

        // Lê o canal ADC 2
        ADCC_StartConversion(channel_ANA2);
        while (!ADCC_IsConversionDone());  // Sondagem - espera até que a conversão esteja completa
        adc_value2 = ADCC_GetConversionResult();

        // Imprime os valores ADC - usar isto se a EUSART estiver configurada para printf
        printf("ADC0: %u, ADC1: %u, ADC2: %u\r\n", adc_value0, adc_value1, adc_value2);
#endif
    }
}
/**
 Fim do Ficheiro
*/