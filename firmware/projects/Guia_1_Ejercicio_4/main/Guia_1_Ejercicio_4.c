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
#include <stdbool.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/


int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number)
{
    if (bcd_number == NULL || digits == 0) {
        return -1; // parámetros inválidos
    }

    // Llenar con ceros por si data tiene menos dígitos que digits
    for (uint8_t i = 0; i < digits; i++) {
        bcd_number[i] = 0;
    }

    // Se llena de derecha a izquierda (digit menos significativo primero)
    for (int i = digits - 1; i >= 0; i--) {
        bcd_number[i] = data % 10;
        data /= 10;
        if (data == 0) {
            break;
        }
    }

    // Si queda data > 0, entonces no entró en el arreglo (overflow de dígitos)
    if (data != 0) {
        return -2;
    }

    return 0; // éxito
}

//==================[external functions definition]==========================//

void app_main(void){
    uint8_t bcd_array[5]; // arreglo para almacenar hasta 5 dígitos BCD

    // Ejemplo: convertir 12345 a BCD con 5 dígitos
    int8_t result = convertToBcdArray(12345, 5, bcd_array);

    if(result == 0){
        printf("BCD convertido exitosamente: ");
        for(uint8_t i = 0; i < 5; i++){
            printf("%d ", bcd_array[i]);
        }
        printf("\n");
    } else {
        printf("Error en la conversión: %d\n", result);
    }
}

//==================[end of file]============================================//