

/*
===============================================================================
 Name        : MICINTA2021TRANSPOETADORA.c
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


uint32_t *maximo = 0x10003001;
uint32_t *minimo = 0x10003005;

void ConfigTimer0();
void PinConfig();


uint32_t altura[10];
uint32_t lectura;
uint32_t promedio;

int main(void) {
   uint16_t i , j;
   uint16_t sumatorio =0;




	PinConfig();
	ConfigTimer0();
    while(1) {
    	  for(i=0;i<10;i++){
    		sumatorio = sumatorio + altura[i];
    	  }

    	  promedio = sumatorio/10;
    	  if((promedio>*minimo)&(promedio<*maximo)){

    		  LPC_GPIO0->FIOSET|=1<<20;//ENCIENDE EL LED VERDE
    		  LPC_SC->PCONP |= 1<<1;//Enciende el TIMER0

    	  }
    	  else{

    		  LPC_GPIO0->FIOSET|=1<<9;// enciendo el LED ROJO
    	  }

    }
    return 0 ;
}

void ConfigTimer0(){

	LPC_SC->PCONP |= 1<<1;//Enciende el TIMER0
		LPC_SC->PCLKSEL0 &=~(3<<2);// CCLK/4  100/4 = 25mhz
		LPC_PINCON->PINSEL3 |= 3<<26;//FINCION 3 COMO MATCH0.0

		//LPC_TIM0->PR = 10; //PR = 10+1 = 11 , en realidad estoy cargango con 11 al PR
		LPC_TIM0->PR =9;//PR = 9+1 = 10
		LPC_TIM0->EMR  |= (3<<6);//EXTERNAL MATCH COMO toogle
		 LPC_TIM0->MCR |= 1<<3;//Habilita la interrupcion por coincidencia con el match0.1
	    LPC_TIM0->MCR |= 1<<4;//se resetea el Timer Counter cuando el natch0.1 coincide con el tc

	   // LPC_TIM0->MCR|= (1<<1);//el timer counter se resetea cuando coincide con el match
	   // LPC_TIM0->MR1 = 700000;
	    LPC_TIM0->MR1 = 1000;//para el adc a una tasa de muestreo de 400us o

	   // LPC_TIM0->IR|=1<<0;//limppio la bandera del match0
	   // LPC_TIM0->CR0|= 1<<0;//el prescaler y el timer counter son habiltados para contar
	    LPC_TIM0->TCR = 3;//0b11 h el bit 0 habilita al tc y prescaler para contar y el bit 1  resetea; se resetea para que empieze todo desde cero
	    LPC_TIM0->TCR &= ~(1<<1);// levanto el reset, se restea para borrar cualquier valor que se halla quedado antes


}

void TIMER0_IRQHandler(void){
	 static uint16_t conta =0;//la variable static mantiene su valor cundo sale y entra de la interrupcion
	if((LPC_GPIO2->FIOPIN)&(1<<10)){//PREFUNTA si se presiono la entrada

		lectura = LPC_GPIO0->FIOPIN;//LA VARIABLE lectura recibe el valor que recibe del censor
		altura[conta] = lectura;//arreglo[0]


		conta++;//en cda interrupcion se incrementa en 1
		if (conta==10){
			LPC_SC->PCONP &= ~(1<<1);//APAGO EL TIMER0
			conta = conta%10;// si conta llega a 10 se hace cero
		}
		else{

			LPC_SC->PCONP |= 1<<1;//Enciende el TIMER0
		}


	}

   //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	if(LPC_GPIO0->FIOPIN &(1<<22)){

		LPC_GPIO0->FIOCLR|=1<<22;

	}
	else{

		LPC_GPIO0->FIOSET|=1<<22;
	}


	 LPC_TIM0->IR|=1<<0;//limppio la bandera del match0
	 //LPC_TIM0->IR = 1;


}

void PinConfig(){
	//pines de entradas simple,hay tres tipos de entradas entrada simple entrada, gpioint y EINTX
LPC_PINCON->PINSEL4 &~(3<<20);//FUNCION COMO GPIO P2.10
LPC_GPIO2->FIODIR&=~(1<<10);//P2.10 como entrada SENIAL DE INICIO O PULSADOR EXTERNO
LPC_PINCON->PINMODE4|= 0b11<<20;//como pulldowm es dcir de va va activar con un flanco de subida de 0..1
LPC_GPIO2->FIOMASK = 0XFFFFFFFF &~(1<<10);//ENMASCARO LA ENTRADA P2.10
 //PINES DE SALIDA
LPC_GPIO0->FIODIR|= 1<<20;//P0.20 COMO SALIDA DE LUCES
 //ENTRADA DE LOS PINES DEL SENSOR
LPC_GPIO0->FIODIR|= 1<<9;//LED ROJO
LPC_GPIO0->FIODIR = ~(0XF) ; //P0.0, P0.1, P0.2, P0.3 SE HAbilita como entradas
LPC_GPIO0->FIOMASK = 0XFFFFFFFF &~(1<<0)&~(1<<1)&~(1<<2)&~(1<<3)&~(1<<20);


}



