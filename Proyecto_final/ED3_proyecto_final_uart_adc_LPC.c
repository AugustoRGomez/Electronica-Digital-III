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
#include "lpc17xx_adc.h"
#include "lpc17xx_timer.h"
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
#define LPC_UART_0 (LPC_UART_TypeDef*) LPC_UART0_BASE
#define LED_RED_TOGGLE() LPC_GPIO0 -> FIOPIN = (LPC_GPIO0 -> FIOPIN) ^ (1 << 22)
#define LED_BLUE_TOGGLE() LPC_GPIO3 -> FIOPIN = (LPC_GPIO3 -> FIOPIN) ^ (1 << 26)
#define LED_GREEN_TOGGLE() LPC_GPIO3 -> FIOPIN = (LPC_GPIO3 -> FIOPIN) ^ (1 << 25)
#define TIME 1000000U
#define IN_BUFF_LEN 12000u

uint32_t currentKey;
uint8_t readyToLoad, flagAdcDone;
uint8_t cmd[3];
uint16_t inBuff[IN_BUFF_LEN];
uint16_t *pInBuff = inBuff;

void initUART0(void);
void initLEDPins(void);
void initADC(void);
void initTMR0(void);
void initTMR2(void);
void blinkRedLed(void);
void blinkGreenLed(void);
void blinkBlueLed(void);
void delay_ms(int);

int main(void) {
	/* Init */
	initLEDPins();
	initUART0();
	initADC();
	initTMR0();
	initTMR2();

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
				UART_Send(LPC_UART_0, (uint8_t *)&currentKey, sizeof(currentKey), BLOCKING);
				blinkRedLed();
				break;
			case 's':
				//Set a new key
				UART_Send(LPC_UART_0, &readyToLoad, sizeof(readyToLoad), BLOCKING);
				UART_Receive(LPC_UART_0, (uint8_t *)&currentKey, sizeof(currentKey), BLOCKING);
				blinkGreenLed();
				break;
			case 'r':
				//Record a 2sec message
				TIM_ResetCounter(LPC_TIM0);
				TIM_Cmd(LPC_TIM0, ENABLE);
				while(!flagAdcDone) {};
				TIM_Cmd(LPC_TIM0, DISABLE);
				blinkBlueLed();
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
	 * P0.2 TX0
	 * P0.3 RX0
	 */

	/* Init con drivers */
	UART_CFG_Type uart0Config;
	uart0Config.Baud_rate = 115200;
	uart0Config.Databits = UART_DATABIT_8;
	uart0Config.Parity = UART_PARITY_NONE;
	uart0Config.Stopbits = UART_STOPBIT_1;
	UART_Init(LPC_UART_0, &uart0Config);
	UART_IntConfig(LPC_UART_0, UART_INTCFG_RBR, ENABLE);
	NVIC_EnableIRQ(UART0_IRQn);

//	LPC_PINCON->PINSEL0 |= (1<<4)|(1<<6); //funcion TXD0/RXD0 para los pines P0.2/P0.3
//	//LPC_SC->PCONP |= 1<<3; //enciendo UART0 (por default encendido)
//
//	LPC_UART_0->LCR = 3|DLAB_BIT ; //8 bits, sin paridad, 1 Stop bit y DLAB enable
//	LPC_UART_0->DLL = 12;
//	LPC_UART_0->DLM = 0;
//
//	LPC_UART_0->IER |= Rx_RDA_INT; //int cada vez que se reciban cierto numero de char (1 en este caso)
//	LPC_UART_0->FCR |= Ux_FIFO_EN|Rx_FIFO_RST|Tx_FIFO_RST;
//	LPC_UART_0->FDR = (MULVAL<<4)|DIVADDVAL; //MULVAL=15(bits - 7:4), DIVADDVAL=2(bits - 3:0)
//	LPC_UART_0->LCR &= ~(DLAB_BIT); //desactivando DLAB lockeamos los divisores para el Baudrate
//	NVIC_EnableIRQ(UART0_IRQn);
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
}

