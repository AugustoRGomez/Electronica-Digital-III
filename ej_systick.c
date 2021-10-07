/*
===============================================================================
 Name        : ejem_sysTick.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>
// TODO: Define leds
#define LED_RED 		22 	//gpio0 --> pinsel1[13:12]
#define LED_BLUE 		26	//gpio3 --> pinsel7[19:18]
#define LED_GREEN		25	//gpio3 --> pinsel7[21:20]
#define PINSEL_RED		12
#define PINSEL_BLUE 	18
#define PINSEL_GREEN 	20


//TODO: define systick
#define SYSTICK_BASE		0xE000 E010 					//SysTick Base Address
#define SYSTICK 			((mySystick*) SysTick_BASE)
//bits of config systick
#define SYS_ENABLE			0 //1 counter enable, 	0 counter disable
#define	SYS_TICKINT			1 //1 interrup enable, 	0 interrup disable
#define SYS_CLKSOURCE		2 //1 cpu clock select, 0 external clock select
#define SYS_COUNTFLAG 		16 // flag is set when the system Tick counter counts down to 0
#define SYS_COUNTFLAG_MASK 	(1<<SYS_COUNTFLAG)

uint32_t calc_load_systick(uint32_t freq);
void setGpio(void);

typedef struct
{
	volatile uint32_t STCTRL;		//System Timer Control and status register
	volatile uint32_t STRELOAD;		//System Timer Reload value register
	volatile uint32_t STCURR;		//System Timer Current value register
	volatile uint32_t STCALIB;		//System Timer Calibration value register
}mySystick;


int main(void) {
	SystemInit();
	setGpio();
	SYSTICK->STRELOAD 	= calc_load_systick(100); //max 16.77ms
	SYSTICK->STCTRL  	|= (1<<SYS_ENABLE) | (1<<SYS_TICKINT) | (1<<SYS_CLKSOURCE);
	SYSTICK->STCURR 	=	0; //limpio el contodar
 	while(1){

	}
    return 0 ;
}

/*
 * Calcula la cantidad de tick a cargar en el reload del systick
 * @param frequency frecuencia a la cual interrumpte el systick
 * 					ej: 10ms --> 1/10ms = 100 [Hz]
 * */
uint32_t calc_load_systick(uint32_t freq){
	uint32_t load = (SystemCoreClock / freq) - 1;
	return load;
}

void setGpio(void){
	LPC_PINCON->PINSEL1 	&= ~(1<<PINSEL_RED);
	LPC_PINCON->PINMODE1 	&= ~(1<<PINSEL_RED);
	LPC_PINCON->PINMODE1 	|= (1<<(PINSEL_RED+1));

	LPC_PINCON->PINSEL7 	&= ~(1<<PINSEL_BLUE);
	LPC_PINCON->PINMODE7 	&= ~(1<<PINSEL_BLUE);
	LPC_PINCON->PINMODE7 	|= (1<<(PINSEL_BLUE+1));

	LPC_PINCON->PINSEL7 	&= ~(1<<PINSEL_GREEN);
	LPC_PINCON->PINMODE7 	&= ~(1<<PINSEL_GREEN);
	LPC_PINCON->PINMODE7 	|= (1<<(PINSEL_GREEN+1));

	LPC_GPIO0->FIODIR 		|= (1<<LED_RED);
	LPC_GPIO3->FIODIR 		|= (1<<LED_BLUE);
	LPC_GPIO3->FIODIR 		|= (1<<LED_GREEN);

}
void SysTick_Handler(void){
	uint32_t pinState0,pinState3;

	pinState0 	= LPC_GPIO0->FIOPIN;
	pinState3 	= LPC_GPIO3->FIOPIN;

	LPC_GPIO0->FIOCLR = (pinState0 & (1 << LED_RED)) ;
	LPC_GPIO0->FIOSET = ((~pinState0) & (1 << LED_RED));

	LPC_GPIO3->FIOCLR = (pinState3 & (1 << LED_BLUE)) ;
	LPC_GPIO3->FIOSET = ((~pinState3) & (1 << LED_BLUE));

	LPC_GPIO3->FIOCLR = (pinState3 & (1 << LED_GREEN)) ;
	LPC_GPIO3->FIOSET = ((~pinState3) & (1 << LED_GREEN));
}