/*
===============================================================================
 Name        : ED3_parcial_ejemplo.c
 Author      : $(author) august
 Version     :
 Copyright   : $(copyright)
 Description :

	 En una cinta transportadora como la que se puede observar en la ilustración 1, se
	encuentra un sensor que nos permite medir la altura de las piezas que pasan por
	ella. En el momento en que una pieza esté correctamente posicionada debajo del
	sensor la cinta se detiene y nos lo hace saber mediante un pulso de 0.1 us
	duración, conectado a la placa LPcxpr1769 a través del Pin P2[10]. El Pin P0[20]
	controla el avance y detención de la cinta de tal modo que un '1' lógico hace
	avanzar la cinta y un '0' lógico la detiene. El sensor de altura está conectado a la
	cinta transportadora y a la placa a través de los pines P0[0], P0[1], P0[2], P0[3],
	de modo que nos permite diferenciar 16 niveles, que se corresponden con alturas que
	van desde 0 mm hasta 16 milímetros.
	En la dirección 0x10003001 se encuentra el valor del límite superior permitido de medición,
	variable esta de tipo  entera expresada en milímetros. Y en la dirección 0x10003005 se encuentra
	el valor de limite inferior permitido de medición, también una variable de tipo entera expresada
	en milímetros.
	 Debido a que el sensor de distancia tiene su respectiva respuesta temporal, es necesario esperar
	un tiempo no menor a 400 us desde que la cinta está detenida hasta que se comience a realizar las
	mediciones (Utilice TIMERO, considere dock de periférico de 25 Mhz).
	 Se pide el software correspondiente debidamente comentado, utilizando lenguaje C o libredas CMSIS, que
	permita:

	1- Cada vez que una pieza se posiciona debajo del sensor se deberá esperar el tiempo indicado, realiza'
	mediciones de altura y almacenarlas en un arreglo de 10 elementos.
	2- Se deberá comparar el valor del promedio obtenido de las mediciones con los límites establecidos, de
	manera tal que, si la lectura obtenida satisface los límites se deberá colocar un estado alto en el pin P0[20]
	para indicarle a la cinta que avance hasta la próxima pieza a medir, caso contrario se encenderá un led rojo
	para indicar una falla.

===============================================================================
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include <cr_section_macros.h>

const uint8_t mask= 0x0F;
int lectura;
uint8_t flag_falla=0;
uint8_t flag_calc_promedio=0;
uint8_t flag_lectura_lista=0;

int main(void) {

	//Conf de GPIO
	//*Se asume que no es necesario configurar registros PINSELx porque por defecto estan en función "GPIO"*
	//*PINMODE se asume pullUp, configurado por defecto*
	LPC_PINCON->PINMODE0 |= (1<<21); //pin 10 no tiene pullUp ni pullDown
	LPC_GPIO0->FIODIR |= (1<<22)|(1<<20); //pin 20 y 22 (LED rojo) son salidas

	LPC_GPIOINT->IO0IntEnR |= (1<<10); //pin 10 interrumpe por flanco de subida
	LPC_GPIOINT->IO0IntClr |= (1<<10);
	NVIC_EnableIRQ(EINT3_IRQn);

	//Conf de SYSTICK
	//*Se asume un CCLK de 25MHz*
	SysTick->CTRL= 0;
	SysTick->LOAD= 9999; //valor para interrupir cada 400us
	NVIC_SetPriority(SysTick_IRQn, 3);
	SysTick->VAL= 0;
	SysTick->CTRL= 0b110; //todavia no lo enciendo

	//RUTINA
	int promedio,i;
	int arreglo[10];
	int *punt= arreglo;
	int *punt_limite_mayor= 0x10003001;
	int *punt_limite_menor= 0x10003005;

	while(1) {

		if(flag_lectura_lista && !flag_falla) {
			*punt= lectura;
			flag_lectura_lista= 0;
			if(punt== &arreglo[9]) {
				flag_calc_promedio= 1; //listo para calcular promedio
				punt= arreglo; //vuelvo a apuntar a primer posicion de array
			}
			else {
				LPC_GPIO0->FIOSET |= (1<<20); //la cinta avanza para otra medicion
				punt++;
			}
		}

		if(flag_calc_promedio) {
			promedio=0;
			for(i=0; i<10; i++) {
				promedio += arreglo[i];
			}
			promedio = promedio/10;

			if((promedio <= *punt_limite_mayor) && (promedio >= *punt_limite_menor)) {
				//to joia
				LPC_GPIO0->FIOSET |= (1<<20); //la cinta vuelve a avanzar
				flag_calc_promedio=0;
			}
			else {
				//ocurrio una falla en la medicion
				LPC_GPIO0->FIOCLR |= (1<<22); //led rojo en ON (funciona con logica negativa, en modelo con RGB)
				flag_falla= 1;
			}
		}
	}

    return 0 ;
}

void EINT3_IRQHandler(void) {
	//Sensor detecto un objeto en posición, dispara el timer
	LPC_GPIO0->FIOCLR |= (1<<20); //detengo la cinta
	SysTick->VAL= 0;
	SysTick->CTRL |= (1<<0); //largo timer
	LPC_GPIOINT->IO0IntClr = (1<<10);
}

void SysTick_Handler(void) {
	//Paso el tiempo necesario para que el sensor pueda medir
	SysTick->CTRL &= ~(1<<0); //apago el timer
	lectura= (LPC_GPIO0->FIOPIN0) & (mask); //me interesan solo los 4 LSB
	flag_lectura_lista= 1;
}

