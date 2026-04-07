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
 * @author Wiesner Agustina (agustina.wiesner@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <gpio_mcu.h>
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

// Estructura para configuración de GPIO

typedef struct
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;

// Vector que mapea los 4 bits del BCD a los pines GPIO
// b0 -> GPIO_20
// b1 -> GPIO_21
// b2 -> GPIO_22
// b3 -> GPIO_23

static gpioConf_t bcd_gpio_map[4] = {
	{GPIO_20, GPIO_OUTPUT},
	{GPIO_21, GPIO_OUTPUT},
	{GPIO_22, GPIO_OUTPUT},
	{GPIO_23, GPIO_OUTPUT}
};

/*==================[internal functions declaration]=========================*/

// Convierte un dígito BCD y setea los GPIOs correspondientes
// Usa multiplicación lógica (AND) para extraer cada bit del BCD
// y setea el estado del GPIO según el valor del bit (0 o 1)
// Parámetros:
//   bcd_digit: Dígito BCD (0-15)
//   gpio_conf: Vector de estructuras gpioConf_t con la configuración de GPIOs

void bcd_to_gpio(uint8_t bcd_digit, gpioConf_t *gpio_conf)
{
	// Itera sobre los 4 bits del BCD

	for (uint8_t i = 0; i < 4; i++)
	{
		// Extrae el bit i usando multiplicación lógica (AND)
		// Desplaza el número BCD 'i' posiciones a la derecha y aplica AND con 0x01
		
		uint8_t bit_value = (bcd_digit >> i) & 0x01;
		
		// Setea el GPIO con el valor del bit extraído
		GPIOState(gpio_conf[i].pin, bit_value);
	}
}

/*==================[external functions definition]==========================*/
void app_main(void){
    // Ejemplo de uso: convertir el dígito BCD 5 (binario 0101) a GPIOs
    uint8_t bcdDigit = 5;  // 5 en BCD: bits b0=1, b1=0, b2=1, b3=0
    bcd_to_gpio(bcdDigit, bcd_gpio_map);
    printf("BCD %d configurado en GPIOs.\n", bcdDigit);
}
/*==================[end of file]============================================*/