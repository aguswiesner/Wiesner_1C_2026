/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
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

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"


struct leds
{
    uint8_t mode;        // ON, OFF, TOGGLE
	uint8_t n_led;      // indica el número de led a controlar
	uint8_t n_ciclos;   // indica la cantidad de ciclos de encendido/apagado
	uint16_t periodo;   // indica el tiempo de cada ciclo
} my_leds;

enum {ON, OFF, TOGGLE}; // definimos una enumeración para los modos de operación de los leds

void maneja_leds (struct leds *leds) // recibe un puntero a la estructura para poder modificar sus valores
{
	switch(leds->mode){ // evaluamos el modo de operación
		case ON:
			LedOn(leds->n_led); // encendemos el led indicado por n_led
		break;
		case OFF:
			LedOff(leds->n_led); // apagamos el led indicado por n_led
		break;
		case TOGGLE:
			for(int i = 0; i < leds->n_ciclos; i++){ // repetimos el ciclo de encendido/apagado la cantidad de veces indicada por n_ciclos
				LedToggle(leds->n_led); // alternamos el estado del led indicado por n_led
				vTaskDelay(leds->periodo / portTICK_PERIOD_MS); // esperamos el tiempo indicado por periodo
			}
		break;
	}
}	

void app_main(void){
	LedsInit();
	my_leds.mode = TOGGLE; 
	my_leds.n_led = LED_1; 
	my_leds.n_ciclos = 5; 
	my_leds.periodo = 500;

	maneja_leds(&my_leds); //Ejecuta el switch basado en el modo configurado.
}		