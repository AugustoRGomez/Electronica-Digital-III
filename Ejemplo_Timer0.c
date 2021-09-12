#include <lpc17xx.h>

#define PRESCALE (25000-1) //25000 PCLK clock cycles to increment TC by 1

void initTimer0();

int main(void)
{
SystemInit();
LPC_GPIO1->FIODIR |= (1<<29); //Configura P1.29 como salida
initTimer0();

while(1)
{

}

}


void initTimer0(void)
{

LPC_SC->PCONP |= (1<<1); //Power up TIM0. By default TIM0 and TIM1 are enabled.
LPC_SC->PCLKSEL0 &= ~(0x3<<3); //Set PCLK for timer = CCLK/4 = 100/4 (default)  

LPC_TIM0->CTCR = 0x0;
LPC_TIM0->PR = PRESCALE;
LPC_TIM0->MR0 = 500;
LPC_TIM0->MCR |= (1<<0) | (1<<1); // Interrupt & Reset on MR0 match
LPC_TIM0->TCR |= (1<<1); //Reset Timer0

NVIC_EnableIRQ(TIMER0_IRQn);

LPC_TIM0->TCR = 0x01; //Enable timer
}

extern "C" void TIMER0_IRQHandler(void) //puse extern c por c++ linker.
{
LPC_TIM0->IR |= (1<<0); //Limpio MR0 Interrupt flag
LPC_GPIO1->FIOPIN ^= (1<<29); //Togleo el led.
}
