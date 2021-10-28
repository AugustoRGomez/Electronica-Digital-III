/*
===============================================================================
 Name        : ED3_proyecto_final_uart_LPC.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description :
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#include "lpc17xx_uart.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#endif

#include <cr_section_macros.h>

#define MULVAL 14
#define DIVADDVAL 2
#define Ux_FIFO_EN (1<<0)
#define Rx_FIFO_RST (1<<1)
#define Tx_FIFO_RST (1<<2)
#define DLAB_BIT (1<<7)
#define Rx_RDA_INT (1<<0)
#define MASK_INT_ID (0xE)
#define MASK_RDA_INT (2)
#define LED_RED_TOGGLE() LPC_GPIO0 -> FIOPIN = (LPC_GPIO0 -> FIOPIN) ^ (1 << 22)
#define LED_BLUE_TOGGLE() LPC_GPIO3 -> FIOPIN = (LPC_GPIO3 -> FIOPIN) ^ (1 << 26)
#define LED_GREEN_TOGGLE() LPC_GPIO3 -> FIOPIN = (LPC_GPIO3 -> FIOPIN) ^ (1 << 25)
#define TIME 1000000U

uint32_t currentKey;
uint8_t buttonFlag, readyToLoad;
uint8_t cmd[3];

void initUART0(void);
void initLEDPins(void);
void blinkRedLed(void);
void blinkGreenLed(void);
void delay(void);

int main(void) {
	/* Init */
	initLEDPins();
	initUART0();

	/* Algorithm */
    currentKey = 0x12345678; //valor por defecto
    readyToLoad = 1;
	while(1){
		if (cmd[0] == '.') {
			/*
			 * decide que comando ejecutar:
			 * 'g': Get current Key
			 * 's': Set a new Key
			 * 'r': Record and send message (FALTA)
			 */
			switch (cmd[1]) {
			case 'g':
				//Get current key
				UART_Send(LPC_UART0, (uint8_t *)&currentKey, sizeof(currentKey), BLOCKING);
				blinkRedLed();
				break;
			case 's':
				//Set a new key
				UART_Send(LPC_UART0, &readyToLoad, sizeof(readyToLoad), BLOCKING);
				UART_Receive(LPC_UART0, (uint8_t *)&currentKey, sizeof(currentKey), BLOCKING);
				blinkGreenLed();
				break;
			}
			cmd[0]=0;
			cmd[1]=0;
		}
	};
    return 0 ;
}

void initUART0(void)
{
	/*
	 * BaudRate= ~115200(114882) (si PCLK=25MHz, lo es por defecto)
	 */
	LPC_PINCON->PINSEL0 |= (1<<4)|(1<<6); //funcion TXD0/RXD0 para los pines P0.2/P0.3
	//LPC_SC->PCONP |= 1<<3; //enciendo UART0 (por default encendido)

	LPC_UART0->LCR = 3|DLAB_BIT ; //8 bits, sin paridad, 1 Stop bit y DLAB enable
	LPC_UART0->DLL = 12;
	LPC_UART0->DLM = 0;

	LPC_UART0->IER |= Rx_RDA_INT; //int cada vez que se reciban cierto numero de char (1 en este caso)
	LPC_UART0->FCR |= Ux_FIFO_EN|Rx_FIFO_RST|Tx_FIFO_RST;
	LPC_UART0->FDR = (MULVAL<<4)|DIVADDVAL; //MULVAL=15(bits - 7:4), DIVADDVAL=2(bits - 3:0)
	LPC_UART0->LCR &= ~(DLAB_BIT); //desactivando DLAB lockeamos los divisores para el Baudrate
	NVIC_EnableIRQ(UART0_IRQn);
}

void initLEDPins(void) {
	/*
	 * P0.22 Red
	 * P3.26 Blue
	 * P3.25 Green
	 */
	const uint8_t ledNum[] = {22, 26, 25};
	const uint8_t ledPort[] = {0, 3, 3};

	PINSEL_CFG_Type ledPin;
	ledPin.Pinmode = PINSEL_PINMODE_PULLUP;
	ledPin.Funcnum = 0;

	for (int i = 0;i < 3; i++) {
		ledPin.Pinnum = ledNum[i];
		ledPin.Portnum = ledPort[i];
		PINSEL_ConfigPin(&ledPin);
	}

	GPIO_SetDir(PINSEL_PORT_0, (1<<PINSEL_PIN_22), 1);
	GPIO_SetDir(PINSEL_PORT_3, (1<<PINSEL_PIN_25)|(1<<PINSEL_PIN_26), 1);
}

void UART0_IRQHandler(void) {

	uint32_t uartIntSt = UART_GetIntId(LPC_UART0); //valor del registro IIR

	if (((uartIntSt & MASK_INT_ID) >> 1) == MASK_RDA_INT) {
		UART_Receive(LPC_UART0, &cmd[0], sizeof(cmd), BLOCKING);
	}
	//en teoria se limpia el flag cuando se efectua la lectura de los datos

}

void blinkRedLed(void) {
	LED_RED_TOGGLE();
	delay();
	LED_RED_TOGGLE();
}

void blinkGreenLed(void) {
	LED_GREEN_TOGGLE();
	delay();
	LED_GREEN_TOGGLE();
}

void delay(void) {
	volatile uint32_t i = 0;
	while (i < TIME) {
		i++;
	}
}

