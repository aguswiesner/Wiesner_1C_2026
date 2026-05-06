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

//Inclusión de librerías estándar de C para manejo de tipos de datos de ancho fijo (como uint8_t) y booleanos.
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//Drivers de Dispositivo
#include "led.h"
#include "switch.h"
/*==================[macros and definitions]=================================*/
//Constante del tiempo de parpadeo de los leds en ms
#define CONFIG_BLINK_PERIOD 100 
/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/
void app_main(void){ //función que no retorna y no recibe parámetros, es el punto de entrada del programa
	uint8_t teclas; 
	LedsInit();
	SwitchesInit();
    while(1)    {
    	teclas  = SwitchesRead();
    	switch(teclas){ // se evalúa el valor de teclas para determinar qué acción tomar con los case
    		case SWITCH_1:
    			LedOn(LED_1);
        		vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS); // convierte el tiempo de ms a ticks que se deben contar para lograr el tiempo deseado de parpadeo
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
										// ya que SWITCH_1 = 00000001 y SWITCH_2 = 00000010, entonces al hacer SWITCH_1 | SWITCH_2 obtenemos 00000011
				LedOn(LED_3);
        		vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
        		LedOff(LED_3);
        		vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
			break;
    	}
	}
}
