/*
===============================================================================
 Name        : ED3_ej_vumetro.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description :
 	1. Generar con timer0 una señal de freq. variable.
	2. Usando el capture “medir” el periodo usando otro timer.
	3. Prender leds tipo vumetro según la frecuencia.
	4. Con un pulsador cambiar la frecuencia de pasos de 100khz. Actualizar el
	   vumetro.

	GENERADOR
		INPUT -> P2.10 (pulsador a masa)
		OUTPUT -> P1.28 (MAT0.0)
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_clkpwr.h"
#include "lpc17xx_exti.h"
#endif

#include <cr_section_macros.h>

void timer0_conf (void);
void exti_conf (void);

static uint8_t select_freq= 0;
uint32_t MR0_array[10]= {49, 55, 62, 70, 82, 99, 124, 166, 249, 499}; //para generar señales de [100KHz-1MHz]
uint32_t MR1_array[10]= {99, 110, 124, 142, 166, 199, 249, 332, 499, 999}; //si PR=0 y PCLK= 100MHz

int main(void) {
	timer0_conf();
	exti_conf();
    return 0 ;
}

void timer0_conf (void) {
	TIM_TIMERCFG_Type timer0_conf;
	TIM_MATCHCFG_Type timer0_match_conf;
	PINSEL_CFG_Type mat_pin_conf;
	uint8_t reset_array[2] = {DISABLE, ENABLE};
	uint32_t match_array[2] = {499, 999}; //para una onda cuadrada inicial de salida f=100KHz

	//MAT pin config (P1.28 -> MAT0.0)
	mat_pin_conf.Pinnum = 28;
	mat_pin_conf.Portnum = 1;
	mat_pin_conf.Funcnum = PINSEL_FUNC_3;
	mat_pin_conf.Pinmode = PINSEL_PINMODE_PULLUP; //?
	PINSEL_ConfigPin(&mat_pin_conf);

	//timer0 config
	timer0_conf.PrescaleOption = TIM_PRESCALE_TICKVAL;
	timer0_conf.PrescaleValue  = 1; //(La funcion resta 1 al valor) PR=0 -> maxima resolucion

	//match0,1 config
	timer0_match_conf.IntOnMatch = DISABLE;
	timer0_match_conf.StopOnMatch = DISABLE;
	timer0_match_conf.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;

	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timer0_conf);
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_TIMER0, CLKPWR_PCLKSEL_CCLK_DIV_1); //Debido a que TIM_Init hace CCLK/4
	TIM_Cmd(LPC_TIM0, DISABLE);

	for(int i= 0; i<2; i++) {
		timer0_match_conf.MatchChannel = (uint8_t) i;
		timer0_match_conf.ResetOnMatch = reset_array[i];
		timer0_match_conf.MatchValue = match_array[i];
		TIM_ConfigMatch(LPC_TIM0, &timer0_match_conf);
	}

	TIM_Cmd(LPC_TIM0, ENABLE);
	TIM_ResetCounter(LPC_TIM0);
	NVIC_EnableIRQ(TIMER0_IRQn);
}

void exti_conf (void) {
	PINSEL_CFG_Type pinEintConf;

	//P2.10 -> EINT0
	pinEintConf.Pinnum  = (uint8_t) 10;
	pinEintConf.Portnum = PINSEL_PORT_2;
	pinEintConf.Pinmode = PINSEL_PINMODE_PULLUP;
	pinEintConf.Funcnum = PINSEL_FUNC_1;

	PINSEL_ConfigPin(&pinEintConf);

	//EXTI conf
	EXTI_InitTypeDef eint0Conf;
	eint0Conf.EXTI_Line     = EXTI_EINT3;
	eint0Conf.EXTI_Mode     = EXTI_MODE_EDGE_SENSITIVE;
	eint0Conf.EXTI_polarity = EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE;

	EXTI_Config(&eint0Conf);
	EXTI_ClearEXTIFlag(EXTI_EINT0);
	NVIC_EnableIRQ(EINT0_IRQn);
}

void EINT0_IRQHandler (void) {
	TIM_UpdateMatchValue(LPC_TIM0, 0, MR0_array[select_freq]);
	TIM_UpdateMatchValue(LPC_TIM0, 1, MR1_array[select_freq]);
	if(select_freq == 9) select_freq = 0;
	else select_freq++;
	EXTI_ClearEXTIFlag(EXTI_EINT0);
}
