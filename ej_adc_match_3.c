/*
===============================================================================
 Name        : ED3_adc_3.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description :

===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"
#endif

#include <cr_section_macros.h>

int main(void) {
	ADC_conf();
	TMR0_conf();
	GPIO_conf();
	while(1) {
	}
    return 0 ;
}

void ADC_conf(void) {
	//conf del pin (p0.23 -> AD0[0])
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

/*
 * Recordar F_MRx = 2*F_s(deseada)
 * F_MRx = 40 KHz
 */
void TMR0_conf(void) {
	//no hace falta configurar PINSEL
	//conf MR0 para Fs= 20KHz
	LPC_SC -> PCONP |= (1 << 1); //enciende TMR0 (recordar que ya lo esta por defecto);
	//LPC_SC -> PCLKSEL0 |= (1 << 2); //CCLK/1
	LPC_TIM0 -> EMR |= (3 << 6); //toglear (funcion 3) EM0 (MAT0.1)
	LPC_TIM0 -> PR = 24; //si PCLK = 25MHz entonces con ese PR se logra resolución de 1us
	LPC_TIM0 -> MR1 = 24; //interrumpe cada 25us -> 40KHz
	LPC_TIM0 -> MCR |= (2 << 3); //solo resetar cuando se llegue match, no int, no stop
	LPC_TIM0 -> TCR = 3; //enable y reset en 1
	LPC_TIM0 -> TCR &= ~(2); //quito el reset
}

void GPIO_conf(void) {
	PINSEL_CFG_Type gpio_pin_conf;
	gpio_pin_conf.Pinnum = 0;
	gpio_pin_conf.Portnum = 0;
	gpio_pin_conf.Pinmode = PINSEL_PINMODE_PULLUP;
	gpio_pin_conf.Funcnum = 0;

	PINSEL_ConfigPin(&gpio_pin_conf);
	GPIO_SetDir(0, 1, 1);
}

void ADC_IRQHandler(void) {
	volatile uint32_t ADC_value;
	ADC_value = (ADC_GlobalGetData(LPC_ADC) >> 4) & 0xFFF; //solo hay un canal habilitado

	if(ADC_value < 2047) GPIO_SetValue(0, 1);
	else GPIO_ClearValue(0, 1);
}
