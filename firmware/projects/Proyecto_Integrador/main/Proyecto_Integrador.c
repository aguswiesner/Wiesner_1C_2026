/**
 * @file Proyecto_Integrador.c
* @brief Medidor de alcohol en sangre y poder obtener el tiempo de espera recomendado antes de conducir. 
 *
 * Este proyecto lee la salida del sensor de alcohol (MQ-3), calcula el nivel estimado en
 * sangre y el tiempo de espera recomendado para poder manejar, luego se envía el resultado a una aplicación en el celular.
 * El usuario puede ingresar su peso y género para obtener una estimación más personalizada.
 * 
 * @section Las conexiones físicas son:
 * 
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * |    MQ-3 (A0)   | 	 GPIO 1	    |
 * 
 * @author Agustina Wiesner (agustina.wiesner@ingenieria.uner.edu.ar)
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>          
#include <math.h>            
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "analog_io_mcu.h"
#include "ble_mcu.h" 


#define DURACION_MEDICION_MS   7000u
#define INTERVALO_MUESTREO_MS  100u     // Muestreo a 10 Hz 
#define UMBRAL_ALCOHOL_MINIMO  0.05f    // Umbral para que no haya un tiempo de espera si el valor es muy bajo

static void vTaskSensorMQ3(void *pvParameters);
void ble_read_data(uint8_t * data, uint8_t length);

/**
 * @brief Variables globales dinámicas utilizadas para el algoritmo biomédico.
 */
volatile float peso_persona = 70.0f;        /**< Peso del usuario estimado en kilogramos */
volatile char sexo_persona = 'M';           /**< Género biológico seleccionado ('M' o 'F') */
volatile bool booleano_medir = false;       /**< Bandera de control para activar/desactivar el muestreo */
volatile TickType_t medicion_fin_tick = 0;  /**< Marca temporal del fin de la ráfaga de soplado */

/**
 * @brief Recibe datos desde la aplicación vía Bluetooth.
 *
 * Interpreta los comandos de peso y de género y arranca la medición.
 * @param data Puntero al array de bytes recibidos.
 * @param length Cantidad de bytes presentes en el paquete.
 */
void ble_read_data(uint8_t * data, uint8_t length) {
    if (length == 0) return;

    char primer_caracter = data[0]; // Extraer el primer carácter identificador de la App

    switch (primer_caracter) {
        case 'D': // El switch está en posición Masculino (envía 'D')
            if (sexo_persona != 'M') { //Solo actúa si cambió
                sexo_persona = 'M';
            }
            break;
            
        case 'd': // El switch está en posición Femenino (envía 'd')
            if (sexo_persona != 'F') { //Solo actúa si cambió
                sexo_persona = 'F';
            }
            break;
        
        //Cuando en la aplicacion se apreta el boton send el dato se envia con una M adelante del peso
        case 'M': 
            if (length > 1 && !booleano_medir) {
                char dato_peso[10];
                
                //Se extra el peso ignorando la letra M
                memcpy(dato_peso, &data[1], length - 1);
                dato_peso[length - 1] = '\0'; 
                
                float peso_leido = atof(dato_peso);
                if (peso_leido > 0.0f) {
                    peso_persona = peso_leido;
                }

                //Se incia el proceso de medicion por 7 segundos
                medicion_fin_tick = xTaskGetTickCount() + pdMS_TO_TICKS(DURACION_MEDICION_MS);
                booleano_medir = true; 
                
                BleSendString("*E1*"); // Encender el indicador de soplado en la App
            }
            break;

        default:
            break;
    }
}


/**
 * @brief Tarea de muestreo del sensor MQ-3.
 *
 * Lee la salida del sensor, calcula el nivel de alcohol en sangre y el tiempo de espera recomendado y envía los datos.
 */
