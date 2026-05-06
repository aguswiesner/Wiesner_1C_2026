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
 * - Usar TEC2 (SWITCH_2) para mantener el resultado en el display (variable HOLD).
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
#include "hc_sr04.h" //Driver para el sensor de distancia ultrasónico.
#include "led.h"
#include "lcditse0803.h" //Control para el display LCD de 7 segmentos.
#include "switch.h"
#include "delay_mcu.h" //Funciones para generar retardos de tiempo.
/*==================[macros and definitions]=================================*/
#define TASK_DELAY_SWITCH 20  // 20 ms
#define TASK_DELAY_MEDIR 1000  // 1 s
/*==================[internal data definition]===============================*/
//
TaskHandle_t medir_task_handle = NULL;
TaskHandle_t teclas_task_handle = NULL;
bool MEDIR = false; 
bool HOLD = false;
uint16_t distancia_actual = 0;
uint16_t distancia_mostrada = 0;
/*==================[internal functions declaration]=========================*/

/**
 * @brief Tarea que maneja la detección de pulsaciones en los switches TEC1 y TEC2.
 *
 * Esta tarea lee el estado de los switches en un bucle infinito, detecta transiciones
 * de no presionado a presionado para TEC1 y TEC2, y actualiza las variables globales
 * MEDIR y HOLD en consecuencia. Utiliza un delay de 20 ms entre lecturas.
 *
 * @param pvParameter Parámetro de tarea (no utilizado).
 */

//Tarea que se encarga de detectar el momento en que se presiona el boton y cambiar el estado de las variables MEDIR y HOLD
static void TeclasTask(void *pvParameter){
    int8_t switch_state;
    int8_t switch_prev = 0; // Va a guarda el estado que tenían los botones en el bucle anterior
    while(true){
        switch_state = SwitchesRead(); // 01 para para el botón 1 y 10 para el botón 2

        // Detectar presión de SWITCH_1 (TEC1) 
        if ((switch_state & SWITCH_1) && !(switch_prev & SWITCH_1)) { // Transición de no presionado a presionado
            MEDIR = !MEDIR;
        }
        
        // Detectar presión de SWITCH_2 (TEC2)
        if ((switch_state & SWITCH_2) && !(switch_prev & SWITCH_2)) {
            HOLD = !HOLD;
        }
        
        switch_prev = switch_state; // El estado que es "ahora" pasará a ser el "antes" en la próxima iteracion.
        vTaskDelay(TASK_DELAY_SWITCH / portTICK_PERIOD_MS); // Delay para evitar rebotes y lecturas excesivas
    } 
}

/**
 * @brief Tarea que mide la distancia con el sensor HC-SR04 y controla LEDs y LCD.
 *
 * Esta tarea mide la distancia en centímetros si MEDIR está activado, actualiza
 * la distancia mostrada si HOLD no está activado, muestra la distancia en el LCD,
 * y controla los LEDs según rangos de distancia. Utiliza un delay de 1 segundo
 * entre mediciones.
 *
 * @param pvParameter Parámetro de tarea (no utilizado).
 */

//Tarea que se encarga de medir la distancia, mostrarla en el LCD y controlar los LEDs según la distancia medida.
static void MedirTask(void *pvParameter){
    while(true){
        if (MEDIR) { //Si es false apaga todo y si es true mide la distancia
            distancia_actual = HcSr04ReadDistanceInCentimeters();
            if (!HOLD) { // Si es false se actualiza la distancia mostrada y si es true no se actualiza la distancia mostrada
                distancia_mostrada = distancia_actual; // se queda con el último valor que tenía
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
        vTaskDelay(TASK_DELAY_MEDIR / portTICK_PERIOD_MS); // es el delay entre las mediciones
    }
}
/*==================[external functions definition]==========================*/

/**
 * @brief Función principal de la aplicación.
 *
 * Inicializa el sensor HC-SR04, los LEDs, el LCD, los switches, y crea las tareas
 * TeclasTask y MedirTask para manejar la funcionalidad del medidor de distancia.
 */
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
    // Se pasa: la funcion de la tarea, el nombre de la tarea, el espacio de memoria, parametros de la tarea, proriedad y el ID)
}
/*==================[end of file]============================================*/