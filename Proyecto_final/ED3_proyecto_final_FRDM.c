
/**
 * @file    ED3_proyecto_final_uart_FRDM.c
 * @brief
 * 	Funciones:
 * 		*Mediante UART, actualizar la cabecera de datos (cabecera de 32 bits)
 * 		*Mediante UART, enviar a la PC el valor de la cabecera actual
 *
 * 	Cosas por hacer:
 * 		*En vez de hacerlo con UART0, hacerlo con otra UART
 * 		*Reducir baudRate a 9600 quiza se pueda ver en el osciloscopio
 * 		*Usar el modulo Serial to USB
 *
 * 	UART_config (default)
 * 	 uartConfig->baudRate_Bps = 115200U;
 *   uartConfig->bitCountPerChar = kUART_8BitsPerChar;
 *   uartConfig->parityMode = kUART_ParityDisabled;
 *   uartConfig->stopBitCount = kUART_OneStopBit;
 *   uartConfig->txFifoWatermark = 0;
 *   uartConfig->rxFifoWatermark = 1;
 *   uartConfig->idleType = kUART_IdleTypeStartBit;
 *   uartConfig->enableTx = false;
 *   uartConfig->enableRx = false;
 *
 *  Comandos:
 *  	.1 = set key
 *  	.2 = get Key
 *
 */

#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "fsl_uart.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MK64F12.h"
#include "fsl_debug_console.h"

/* TODO: insert other definitions and declarations here. */
#define UART0_CLK_FREQ   CLOCK_GetFreq(UART0_CLK_SRC)
#define UART1_CLK_FREQ   CLOCK_GetFreq(UART1_CLK_SRC)
#define TIME 1200000U

uint32_t currentKey;
uint64_t temporalKey;
uint8_t buttonFlag, readyToLoad;
uint8_t cmd[3];

void delay (void);

/*
 * @brief   Application entry point.
 */
int main(void) {

    /* Init board hardware. */
	BOARD_InitBootPins();
    BOARD_InitButtonsPins();
    BOARD_InitLEDsPins();
    BOARD_InitBootPeripherals();
    BOARD_InitBootClocks();

#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif

    /* UART Init */
	uart_config_t myUartConfig;
	UART_GetDefaultConfig(&myUartConfig);

	myUartConfig.baudRate_Bps = 9600u;
	myUartConfig.enableTx = 1;
	myUartConfig.enableRx = 1;

	port_pin_config_t uartPin = {
			/* Internal pull-up resistor is enabled */
			kPORT_PullUp,
			/* Fast slew rate is configured */
			kPORT_FastSlewRate,
			/* Passive filter is enabled */
			kPORT_PassiveFilterDisable,
			/* Open drain is disabled */
			kPORT_OpenDrainDisable,
			/* Low drive strength is configured */
			kPORT_LowDriveStrength,
			/* Pin is configured as UART1 */
			kPORT_MuxAlt3,
			/* Pin Control Register fields [15:0] are not locked */
			kPORT_UnlockRegister};

	CLOCK_EnableClock(kCLOCK_PortC); //solo para UART1
	PORT_SetPinConfig(PORTC, 3, &uartPin); //solo para UART1: PTC3 -> UART1-RX
	PORT_SetPinConfig(PORTC, 4, &uartPin); //solo para UART1: PTC4 -> UART1-TX
	UART_Init(UART1, &myUartConfig, UART1_CLK_FREQ);

    /* Algorithm */
    currentKey = 0x12345678; //valor por defecto
    readyToLoad = 1;

    while (1) {
    	UART_ReadBlocking(UART1, &cmd[0], 3);
    	if (cmd[0] == '.') {
    		switch (cmd[1]) {
    		case 'g':
    			//Get current key
    			UART_WriteBlocking(UART1, (uint8_t *)&currentKey, sizeof(currentKey));
    			LED_RED_TOGGLE();
    			delay();
    			LED_RED_TOGGLE();

    			break;
    		case 's':
    			//Set a new key
    			UART_WriteBlocking(UART1, &readyToLoad, sizeof(readyToLoad)); //acknowledge
    			UART_ReadBlocking(UART1, (uint8_t *)&currentKey, sizeof(currentKey));
    			LED_GREEN_TOGGLE();
    			delay();
    			LED_GREEN_TOGGLE();
    			break;
    		}
    		cmd[0]=0;
    		cmd[1]=0;
    		//UART0 -> CFIFO |= (1 << 6); //flush rx FIFO
    	}
    }
    return 0 ;
}

/* PORTC_IRQn interrupt handler */
void GPIOC_IRQHANDLER(void) {
  /* Get pin flags */
  uint32_t pin_flags = GPIO_PortGetInterruptFlags(GPIOC);

  /* Place your interrupt code here */
  buttonFlag = 1;
  /* Clear pin flags */
  GPIO_PortClearInterruptFlags(GPIOC, pin_flags);

  /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F
     Store immediate overlapping exception return operation might vector to incorrect interrupt. */
  #if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
  #endif
}

void delay(void) {
	uint32_t i = 0;
	while (i < TIME) {
		i++;
	}
}
