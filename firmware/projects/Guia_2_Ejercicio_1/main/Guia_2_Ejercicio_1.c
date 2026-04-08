/*! @mainpage Medidor de Distancia por Ultrasonido
 *
 * @section genDesc Descripción General
 *
 * Este programa implementa un medidor de distancia ultrasónico utilizando el sensor HC-SR04.
 * Mide la distancia y la muestra en el LCD, mientras controla los LEDs según rangos de distancia.
 *
 * Rangos de distancia para LEDs:
 * - < 10 cm: Todos los LEDs apagados
 * - 10-20 cm: LED_1 encendido (verde)
 * - 20-30 cm: LED_1 y LED_2 encendidos (verde y amarillo)
 * - > 30 cm: LED_1, LED_2, LED_3 encendidos (verde, amarillo, rojo)
 *
 * Frecuencia de refresco de medición: 1 segundo
 *
 * @section flowchart Diagrama de Flujo
 *
 * ```mermaid
 * flowchart TD
 *     A[Inicio] --> B[Inicializar HC-SR04 GPIO_3 ECHO, GPIO_2 TRIGGER]
 *     B --> C[Inicializar LEDs]
 *     C --> D[Inicializar LCD]
 *     D --> E[Bucle Inicio]
 *     E --> F[Leer Distancia en cm]
 *     F --> G[Mostrar Distancia en LCD]
 *     G --> H{Distancia < 10 cm?}
 *     H -->|Sí| I[Apagar todos los LEDs]
 *     H -->|No| J{Distancia <= 20 cm?}
 *     J -->|Sí| K[Encender LED_1]
 *     J -->|No| L{Distancia <= 30 cm?}
 *     L -->|Sí| M[Encender LED_1 y LED_2]
 *     L -->|No| N[Encender LED_1, LED_2, LED_3]
 *     I --> O[Retardo 1 segundo]
 *     K --> O
 *     M --> O
 *     N --> O
 *     O --> E
 * ```
 *
 * @section hardConn Conexiones de Hardware
 *
 * |    Peripheral  |   EDU-ESP   	|
 * |:--------------:|:--------------|
 * |  HC-SR04 ECHO | 	GPIO_3		|
 * |  HC-SR04 TRIGGER| GPIO_2		|
 * |  HC-SR04 VCC | 	5V		|
 * |  HC-SR04 GND | 	GND		|
 * |  LCD VCC     | 	5V		|
 * |  LCD GND     | 	GND		|
 * |  LCD BCD1    | 	GPIO_20		|
 * |  LCD BCD2    | 	GPIO_21		|
 * |  LCD BCD3    | 	GPIO_22		|
 * |  LCD BCD4    | 	GPIO_23		|
 * |  LCD SEL1    | 	GPIO_19		|
 * |  LCD SEL2    | 	GPIO_18		|
 * |  LCD SEL3    | 	GPIO_9		|
 *
 * @author Agustina Wiesner (agustina.wiesner@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "hc_sr04.h"
#include "led.h"
#include "lcditse0803.h"
#include "delay_mcu.h"
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/*==================[internal functions declaration]=========================*/

/*==================[external functions definition]==========================*/
void app_main(void){
    // Inicializar sensor HC-SR04
    HcSr04Init(GPIO_3, GPIO_2);  // ECHO en GPIO_3, TRIGGER en GPIO_2

    // Inicializar LEDs
    LedsInit();

    // Inicializar LCD
    LcdItsE0803Init();

    uint16_t distancia;

    while(1){
        // Leer distancia en centímetros
        distancia = HcSr04ReadDistanceInCentimeters();

        // Mostrar distancia en LCD
        LcdItsE0803Write(distancia);

        // Controlar LEDs según la distancia
        if(distancia < 10){
            // Apagar todos los LEDs
            LedsOffAll();
        } else if(distancia <= 20){
            // Encender LED_1 (verde)
            LedsOffAll();
            LedOn(LED_1);
        } else if(distancia <= 30){
            // Encender LED_1 y LED_2 (verde y amarillo)
            LedsOffAll();
            LedOn(LED_1);
            LedOn(LED_2);
        } else {
            // Encender LED_1, LED_2, LED_3 (verde, amarillo, rojo)
            LedsOffAll();
            LedOn(LED_1);
            LedOn(LED_2);
            LedOn(LED_3);
        }

        // Retardo de 1 segundo
        DelayMs(1000);
    }
}
/*==================[end of file]============================================*/