static void vTaskSensorMQ3(void *pvParameters) {
    (void)pvParameters;
    char mensaje_bluetooth[32]; 
    uint32_t acumulador_adc = 0;
    uint32_t cantidad_muestras = 0;

    for (;;) {
        uint16_t lectura_adc = 0;
        TickType_t ahora = xTaskGetTickCount();

        if (booleano_medir) {
            if ((int32_t)(medicion_fin_tick - ahora) > 0) {
                // Al iniciar un nuevo ciclo, vaciar el acumulador de ráfaga
                if (cantidad_muestras == 0) acumulador_adc = 0;
                
                AnalogInputReadSingle(CH1, &lectura_adc);
                acumulador_adc += lectura_adc;
                cantidad_muestras++;
            } else {
                booleano_medir = false;
                BleSendString("*E0*"); // Apagar el indicador de soplado en la App
                vTaskDelay(50 / portTICK_PERIOD_MS);

                //A partir de acá se realizan los calculos necesarios para obtener el tiempo de espera
                float promedio_adc = (float)acumulador_adc / (float)cantidad_muestras;
                cantidad_muestras = 0;
                
                // Paso de mV a V y multiplico x2 por el divisor de tension
                float voltaje_sensor = (promedio_adc / 1000.0f) * 2.0f;
                // Resistencia en aire limpio (R_o) del MQ-3, valor típico de fábrica
                float Ro = 5463.0f;
                float alcohol_sangre_gl = 0.0000f; // Inicializamos por defecto en cero absoluto

                // Si hay suficiente voltaje, significa que hay alcohol y calculamos las curvas exponenciales
                if (voltaje_sensor > 0.2f) {
                    // 1. Calcular la resistencia dinámica del gas del MQ-3 (R_L = 1000 Ohm)
                    float Rs = 1000.0f * ((5.0f - voltaje_sensor) / voltaje_sensor);
                    
                    // 2. Concentración en Aire (mg/L) mediante la regresión exponencial (R_o = 5463 Ohm)
                    float alcohol_aire_mgl = 0.4091f * powf((Rs / Ro), -1.497f);
                    
                    // 3. Conversión de unidades aire-sangre mediante el factor clínico 2.0
                    float calculo_sangre = alcohol_aire_mgl * 2.0f;

                    // Aplicamos el umbral mínimo de zona muerta para limpiar ruidos residuales
                    if (calculo_sangre >= UMBRAL_ALCOHOL_MINIMO) {
                        alcohol_sangre_gl = calculo_sangre; // Solo se actualiza si supera el mínimo clínico
                    }
                } 
                else {
                    // ELSE: Si el voltaje es <= 0.2V, es aire limpio absoluto. 
                    // No calculamos Rs (evitamos dividir por cero o desbordar powf) y se mantiene en 0.0000 g/L
                    alcohol_sangre_gl = 0.0000f; 
                }

                // 4. Ecuación lineal de depuración hepática (Tasa de Widmark)
                float beta = 0.15f; 
                float horas_espera = alcohol_sangre_gl / beta;

                //Se envían los resultados a la aplicacion en el celular con sus respectivas etiquetas
                
                //Tiempo de espera en minutos (Identificador 'T')
                snprintf(mensaje_bluetooth, sizeof(mensaje_bluetooth), "*T%d*", horas_espera);
                BleSendString(mensaje_bluetooth);
                vTaskDelay(50 / portTICK_PERIOD_MS); 

                //Concentración directa en sangre (Identificador 'S')
                snprintf(mensaje_bluetooth, sizeof(mensaje_bluetooth), "*S%.2f*", alcohol_sangre_gl);
                BleSendString(mensaje_bluetooth);
                vTaskDelay(50 / portTICK_PERIOD_MS);

                //Voltaje Promedio Real para control de laboratorio (Identificador 'V')
                snprintf(mensaje_bluetooth, sizeof(mensaje_bluetooth), "*V%.2f*", voltaje_sensor);
                BleSendString(mensaje_bluetooth);
            }
        }

        vTaskDelay(INTERVALO_MUESTREO_MS / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Inicializa Bluetooth, ADC y la tarea principal.
 *
 * Prepara el sistema para comenzar la medición y el envío de datos.
 */
void app_main(void) {
    // Inicialización del módulo Bluetooth Low Energy con el nombre de red para el celular
    ble_config_t ble_configuracion = {
        .device_name = "ALCOHOLIMETRO", 
        .func_p = ble_read_data         
    };
    BleInit(&ble_configuracion);

    // Inicialización del Conversor Analógico Digital (ADC) en CH1
    analog_input_config_t configuracion_adc = {
        .input = CH1,
        .mode = ADC_SINGLE,
        .func_p = NULL,
        .param_p = NULL,
        .sample_frec = 0u
    };
    AnalogInputInit(&configuracion_adc);

    //Se crea la tarea del sensor MQ-3  
    xTaskCreate(&vTaskSensorMQ3, "SENSOR_MQ3", 4096, NULL, 5, NULL);
}