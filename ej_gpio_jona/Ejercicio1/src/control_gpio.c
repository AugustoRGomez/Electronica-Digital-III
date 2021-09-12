/*
 * control_gpio.c
 *
 *  Created on: 24 ago. 2021
 *      Author: Administrador
 */
/*
 * Esta funcion
 */
void conf_pin(uint8_t portNum,  uint32_t bitValue,uint8_t dir){

	   LPC_GPIO_TypeDef *pGPIO = GPIO_GetPointer(portNum);

	    if (pGPIO != NULL) {
	        // Enable Output
	        if (dir) {
	            pGPIO->FIODIR |= bitValue;
	        }
	        // Enable Input
	        else {
	            pGPIO->FIODIR &= ~bitValue;
	        }
	    }
}
