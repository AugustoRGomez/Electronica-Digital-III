/*
===============================================================================
 Name        : ED3_ej_vumetro.c
 Author      : AugustoRGomez
 Version     :
 Copyright   : $(copyright)
 Description :

	//OJO: Crear proyecto como C [Semihost]
	
 	1. Generar con timer0 una señal de freq. variable.
	2. Usando el capture “medir” el periodo usando otro timer.
	3. Prender leds tipo vumetro según la frecuencia.
	4. Con un pulsador cambiar la frecuencia de pasos de 100khz. Actualizar el
	   vumetro.

	GENERADOR
		INPUT -> P2.10 (pulsador a masa J2.51) 
		OUTPUT -> P1.28 (MAT0.0 PAT10)

	CONTADOR DE FREC (capture)
		INPUT -> P1.18 (CAP1.0 PAT19)

	LEDS VUMETRO
		OUTPUT -> P0.0 ... 9 (GPIO) no usados
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
#include "stdio.h"
#include <cr_section_macros.h>

#define GATING_TIME 1000 //[ms]

void timer0_conf (void);
void timer1_conf (void);
void timer2_conf (void);
void exti_conf (void);
void delay_ms(int);

static uint8_t select_freq= 0; //TODO: init en 1?
uint32_t MR0_array[10]= {499, 249, 166, 124, 99, 82, 70, 62, 55 ,49}; //para generar señales de [100KHz-1MHz]
uint32_t n_pulses, freq;

int main(void) {
	timer0_conf();
	timer1_conf();
	timer2_conf();
	exti_conf();

	while(1) {
		TIM_Cmd(LPC_TIM1, ENABLE);
		delay_ms(GATING_TIME);
		TIM_Cmd(LPC_TIM1, DISABLE);

		n_pulses= LPC_TIM1->TC;
		TIM_ResetCounter(LPC_TIM1);
		freq = (uint32_t) n_pulses/GATING_TIME; //resultado en [KHz], redondeado
		printf("f_in= %d [KHz]\n",freq);
	}

    return 0 ;
}

void delay_ms (int time) {
	TIM_ResetCounter(LPC_TIM2);
	TIM_Cmd(LPC_TIM2, ENABLE);
	while (LPC_TIM2 -> TC < time); //en [ms]
	TIM_Cmd(LPC_TIM2, DISABLE);
}

//Conf pines de salida GPIO
// void gpio_conf (void){
// 	//Asumo configuracion por defecto de PINSELx y PINMODEx
// 	GPIO_SetDir(PINSEL_PORT_0, (3<<8)|(0xFF), 1); //p0.0...9 como salidas
// }

//Generador de onda cuadrada frec. variable
void timer0_conf (void) {
	TIM_TIMERCFG_Type timer0_conf;
	TIM_MATCHCFG_Type timer0_match_conf;
	PINSEL_CFG_Type mat_pin_conf;

	//MAT pin config (P1.28 -> MAT0.0)
	mat_pin_conf.Pinnum = 28;
	mat_pin_conf.Portnum = 1;
	mat_pin_conf.Funcnum = PINSEL_FUNC_3;
	mat_pin_conf.Pinmode = PINSEL_PINMODE_PULLUP; //?

	PINSEL_ConfigPin(&mat_pin_conf);

	//timer0 config
	timer0_conf.PrescaleOption = TIM_PRESCALE_TICKVAL;
	timer0_conf.PrescaleValue  = 1; //(La funcion resta 1 al valor) PR=0 -> maxima resolucion

	//match0 config
	timer0_match_conf.MatchChannel = 0;
	timer0_match_conf.ResetOnMatch = ENABLE;
	timer0_match_conf.IntOnMatch = DISABLE;
	timer0_match_conf.StopOnMatch = DISABLE;
	timer0_match_conf.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
	timer0_match_conf.MatchValue = MR0_array[0];

	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timer0_conf); //TIM_Init no le da enable
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_TIMER0, CLKPWR_PCLKSEL_CCLK_DIV_1); //Debido a que TIM_Init hace CCLK/4
	TIM_ConfigMatch(LPC_TIM0, &timer0_match_conf);
	TIM_Cmd(LPC_TIM0, ENABLE);
	TIM_ResetCounter(LPC_TIM0); //TODO: necesario?
}

/* 
* Contador de frecuencia
* DATAZO: No hay que configurar el modo captura, porque el timer actua como contador de pulsos!
*/
void timer1_conf (void) {
	TIM_TIMERCFG_Type timer1_conf;
	//TIM_CAPTURECFG_Type timer1_capture_conf;
	PINSEL_CFG_Type cap_pin_conf;

	//CAP pin config (P1.18 -> CAP1.0)
	cap_pin_conf.Pinnum = 18;
	cap_pin_conf.Portnum = 1;
	cap_pin_conf.Funcnum = PINSEL_FUNC_3;
	cap_pin_conf.Pinmode = PINSEL_PINMODE_PULLUP; //?

	PINSEL_ConfigPin(&cap_pin_conf);

	//timer1 config
	timer1_conf.PrescaleOption = TIM_PRESCALE_TICKVAL;
	timer1_conf.PrescaleValue  = 1; //(La funcion resta 1 al valor) PR=0 -> maxima resolucion

	//capture0 config
	// timer1_capture_conf.CaptureChannel = 0;
	// timer1_capture_conf.FallingEdge = DISABLE;
	// timer1_capture_conf.RisingEdge = ENABLE;
	// timer1_capture_conf.IntOnCaption = DISABLE;

	TIM_Init(LPC_TIM1, TIM_COUNTER_RISING_MODE, &timer1_conf);
	CLKPWR_SetPCLKDiv(CLKPWR_PCLKSEL_TIMER1, CLKPWR_PCLKSEL_CCLK_DIV_1); //Debido a que TIM_Init hace CCLK/4
    //TIM_ConfigCapture(LPC_TIM1, &timer1_capture_conf);
	LPC_TIM1 -> CTCR |= (1<<0); //Bug en la funcion Tim_Init no cambia el registro CTCR! (flancos + en CAP1.0 incrementan TC)
}

//Temporizador de ventana de 1000ms (Gating-time)
void timer2_conf (void) {
	TIM_TIMERCFG_Type timer2_conf;

	//timer2 config
	timer2_conf.PrescaleOption = TIM_PRESCALE_USVAL;
	timer2_conf.PrescaleValue  = 1000; //resolucion de 1ms (si PCLK= 25MHz)

	TIM_Init(LPC_TIM2, TIM_TIMER_MODE, &timer2_conf); //TIM_Init no le da enable
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

//Pulsador, cambia la frecuencia de la onda generada
void EINT0_IRQHandler (void) {
	TIM_Cmd(LPC_TIM0, DISABLE);
	TIM_UpdateMatchValue(LPC_TIM0, 0, MR0_array[select_freq]);
	TIM_Cmd(LPC_TIM0, ENABLE);
	TIM_ResetCounter(LPC_TIM0); //No olvidar reset
	
	if(select_freq == 9) select_freq = 0;
	else select_freq++;
	EXTI_ClearEXTIFlag(EXTI_EINT0);
}