void initADC(void) {
	/*
	 * P0.23 AD0[0]
	 */
	PINSEL_CFG_Type ADC_pin;
	ADC_pin.Pinnum = 23;
	ADC_pin.Portnum = 0;
	ADC_pin.Funcnum = 1;
	ADC_pin.Pinmode = 2; //TRISTATE

	PINSEL_ConfigPin(&ADC_pin);

	//conf del ADC
	LPC_SC-> PCLKSEL0 |= (3 << 24);  // CCLK/8 = 100Mhz/8 = 12.5 MHz
	ADC_Init(LPC_ADC, 192307); //enciende PCONP, da PDN = 1 y carga un CLKDIV medio choto
	ADC_ChannelCmd(LPC_ADC, 0, ENABLE); //selecciona bits SEL, o sea el canal habiliado, recordar que es modo hardware
	ADC_BurstCmd(LPC_ADC, DISABLE); //configura BURST de ADCR
	ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01); //configura START de ADCR
	ADC_EdgeStartConfig(LPC_ADC, ADC_START_ON_FALLING); //configura EDGE de ADCR
	ADC_IntConfig(LPC_ADC, ADC_ADGINTEN, SET); //configura individualmente las int por canales o la global
	NVIC_EnableIRQ(ADC_IRQn);
}

void initTMR0(void) {
	/*
	 * No hace falta configurar PINSEL
	 * Recordar F_MRx = 2*F_s(deseada)
	 * F_s = 6 KHz -> F_MRx = 12 KHz
	 * T_MRx = 83.33 us = res*(MRx+1)
	 * MRx = (T_MRx/res)-1 = 82.33 ~= 83
	 * F_s'= 5.952 KHz
	 */
	LPC_SC -> PCONP |= (1 << 1); //enciende TMR0 (recordar que ya lo esta por defecto);
	//LPC_SC -> PCLKSEL0 |= (1 << 2); //CCLK/1
	LPC_TIM0 -> EMR |= (3 << 6); //toglear (funcion 3) EM0 (MAT0.1)
	LPC_TIM0 -> PR = 24; //si PCLK = 25MHz entonces con ese PR se logra resolución de 1us
	LPC_TIM0 -> MR1 = 83;
	LPC_TIM0 -> MCR |= (2 << 3); //solo resetar cuando se llegue match, no int, no stop
	//LPC_TIM0 -> TCR = 3; //enable y reset en 1
	//LPC_TIM0 -> TCR &= ~(2); //quito el reset
}


void initTMR2(void) {
	/*
	 * Temporizador de resolucion de 1ms
	 */
	TIM_TIMERCFG_Type timer2_conf;

	//timer2 config
	timer2_conf.PrescaleOption = TIM_PRESCALE_USVAL;
	timer2_conf.PrescaleValue  = 1000; //resolucion de 1ms (si PCLK= 25MHz)

	TIM_Init(LPC_TIM2, TIM_TIMER_MODE, &timer2_conf); //TIM_Init no le da enable
}

void UART0_IRQHandler(void) {
	uint32_t uartIntSt = UART_GetIntId(LPC_UART_0); //valor del registro IIR

	if (((uartIntSt & MASK_INT_ID) >> 1) == MASK_RDA_INT) {
		UART_Receive(LPC_UART_0, &cmd[0], sizeof(cmd), BLOCKING);
	}
	//en teoria se limpia el flag cuando se efectua la lectura de los datos
}

void ADC_IRQHandler(void) {
	volatile uint16_t ADC_value;
	ADC_value = (ADC_GlobalGetData(LPC_ADC) >> 4) & 0xFFF;
	*pInBuff = ADC_value;

	if(pInBuff == &inBuff[IN_BUFF_LEN-1]) {
		pInBuff = inBuff;
		flagAdcDone = 1;
	}
	else {
		pInBuff++;
		flagAdcDone = 0;
	}
}

void blinkRedLed(void) {
	LED_RED_TOGGLE();
	delay_ms(100);
	LED_RED_TOGGLE();
}

void blinkGreenLed(void) {
	LED_GREEN_TOGGLE();
	delay_ms(100);
	LED_GREEN_TOGGLE();
}

void blinkBlueLed(void) {
	LED_BLUE_TOGGLE();
	delay_ms(100);
	LED_BLUE_TOGGLE();
}
void delay_ms(int time) {
	TIM_ResetCounter(LPC_TIM2);
	TIM_Cmd(LPC_TIM2, ENABLE);
	while (LPC_TIM2 -> TC < time); //en [ms]
	TIM_Cmd(LPC_TIM2, DISABLE);
}
