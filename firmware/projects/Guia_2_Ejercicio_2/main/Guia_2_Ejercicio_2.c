/*! @mainpage Proyecto 2: Medidor de distancia por ultrasonido c/interrupciones
 *
 * @section Descripción general del proyecto
 *
 * Esta aplicación implementa un medidor de distancia ultrasónico utilizando el sensor HC-SR04.
 * Controla LEDs según rangos de distancia y muestra el valor en un display LCD.
 * Utiliza interrupciones para los switches TEC1 y TEC2, y un timer para el refresco de mediciones.
 *
 * Rangos de distancia para LEDs:
 * - < 10 cm: Todos los LEDs apagados
 * - 10-20 cm: LED_1 encendido
 * - 20-30 cm: LED_1 y LED_2 encendidos
 * - > 30 cm: LED_1, LED_2, LED_3 encendidos
 *
 * Funcionalidades:
 * - TEC1: Activar/detener medición
 * - TEC2: Mantener resultado en el display (HOLD)
 * - Refresco: 1 segundo
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
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
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 29/04/2026 | Document creation		                         |
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
#include "timer_mcu.h"
#include "delay_mcu.h"
/*==================[macros and definitions]=================================*/
/**
 * @brief Periodo del temporizador en microsegundos (1 segundo)
 */
#define TIMER_PERIOD_US 1000000  // 1 segundo = 1,000,000 us
/*==================[internal data definition]===============================*/

/**
 * @brief Variable que indica si se debe realizar medición
 */
volatile bool MEDIR = false;

/**
 * @brief Variable que indica si se debe mantener el resultado en el display (HOLD)
 */
volatile bool HOLD = false;

/**
 * @brief Distancia medida actualmente por el sensor HC-SR04 en centímetros
*/
uint16_t distancia_actual = 0;

/**
 * @brief Distancia que se muestra en el LCD
*/
uint16_t distancia_mostrada = 0;

/**
 * @brief Handle de la tarea de medición
*/
TaskHandle_t tarea_medicion_handle = NULL;
/*==================[internal functions declaration]=========================*/

/**
 * @brief Interrupción para SWITCH_1 (TEC1)
 * 
 * Esta función se ejecuta cuando se presiona TEC1. Alterna el estado de la variable MEDIR
 * para activar o detener las mediciones periódicas.
 * 
 * @param args Parámetros de la interrupción (no utilizados)
 */
void InterrupcionTec1(void *args) {
    MEDIR = !MEDIR;
}

/**
 * @brief Interrupción para SWITCH_2 (TEC2) - Toggle HOLD
 * 
 * Esta función se ejecuta cuando se presiona TEC2. Alterna el estado de la variable HOLD
 * para congelar o liberar la actualización del display LCD.
*/
void InterrupcionTec2(void *args) {
    HOLD = !HOLD;
}

/**
 * @brief Callback del timer para realizar mediciones periódicas
 * 
 * Esta función se ejecuta cada TIMER_PERIOD_US (1 segundo) desde la interrupción del timer.
 * Envía una notificación a la tarea de medición para que realice una nueva lectura del sensor.
 * 
 */
void TemporizadorMedicion(void *param) {
    vTaskNotifyGiveFromISR(tarea_medicion_handle, pdFALSE); 
}

/**
 * @brief Tarea que maneja las mediciones y actualización de display/LEDs
 * 
 * Esta tarea se ejecuta en un bucle infinito esperando notificaciones del timer.
 * Cuando recibe una notificación, realiza la medición del sensor HC-SR04, actualiza
 * el LCD y controla los LEDs según los rangos de distancia definidos.
 * 
 */
static void TareaMedicion(void *pvParameter) {
    while (true) {
        // Esperar notificación del timer
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //recibe la notificación del timer para realizar la medición
        if (MEDIR) {
            // Realizar medición
            distancia_actual = HcSr04ReadDistanceInCentimeters();
            
            // Actualizar distancia mostrada si no está en HOLD
            if (!HOLD) {
                distancia_mostrada = distancia_actual;
            }
            
            // Mostrar distancia en LCD
            LcdItsE0803Write(distancia_mostrada);
            
            // Controlar LEDs según distancia actual
            if (distancia_actual < 10) {
                LedsOffAll();
            } else if (distancia_actual <= 20) {
                LedsOffAll();
                LedOn(LED_1);
            } else if (distancia_actual <= 30) {
                LedsOffAll();
                LedOn(LED_1);
                LedOn(LED_2);
            } else {
                LedsOffAll();
                LedOn(LED_1);
                LedOn(LED_2);
                LedOn(LED_3);
            }
        } else {
            // Apagar LEDs y display cuando no se mide
            LedsOffAll();
            LcdItsE0803Off();
        }
    }
}

/*==================[external functions definition]==========================*/

/**
 * @brief Función principal de la aplicación
 * 
 * Inicializa todos los periféricos necesarios (HC-SR04, LEDs, LCD, switches),
 * configura las interrupciones para los switches, crea la tarea de medición
 * y configura el timer para mediciones periódicas cada 1 segundo.
 * 
 * Esta función se ejecuta una vez al inicio del programa y luego termina,
 * dejando que las tareas y interrupciones manejen la funcionalidad.
 */
void app_main(void) {
    // Inicializar sensor HC-SR04
    HcSr04Init(GPIO_3, GPIO_2);
    
    // Inicializar LEDs
    LedsInit();
    
    // Inicializar LCD
    LcdItsE0803Init();
    
    // Inicializar switches
    SwitchesInit();
    
    // Configurar interrupciones para switches, donde le paso el swich que prende, la función que se va a ejecutar cuando se active la interrupción y los argumentos de esa función (en este caso no tengo argumentos, por eso le paso NULL)
    SwitchActivInt(SWITCH_1, InterrupcionTec1, NULL);
    SwitchActivInt(SWITCH_2, InterrupcionTec2, NULL);
    
    // Crear tarea de medición antes de arrancar el timer, debido a que el timer va a mandar notificaciones a esta tarea para que realice las mediciones periódicas
    xTaskCreate(&TareaMedicion, "MEDICION", 2048, NULL, 5, &tarea_medicion_handle);
    
    // Configurar timer para mediciones cada 1 segundo
    timer_config_t timer_config = {
        .timer = TIMER_A,
        .period = TIMER_PERIOD_US,
        .func_p = TemporizadorMedicion,
        .param_p = NULL
    };
    TimerInit(&timer_config); // inicializo el timer con la configuración definida
    TimerStart(TIMER_A); // arranco el timer para que empiece a contar y a mandar notificaciones cada 1 segundo a la tarea de medición
    
    // La tarea principal puede terminar
}