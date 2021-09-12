
/*
===============================================================================
 Name        Systick PWM
 Author      : $(author) Gomez - Patiño
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================

Utilizando Systick Timer con interrupción_1
1.- Configure el Systick Timer de modo que genere una forma de onda llamada PWM tal como la que se muestra
en la figura adjunta.
Esta señal debe ser sacada por el pin P0.22 para que controle la intensidad de brillo del led.
El periodo de la señal debe ser de 10 mseg con un duty cycle de 10%.
Configure la interrupción externa EINT0 de modo que cada vez que se entre en una rutina de interrupción
externa el duty cycle incremente en un 10% (1 mseg).
Esto se repite hasta llegar al 100%, luego, si se entra nuevamente a la interrupción externa,
el duty cycle volverá al 10%.
2.- Modificar los niveles de prioridad para que la interrupción por systick tenga mayor prioridad que
la interrupción externa.
3. Adjuntar el .C  del código generado.
4. Le pongamos un reset co interupcion externa

PINES DE SALIDA
P0.22 (GPIO)
PINES DE ENTRADA (INT GPIO)
P0.0 (GPIO)
P2.10(INT EXTERNA)
SYSTICK
Cada 1ms
PWM 10% 20% .... 90% 10%

*/

#include "LPC17xx.h"

void confGPIO(void);
void confIntExt(void);
void confIntGPIO(void);
void confSystick(void);
uint8_t contador = 0;
const uint8_t cont_final = 10;
uint8_t cont_duty= 1;

int main(void){

    confGPIO();
    confIntExt();
    confIntGPIO();
    confSystick();

    while(1){

        while (contador < cont_duty)
        {
            LPC_GPIO2->FIOSET |= (1<<0);
        }

        LPC_GPIO2->FIOCLR |= (1<<0);

        if(contador == cont_final)
        {
            contador = 0;
        }
}
    return 0;
}
void confGPIO(void){
    //P0.22
    //LPC_PINCON -> PINSEL1 &= ~(0b11 << 12);  // PINSEL1 13:12
    LPC_GPIO2 -> FIODIR |= (1<<0); //SALIDA
    LPC_PINCON -> PINMODE4 &= ~(0b11 << 0); //PINMODE1 1:0 pullup
    LPC_GPIO2->FIOCLR |= (1<<0);
    //P0.0
    LPC_PINCON -> PINSEL1 &= ~(0b11 << 0);  // PINSEL0 1:0
    LPC_GPIO0 -> FIODIR &= ~(1<<0); //ENTRADA
    LPC_PINCON -> PINMODE0 &= ~(0b11 << 0); //PINMODE0 1:0 pullup
}

void confIntGPIO(void){
   LPC_GPIOINT -> IO0IntEnF |= (1 << 0); //Selecciono la interrupcion por flanco de bajada
   LPC_GPIOINT -> IO0IntClr |= (1 << 0); //Limpia la bandera
   NVIC_EnableIRQ(EINT3_IRQn); //Habilita las interrupciones por GPIO
   NVIC_SetPriority(EINT3_IRQn, 2);
}

/*
 * Configuracion de una interrupcion externa.
 */
void confIntExt(void){
   //P2.10 (funcion de EINT)
    LPC_PINCON -> PINSEL4 |= (1 << 20);  // PINSEL4 21:20  (0 1)
    LPC_SC -> EXTMODE |= 1; //Selecciona interrupcion por flanco (tabla 11 pag 28)
    LPC_SC -> EXTPOLAR |= 1; //Interrumpe cuando el flanco es de subida (tabla 12 pag 28)
    LPC_SC -> EXTINT |= 1; //Limpia la bandera de interrupciones externas.
    NVIC_EnableIRQ(EINT0_IRQn); //Habilita las interrupciones externas.
    NVIC_SetPriority(EINT0_IRQn, 1);
}

/*
 *Ojo funciona con clock de 100 Mhz
 *Interrumpir cada 1ms
 *Formula PERIODO_INT= (RECARGA+1)*PERIODO_CLK
          RECARGA= (PERIODO_INT/PERIODO_CLK)-1
 */
void confSystick(void){
    SysTick -> CTRL = 0; // Deshabilito el systick
    SysTick -> LOAD = 99999UL; //
    NVIC_SetPriority(SysTick_IRQn, 3); // Setea prioridades, por defecto esta en -1
    SysTick ->VAL = 0; //Limpia el valor actual
    SysTick ->CTRL = 0x00000007; // Enable = 1 TickInt = 1 Clock Source = 1
    //NVIC_EnableIRQ(SysTick_IRQn); // Habilita la interrupcion (no funciona)
}


/*
 * Manejo de la rutina de interrupcion
 * Interrupciones de GPIO
 * P[0]0
 */
void EINT3_IRQHandler(void){


   if((LPC_GPIOINT->IO0IntEnF) & (1<<0)){
	   if(cont_duty == 9){
		   cont_duty = 1;
	   }
	   else
	   {
		   cont_duty ++;
	   }
   }
   LPC_GPIOINT->IO0IntClr |= (1<<0);
}

/*
 * Manejo de la rutina de interrupcion
 * Interrupciones Externas
 * P[2]10
 */
void EINT0_IRQHandler(void){
	cont_duty = 1;
	LPC_SC ->EXTINT |= 1;// limpio la bandera de interrupcion
}

void SysTick_Handler(){
    contador ++;
}
