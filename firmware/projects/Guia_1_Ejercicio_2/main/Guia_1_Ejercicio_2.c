/*! @mainpage Blinking switch
 *
 * \section genDesc General Description
 *
 * This example makes LED_1 and LED_2 blink if SWITCH_1 or SWITCH_2 are pressed.
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Agustina Wiesner (agustina.wiesner@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD 100
/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/
void app_main(void){
	uint8_t teclas;
	LedsInit();
	SwitchesInit();
    while(1)    {
    	teclas  = SwitchesRead();
    	switch(teclas){
    		case SWITCH_1:
    			LedOn(LED_1);
        		vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
        		LedOff(LED_1);
        		vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    		break;
    		case SWITCH_2:
    			LedOn(LED_2);
        		vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
        		LedOff(LED_2);
        		vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
    		break;
			case (SWITCH_1 | SWITCH_2): // utilizamos | (or) porque queremos detectar cuando ambas teclas están presionadas 
										// (ambas están a 1) por lo que el resultado de la operación será distinto a 0 
				LedOn(LED_3);
        		vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
        		LedOff(LED_3);
        		vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
			break;
    	}
	}
}
