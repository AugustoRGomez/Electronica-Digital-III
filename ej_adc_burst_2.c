/*
===============================================================================
 Name        : ED3_ej_adc_2.c
 Author      : AugustoRGomez
 Version     :
 Copyright   : $(copyright)
 Description :

	Configurar 4 canales del ADC para que funcionando en modo burst se obtenga
	una frecuencia de muestreo en cada uno de 50Kmuestras/seg. Suponer un
	cclk = 100 Mhz.

===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>
#include <stdio.h>

#define ADDR_INIT 0x10003000
#define ADDR_END (ADDR_INIT + 2048U)
#define MASK 0xFF

void ADC_config(void);

uint16_t *point = ADDR;
unit16_t avg_array[4];

int main(void) {
	ADC_config();
	while (1) {}
    return 0 ;
}

void ADC_config(void) {
	//pin config (P0.23...26)
	LPC_PINCON -> PINSEL1 |= (1 << 14) | (1 << 16) | (1 << 18) | (1 << 20); //configuracion como ADC (funcion 01)
	LPC_PINCON -> PINMODE1 |= (1 << 15) | (1 << 17) | (1 << 19) | (1 << 21) //sin pullUp ni pullDown (funcion 10)

	//ADC config
	LPC_SC -> PCONP |= (1 << 12);
	LPC_ADC -> ADCR |= (1 << 21); // PDN en 1
	LPC_ADC -> ADCR |= 0xF; // SEL primeros cuatro canales habilitados
	LPC_SC-> PCLKSEL0 |= (3 << 24);  // CCLK/8 = 100Mhz/8 = 12.5 MHz
	LPC_ADC -> ADCR &= ~(0xFF << 8); // PCLK/(CLKDIV+1)= PCLK/(0+1) = 12.5 MHz
	LPC_ADC -> ADCR |= (1 << 16); // BURST= 1
	LPC_ADC -> ADCR &= ~(0b111 << 24); // START debe ser 000 para burst mode (esta linea capaz no es necesaria)
	LPC_ADC -> ADINTEN |= 0xF; // canales 0 al 3 provocan interrupcion
	LPC_ADC -> ADINTEN &= ~(1 << 8); // ADGINTEN = 0 (solo los canales habilitados en ADINTEN provocan interrupciones)
	NVIC_EnableIRQ(ADC_IRQn); //
}

void ADC_IRQHandler(void) {
	if((LPC_ADC -> ADSTAT & 0x1) && (point == ADDR_INIT))
		*point = ((LPC_ADC -> ADGDR) >> 4) & 0xFFF;
	else
		*point = ((LPC_ADC -> ADGDR) >> 4) & 0xFFF;

	if(point == ADDR_END) point = ADDR_INIT;
	else point++;
}