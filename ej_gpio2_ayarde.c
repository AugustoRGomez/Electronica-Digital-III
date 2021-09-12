/*
===============================================================================
 Name        : ejerGpio_2.c
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
void sort(int *pStart,int *pEnd,int newData);

uint8_t flag_reading=0;
int main(void) {
	SystemInit();
	//set gpio 0
	LPC_PINCON->PINSEL0 &= ~(0xFFFFFFFF);
	LPC_PINCON->PINSEL1 &= ~(0x0000001F);
	LPC_GPIO0->FIODIR	|= (0x0003FFFF); 	//output
	//set gpio 1
	LPC_PINCON->PINSEL2 &= ~(0xFFFFFFFF); 	//
	LPC_PINCON->PINSEL3 &= ~(0x00007FFF);
	LPC_GPIO1->FIODIR	&= ~(0xFFC713); 	//input
	LPC_GPIO1->FIOMASK	|= ~(0xFFC713);		//
	//systick
	SysTick->CTRL 		|= (1<<0) | (1<<1) | (1<<2);
	SysTick->LOAD 		= (SystemCoreClock / 100) - 1;
	SysTick->VAL 		=0;

	//variables
	int32_t buffer[8];
	uint8_t count=0;
	uint8_t length_buffer = sizeof(buffer)/sizeof(buffer[0]);
	uint8_t start=0;
	while(1){
	if(flag_reading){
		int32_t data 		= 	 (uint32_t)((0x03&LPC_GPIO1->FIOPIN0))
									|(uint32_t)((0x10&LPC_GPIO1->FIOPIN0)>>2)
									|(uint32_t)((0x07&LPC_GPIO1->FIOPIN1)<<3)
									|(uint32_t)((0xC0&LPC_GPIO1->FIOPIN1)<<0)
									|(uint32_t)((0xFF&LPC_GPIO1->FIOPIN2)<<8);
		if(!start){
			sort(buffer,&buffer[7],data);
			count++;
			if(count==8){
				int32_t avg	= average(buffer,length_buffer);
				start=1;
				LPC_GPIO0->FIOPIN =  (uint32_t)(0x0FFF&avg)
									|(uint32_t)((0xF000&avg)<<3);
				}
			}
			else {
				sort(buffer,&buffer[7],data);
				int32_t avg	= average(buffer,length_buffer);
				LPC_GPIO0->FIOPIN =  (uint32_t)(0x0FFF&avg)
									|(uint32_t)((0xF000&avg)<<3);
			}
		flag_reading = 0;
		}


	}
    return 0 ;
}

/* buffer circular donde le ultimo dato se pierde
 * funcion armada por augusto crack*/
void sort(int *pStart,int *pEnd,int newData) {
    int *p1=pEnd;
    int *p2=pEnd-1;

     while (p2 != pStart) {
        *p1= *p2;
         p1= p2;
         p2--;
    }

    *p1= *p2;
    *p2= newData;
}
int32_t average(uint32_t* p_buffer,uint32_t length){
	uint32_t avg = 0;
	for (uint8_t i = 0;i < length ; i++){
		avg += (*p_buffer);
		p_buffer++;
	}
	avg = avg / length;
}

void SysTick_Handler(){
	flag_reading = 1;
}

/*
 * mio!!
 * int main() {
    int buffer[8] = {10,0,0,0,0,0,0,0};
    int length_buffer = sizeof(buffer)/sizeof(buffer[0]);
    int count = 0;
    while(count<10){
        for (int i=0;i<length_buffer-1;i++){
		    printf("%d\n",buffer[i]);
		}
        printf("---------------\n");
        for (int i=length_buffer-1;i>0;i--){
            buffer[i] = buffer[i-1];
		}
        buffer[0] =count+1;
        count++;
    }
    return 0;
 * */