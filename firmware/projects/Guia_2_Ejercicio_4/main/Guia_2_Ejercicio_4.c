/*! @file Guia_2_Ejercicio_4.c
 *  @brief Osciloscopio serie con generador ECG usando ADC y UART.
 *
 *  @mainpage Osciloscopio serie con generador ECG
 *
 *  @section genDesc General Description
 *
 *  Esta aplicación genera una señal ECG programable usando el DAC, la convierte
 *  a analógica y la visualiza utilizando el osciloscopio implementado que lee
 *  del canal CH1 del ADC y transmite los resultados por UART al PC.
 *
 *  @section hardConn Hardware Connection
 *
 *  |    Peripheral     |   ESP32 Pin / Signal    |
 *  |:-----------------:|:-----------------------:|
 *  | Potenciómetro extremo | GPIO0                |
 *  | Potenciómetro wiper  | GPIO1 (ADC CH1)      |
 *  | Potenciómetro extremo | GND                  |
 *
 *  @note El potenciómetro está conectado como divisor de tensión:
 *        - un extremo a GPIO0
 *        - el otro extremo a GND
 *        - la pata media (wiper) a GPIO1, que se lee como ADC CH1
 *
 *  @section changelog Changelog
 *
 *  |   Date     | Description                                    |
 *  |:----------:|:-----------------------------------------------|
 *  | 13/05/2026 | ECG generator + oscilloscope implementation   |
 *
 *  @author Wiesner Agustina (agustina.wiesner@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"
#include "timer_mcu.h"

/*==================[macros and definitions]=================================*/
/** @brief Frecuencia de muestreo del osciloscopio en Hz. */
#define FRECUENCIA_MUESTREO_HZ    500u

/** @brief Período de muestreo en microsegundos. */
#define PERIODO_MUESTREO_US       (1000000u / FRECUENCIA_MUESTREO_HZ)

/** @brief Velocidad de la UART hacia el PC. */
#define VELOCIDAD_BAUD_RATE    115200u

/** @brief Datos de la señal ECG programable que se envía al DAC. */
//arreglo unidimensional de constantes de tipo byte que contiene los valores de la señal ECG a generar 
const unsigned char datos_ECG[] = {
17,17,17,17,17,17,17,17,17,17,17,18,18,18,17,17,17,17,17,17,17,18,18,18,18,18,18,18,17,17,16,16,16,16,17,17,18,18,18,17,17,17,17,
18,18,19,21,22,24,25,26,27,28,29,31,32,33,34,34,35,37,38,37,34,29,24,19,15,14,15,16,17,17,17,16,15,14,13,13,13,13,13,13,13,12,12,
10,6,2,3,15,43,88,145,199,237,252,242,211,167,117,70,35,16,14,22,32,38,37,32,27,24,24,26,27,28,28,27,28,28,30,31,31,31,32,33,34,36,
38,39,40,41,42,43,45,47,49,51,53,55,57,60,62,65,68,71,75,79,83,87,92,97,101,106,111,116,121,125,129,133,136,138,139,140,140,139,137,
133,129,123,117,109,101,92,84,77,70,64,58,52,47,42,39,36,34,31,30,28,27,26,25,25,25,25,25,25,25,25,24,24,24,24,25,25,25,25,25,25,25,
24,24,24,24,24,24,24,24,23,23,22,22,21,21,21,20,20,20,20,20,19,19,18,18,18,19,19,19,19,18,17,17,18,18,18,18,18,18,18,18,17,17,17,17,
17,17,17
};

/** @brief Número de muestras disponibles en datos_ECG. */
#define LONGITUD_ECG (sizeof(datos_ECG) / sizeof(datos_ECG[0]))

/*==================[internal data definition]===============================*/
/** @brief Manejador de la tarea que procesa osciloscopio y ECG. */
static TaskHandle_t manejadorTareaOsciloscopio = NULL;

/** @brief Bandera que indica cuándo el temporizador solicitó una muestra. */
static volatile bool pedir_muestra = false;

/** @brief Valor leído del ADC en cada ciclo de muestreo. */
static uint16_t valor_muestra = 0;

