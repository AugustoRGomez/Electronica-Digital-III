/*
===============================================================================
 Name        : ED3_ej_TMR_match_4.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description :

 Utilizando los 4 registros match del Timer 0 y las salidas P0.0, P0.1, P0.2
 y P0.3, realizar un programa en C que permita obtener las formas de ondas
 adjuntas, donde los pulsos en alto tienen una duración de 5 mseg. Un pulsador
 conectado a la entrada EINT3, permitirá elegir entre las dos secuencias mediante
 una rutina de servicio a la interrupción. La prioridad de la interrupción del
 Timer tiene que ser mayor que la del pulsador. Estas formas de ondas son muy
 útiles para controlar un motor paso a paso. Adjuntar el código en C.
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_exti.h"
#endif

#include <cr_section_macros.h>

#define INPUT  (uint8_t) 0
#define OUTPUT (uint8_t) 1
#define A (1<<0)
#define A_hat (1<<2)
#define B (1<<1)
#define B_hat (1<<3)


/*
 * Diferentes estados para pines GPIO
 */
typedef enum {
	state_0,
	state_1,
	state_2,
	state_3,
	state_4,
	state_5,
	state_6,
	state_7,
} GPIO_state;


void confGPIO(void);
void confEint(void);
void confTimer0(void);

GPIO_state current_state;
uint8_t eint_flag = 1;

int main(void) {
	confGPIO();
	confEint();
	confTimer0();
	//inicio de secuencia
	GPIO_ClearValue(PINSEL_PORT_0, 0xF);
	GPIO_SetValue(PINSEL_PORT_0, A);

	while(1) {
		switch(current_state) {
		/* state_0..3 secuencia 1
		 * state_4..7 secuencia 2
		 */
		case state_0:
			GPIO_ClearValue(PINSEL_PORT_0, A);
			GPIO_SetValue(PINSEL_PORT_0, B);
			break;
		case state_1:
			GPIO_ClearValue(PINSEL_PORT_0, B);
			GPIO_SetValue(PINSEL_PORT_0, A_hat);
			break;
		case state_2:
			GPIO_ClearValue(PINSEL_PORT_0, A_hat);
			GPIO_SetValue(PINSEL_PORT_0, B_hat);
			break;
		case state_3:
			GPIO_ClearValue(PINSEL_PORT_0, B_hat);
			GPIO_SetValue(PINSEL_PORT_0, A);
			break;
		case state_4:
			GPIO_ClearValue(PINSEL_PORT_0, B_hat);
			GPIO_SetValue(PINSEL_PORT_0, B);
			break;
		case state_5:
			GPIO_ClearValue(PINSEL_PORT_0, A);
			GPIO_SetValue(PINSEL_PORT_0, A_hat);
			break;
		case state_6:
			GPIO_ClearValue(PINSEL_PORT_0, B);
			GPIO_SetValue(PINSEL_PORT_0, B_hat);
			break;
		case state_7:
			GPIO_ClearValue(PINSEL_PORT_0, A_hat);
			GPIO_SetValue(PINSEL_PORT_0, A);
			break;
		}
	}
    return 0 ;
}

void confGPIO (void) {
	PINSEL_CFG_Type pinConf;
	pinConf.Portnum = PINSEL_PORT_0;
	pinConf.Pinmode = PINSEL_PINMODE_PULLUP;
	pinConf.Funcnum = PINSEL_FUNC_0;

	for(int i=0; i<4; i++) {
		pinConf.Pinnum  = (uint8_t) i;
		PINSEL_ConfigPin(&pinConf);
	}

	GPIO_SetDir(PINSEL_PORT_0, 0xF, OUTPUT); //p0.0 p0.1 p0.2 p0.3
}

void confEint (void) {
	PINSEL_CFG_Type pinEintConf;
	pinEintConf.Pinnum  = (uint8_t) 13;
	pinEintConf.Portnum = PINSEL_PORT_2;
	pinEintConf.Pinmode = PINSEL_PINMODE_PULLUP;
	pinEintConf.Funcnum = PINSEL_FUNC_1;

	PINSEL_ConfigPin(&pinEintConf);

	EXTI_SetMode(EXTI_EINT3, EXTI_MODE_EDGE_SENSITIVE);
	EXTI_SetPolarity(EXTI_EINT3, EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE);
	EXTI_ClearEXTIFlag(EXTI_EINT3);
	NVIC_EnableIRQ(EINT3_IRQn);

	/* Otra forma de hacerlo */
	//	EXTI_InitTypeDef eint3Conf;
	//	eint3Conf.EXTI_Line     = EXTI_EINT3;
	//	eint3Conf.EXTI_Mode     = EXTI_MODE_EDGE_SENSITIVE;
	//	eint3Conf.EXTI_polarity = EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE;
	//
	//	EXTI_Config(&eint3Conf);
	//	EXTI_ClearEXTIFlag(EXTI_EINT3);
	//	NVIC_EnableIRQ(EINT3_IRQn);
}

/* TIP:
 * si PR esta en [tiempo] -> PR[tick]  = PR[tiempo]*PCLK-1
 *    PR 	     [tick]   -> PR[tiempo]= (PR[tick]+1)/PCLK
 *
 * MRx = TiempoMatch/PR[tiempo]-1
 */
void confTimer0 (void) {
	TIM_TIMERCFG_Type	timer_config;
	TIM_MATCHCFG_Type	timer_match;

	uint8_t reset_array[4] = {DISABLE, DISABLE, DISABLE, ENABLE};
	uint32_t match_array[4]= {49, 99, 149, 199};

	timer_config.PrescaleOption = TIM_PRESCALE_USVAL;
	timer_config.PrescaleValue = 100;

	timer_match.IntOnMatch = ENABLE;
	timer_match.StopOnMatch = DISABLE;
	timer_match.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;

	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timer_config);

	for (int i= 0; i< 4; i++) {
		timer_match.MatchChannel = (uint8_t) i;
		timer_match.ResetOnMatch = reset_array[i];
		timer_match.MatchValue = match_array[i];
		TIM_ConfigMatch(LPC_TIM0, &timer_match);
	}
	TIM_Cmd(LPC_TIM0, ENABLE);
	TIM_ResetCounter(LPC_TIM0);
	NVIC_EnableIRQ(TIMER0_IRQn);
}

void TIMER0_IRQHandler (void) {
	if(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)) {
		//estado 1
		current_state = (eint_flag)? state_0: state_4;
		TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
	}

	if(TIM_GetIntStatus(LPC_TIM0, TIM_MR1_INT)) {
		//estado 2
		current_state = (eint_flag)? state_1: state_5;
		TIM_ClearIntPending(LPC_TIM0, TIM_MR1_INT);
	}

	if(TIM_GetIntStatus(LPC_TIM0, TIM_MR2_INT)) {
		//estado 3
		current_state = (eint_flag)? state_2: state_6;
		TIM_ClearIntPending(LPC_TIM0, TIM_MR2_INT);
	}

	if(TIM_GetIntStatus(LPC_TIM0, TIM_MR3_INT)) {
		//estado 4
		current_state = (eint_flag)? state_3: state_7;
		TIM_ClearIntPending(LPC_TIM0, TIM_MR3_INT);
	}
}

void EINT3_IRQHandler (void) {
	eint_flag ^= 1;
	EXTI_ClearEXTIFlag(EXTI_EINT3);
}

