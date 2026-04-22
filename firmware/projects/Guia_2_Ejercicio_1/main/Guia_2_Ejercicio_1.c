/*! @mainpage Medidor de Distancia por Ultrasonido
 *
 * @section genDesc Descripción General
 *
 * Este programa implementa un medidor de distancia ultrasónico utilizando el sensor HC-SR04.
 * Mide la distancia y la muestra en el LCD, mientras controla los LEDs según rangos de distancia.
 * Utiliza tareas de FreeRTOS para manejar los switches TEC1 y TEC2.
 *
 * Rangos de distancia para LEDs:
 * - < 10 cm: Todos los LEDs apagados
 * - 10-20 cm: LED_1 encendido (verde)
 * - 20-30 cm: LED_1 y LED_2 encendidos (verde y amarillo)
 * - > 30 cm: LED_1, LED_2, LED_3 encendidos (verde, amarillo, rojo)
 *
 * Funcionalidades:
 * - Usar TEC1 (SWITCH_1) para activar/detener medición (variable MEDIR).
 * - Usar TEC2 (SWITCH_2) para mantener el resultado (variable HOLD).
 * - Refresco de medición: 1 segundo.
 * - Delay de tareas de switches: 20 ms.
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
 * |  SWITCH_1 (TEC1) | GPIO_4		|
 * |  SWITCH_2 (TEC2) | GPIO_15		|
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
#include "hc_sr04.h"
#include "led.h"
#include "lcditse0803.h"
#include "switch.h"
#include "delay_mcu.h"
/*==================[macros and definitions]=================================*/
#define TASK_DELAY_SWITCH 20  // 20 ms
#define TASK_DELAY_MEDIR 1000  // 1 s
/*==================[internal data definition]===============================*/
TaskHandle_t medir_task_handle = NULL;
TaskHandle_t teclas_task_handle = NULL;
bool MEDIR = false; 
bool HOLD = false;
uint16_t distancia_actual = 0;
uint16_t distancia_mostrada = 0;
/*==================[internal functions declaration]=========================*/
static void TeclasTask(void *pvParameter){
    int8_t switch_state;
    int8_t switch_prev = 0;
    while(true){
        switch_state = SwitchesRead();
        
        // Operador lógico AND ( && ) verifica si todos los operandos son verdaderos 
        //y devuelve verdadero solo si ambos operandos son verdaderos.

        // Detectar presión de SWITCH_1 (TEC1) 
        if ((switch_state & SWITCH_1) && !(switch_prev & SWITCH_1)) { // Transición de no presionado a presionado
            MEDIR = !MEDIR;
        }
        
        // Detectar presión de SWITCH_2 (TEC2)
        if ((switch_state & SWITCH_2) && !(switch_prev & SWITCH_2)) {
            HOLD = !HOLD;
        }
        
        switch_prev = switch_state;
        vTaskDelay(TASK_DELAY_SWITCH / portTICK_PERIOD_MS);
    }
}

static void MedirTask(void *pvParameter){
    while(true){
        if (MEDIR) {
            distancia_actual = HcSr04ReadDistanceInCentimeters();
            if (!HOLD) {
                distancia_mostrada = distancia_actual;
            }
            
            // Mostrar distancia en LCD (congelada si HOLD está activado)
            LcdItsE0803Write(distancia_mostrada);

            // Controlar LEDs según la distancia actual (siempre en tiempo real)
            if(distancia_actual < 10){
                // Apagar todos los LEDs
                LedsOffAll();
            } else if(distancia_actual <= 20){
                // Encender LED_1 (verde)
                LedsOffAll();
                LedOn(LED_1);
            } else if(distancia_actual <= 30){
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
        } else {
            // Apagar LEDs y display cuando MEDIR está desactivado (TEC1 presionado)
            LedsOffAll();
            LcdItsE0803Off();
        }
        vTaskDelay(TASK_DELAY_MEDIR / portTICK_PERIOD_MS);
    }
}
/*==================[external functions definition]==========================*/
void app_main(void){
    // Inicializar sensor HC-SR04
    HcSr04Init(GPIO_3, GPIO_2);  // ECHO en GPIO_3, TRIGGER en GPIO_2

    // Inicializar LEDs
    LedsInit();

    // Inicializar LCD
    LcdItsE0803Init();

    // Inicializar Switches
    SwitchesInit();

    // Crear tareas 
    xTaskCreate(&TeclasTask, "TECLAS", 512, NULL, 5, &teclas_task_handle);
    xTaskCreate(&MedirTask, "MEDIR", 512, NULL, 5, &medir_task_handle);
}
/*==================[end of file]============================================*/