/** @brief Índice actual dentro del arreglo de muestras ECG. */
static uint16_t indice_ecg = 0;

/*==================[internal functions declaration]=========================*/
/**
 * @brief Rutina de servicio de temporizador que solicita una nueva muestra.
 *
 * Esta función se ejecuta desde la ISR del temporizador y notifica a la tarea
 * principal para que lea el ADC y actualice el DAC.
 *
 * @param param Datos adicionales del temporizador (no usado).
 */
static void TemporizadorMuestra(void *param);

/**
 * @brief Tarea principal del osciloscopio y generador ECG.
 *
 * Esta tarea espera a que el temporizador indique que hay que tomar una
 * muestra, actualiza la salida DAC con el siguiente valor ECG, lee el ADC
 * desde CH1 y envía el resultado por UART.
 *
 * @param pvParameters Parámetros de la tarea (no usados).
 */
static void TareaOsciloscopioYECG(void *pvParameters);

/*==================[external functions definition]==========================*/

/**
 * @brief Temporizador de muestreo que activa la tarea de adquisición.
 *
 * Se establece la bandera de nueva muestra y se notifica a la tarea de
 * osciloscopio para procesar el siguiente ciclo.
 */
static void TemporizadorMuestra(void *param) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    // Cambiamos la bandera de forma segura
    pedir_muestra = true;

    // Despertamos la tarea por UART
    if (manejadorTareaOsciloscopio != NULL) {
        vTaskNotifyGiveFromISR(manejadorTareaOsciloscopio, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
 * @brief Tarea que genera señales ECG y transmite lecturas ADC.
 *
 * Esta tarea espera por la notificación del temporizador, actualiza el DAC con
 * el siguiente valor del ECG, lee el ADC en CH1 y envía el valor por UART.
 */
static void TareaOsciloscopioYECG(void *pvParameters) {
    (void)pvParameters;
    char mensaje[32];

    for (;;) {
        // Espera la interrupción del temporizador 
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Si el temporizador dio la orden:
        if (pedir_muestra) {
            pedir_muestra = false;

            // 1. Actualizar DAC
            AnalogOutputWrite(datos_ECG[indice_ecg]);
            indice_ecg = (indice_ecg + 1u) % LONGITUD_ECG;

            // 2. Leer ADC
            AnalogInputReadSingle(CH1, &valor_muestra);

            // 3. Enviar por UART en formato puro numérico (máxima compatibilidad)
            snprintf(mensaje, sizeof(mensaje), ">brightness:%u\n", valor_muestra);
            UartSendString(UART_PC, mensaje);
        }
    }
}

/**
 * @brief Inicializa el sistema, periféricos y arranca el muestreo.
 *
 * Se configura la UART, el ADC y el temporizador; luego se crea la tarea
 * principal y se arranca el temporizador de muestreo.
 */
void app_main(void) {
    serial_config_t configuracion_uart = {
        .port = UART_PC,
        .baud_rate = VELOCIDAD_BAUD_RATE,
        .func_p = UART_NO_INT,
        .param_p = NULL
    };

    analog_input_config_t configuracion_adc = {
        .input = CH1,
        .mode = ADC_SINGLE,
        .func_p = NULL,
        .param_p = NULL,
        .sample_frec = 0u
    };

    timer_config_t configuracion_temporizador = {
        .timer = TIMER_A,
        .period = PERIODO_MUESTREO_US,
        .func_p = TemporizadorMuestra,
        .param_p = NULL
    };

    // 1. Inicializar Hardware primero
    UartInit(&configuracion_uart);
    AnalogInputInit(&configuracion_adc);
    AnalogOutputInit(); 

    // 2. Crear la tarea (se queda esperando pacientemente)
    xTaskCreate(TareaOsciloscopioYECG, "TareaOsciloscopioYECG", 4096, NULL, 5, &manejadorTareaOsciloscopio);

    // 3. Iniciar el temporizador AL FINAL (así todo está listo en memoria)
    TimerInit(&configuracion_temporizador);
    TimerStart(configuracion_temporizador.timer);
}
/*==================[end of file]============================================*/