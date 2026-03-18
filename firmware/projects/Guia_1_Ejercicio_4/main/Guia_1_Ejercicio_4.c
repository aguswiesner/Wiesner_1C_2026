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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/


int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t * bcd_number)
{
    if (bcd_number == NULL || digits == 0) {
        return -1;
    }

    // Convierte el numero a BCD desde el dígito menos significativo al más significativo
    for (int i = digits - 1; i >= 0; --i) { 
        bcd_number[i] = (uint8_t)(data % 10u); // Extraemos el dígito menos significativo
        data /= 10u;
    }

    /* If there is remaining data, it doesn't fit in the requested digit count. */
    if (data != 0u) {
        return -1;
    }

    return 0;
}

void app_main(void)
{
    uint32_t mi_dato = 12345;
    uint8_t cantidad_digitos = 5;
    uint8_t mi_bcd[5]; // Arreglo donde se guardará el resultado

    // Llamamos a la función
    int8_t resultado = convertToBcdArray(mi_dato, cantidad_digitos, mi_bcd);

    if (resultado == 0) {
        printf("Valor %u en BCD: ", mi_dato);
        for (uint8_t i = 0; i < cantidad_digitos; ++i) {
            printf("%u", mi_bcd[i]);
        }
        printf("\n");
    } else {
        printf("No entró el valor %u en %u dígitos\n", mi_dato, cantidad_digitos);
    }

    /* Mantener el programa corriendo (FreeRTOS) */
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/*==================[end of file]============================================*/