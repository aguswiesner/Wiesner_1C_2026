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
#define INTERVALO_MUESTREO_MS  100u     // Muestreo a 10 Hz (Cumple Criterio de Nyquist)
#define UMBRAL_ALCOHOL_MINIMO  0.05f    // Zona muerta para limpiar ruidos de línea

static void vTaskSensorMQ3(void *pvParameters);
void ble_read_data(uint8_t * data, uint8_t length);

// Variables globales de control de estado dinámico
volatile float peso_persona = 70.0f;    
volatile char sexo_persona = 'M';       
volatile bool bandera_medir = false;
volatile TickType_t medir_end_tick = 0;

/**
 * @brief Callback de recepción Bluetooth con filtro anti-saturación para el Switch.
 * Procesa los datos de la terminal (M[peso]) y el interruptor de género (D / d).
 */
void ble_read_data(uint8_t * data, uint8_t length) {
    if (length == 0) return;

    char token = data[0]; // Extraer el primer carácter identificador de la App

    switch (token) {
        case 'D': // El switch está en posición Masculino (envía 'D' cada 50ms)
            if (sexo_persona != 'M') { // FILTRO ANTI-REPETICIÓN: Solo actúa si cambió
                sexo_persona = 'M';
            }
            break;
            
        case 'd': // El switch está en posición Femenino (envía 'd' cada 50ms)
            if (sexo_persona != 'F') { // FILTRO ANTI-REPETICIÓN: Solo actúa si cambió
                sexo_persona = 'F';
            }
            break;
            
        case 'M': // ¡BOTÓN SEND DE LA TERMINAL PRESIONADO! (Llega por ejemplo "M54")
            if (length > 1 && !bandera_medir) {
                char buffer_peso[10];
                
                // Extraer el número del peso ignorando la letra 'M' inicial
                memcpy(buffer_peso, &data[1], length - 1);
                buffer_peso[length - 1] = '\0'; 
                
                float peso_leido = atof(buffer_peso);
                if (peso_leido > 0.0f) {
                    peso_persona = peso_leido; // Guardar peso real en memoria
                }

                // Disparar inmediatamente el ensayo de soplado de 7 segundos
                medir_end_tick = xTaskGetTickCount() + pdMS_TO_TICKS(DURACION_MEDICION_MS);
                bandera_medir = true; 
                
                BleSendString("*E1*"); // Encender el indicador de soplado en la App
            }
            break;

        default:
            break;
    }
}

/**
 * @brief Tarea principal de FreeRTOS encargada del muestreo y procesamiento analítico.
 */
static void vTaskSensorMQ3(void *pvParameters) {
    (void)pvParameters;
    char mensaje_bluetooth[32]; 
    uint32_t acumulador_adc = 0;
    uint32_t cantidad_muestras = 0;

    for (;;) {
        uint16_t lectura_adc = 0;
        TickType_t ahora = xTaskGetTickCount();

        if (bandera_medir) {
            if ((int32_t)(medir_end_tick - ahora) > 0) {
                // Al iniciar un nuevo ciclo, vaciar el acumulador de ráfaga
                if (cantidad_muestras == 0) acumulador_adc = 0;
                
                AnalogInputReadSingle(CH1, &lectura_adc);
                acumulador_adc += lectura_adc;
                cantidad_muestras++;
            } else {
                bandera_medir = false;
                BleSendString("*E0*"); // Apagar el indicador de soplado en la App
                vTaskDelay(50 / portTICK_PERIOD_MS);

                // --- PROCESAMIENTO BIOMÉDICO GENERAL ---
                float promedio_adc = (float)acumulador_adc / (float)cantidad_muestras;
                cantidad_muestras = 0; // Resetear contador para la próxima prueba
                
                // Conversión matemática a Voltios del Pin y reconstrucción del divisor x2
                float v_pin_promedio = (promedio_adc * 3.3f) / 4095.0f;
                float v_sensor_promedio = v_pin_promedio * 2.0f;

                // Protecciones de saturación de hardware
                if (v_sensor_promedio < 0.1f) v_sensor_promedio = 0.1f;
                if (v_sensor_promedio > 4.9f) v_sensor_promedio = 4.9f;

                // 1. Calcular la resistencia dinámica del gas del MQ-3
                float Rs = 1000.0f * ((5.0f - v_sensor_promedio) / v_sensor_promedio);

                // 2. Concentración en Aire (mg/L) con protección de umbral contra ruidos
                float alcohol_aire_mgl = 0.0000f;
                float Ro = 5463.0f;
                if (v_sensor_promedio > 0.2f) {
                    alcohol_aire_mgl = 0.4091f * powf((Rs / Ro), -1.497f);
                }

                // 3. Conversión de unidades aire-sangre mediante el factor clínico de reparto 2.0
                float alcohol_sangre_gl = alcohol_aire_mgl * 2.0f;

                // --- FILTRADO DE HUMEDAD (ZONA MUERTA) ---
                if (alcohol_sangre_gl < UMBRAL_ALCOHOL_MINIMO) {
                    alcohol_sangre_gl = 0.0000f;
                }

                // 4. Ecuación lineal de depuración hepática (Tasa de Widmark)
                float beta = 0.15f; 
                float horas_espera = alcohol_sangre_gl / beta;
                int minutos_totales_espera = (int)(horas_espera * 60.0f);

                // 5. Constante de Widmark según sexo biológico actual en memoria
                float r_widmark = (sexo_persona == 'F' || sexo_persona == 'f') ? 0.6f : 0.7f;

                // 6. Cálculo inverso de gramos de alcohol puro activos en el organismo
                float gramos_alcohol_cuerpo = alcohol_sangre_gl * peso_persona * r_widmark;

                // --- TRANSMISIÓN COMPACTA BLE (MENOR A 20 BYTES POR PAQUETE) ---
                
                // Enviar Tiempo de espera en minutos (Identificador 'T')
                snprintf(mensaje_bluetooth, sizeof(mensaje_bluetooth), "*T%d*", minutos_totales_espera/60);
                BleSendString(mensaje_bluetooth);
                vTaskDelay(50 / portTICK_PERIOD_MS); 

                // Enviar Gramos de alcohol en cuerpo (Identificador 'A') -> Ej: *A15.4*
                snprintf(mensaje_bluetooth, sizeof(mensaje_bluetooth), "*A%.1f*", gramos_alcohol_cuerpo);
                BleSendString(mensaje_bluetooth);
                vTaskDelay(50 / portTICK_PERIOD_MS); 

                // Enviar Concentración directa en sangre (Identificador 'S') -> Ej: *S0.45*
                snprintf(mensaje_bluetooth, sizeof(mensaje_bluetooth), "*S%.2f*", alcohol_sangre_gl);
                BleSendString(mensaje_bluetooth);
                vTaskDelay(50 / portTICK_PERIOD_MS);

                // Enviar Voltaje Promedio Real para control de laboratorio (Identificador 'V') -> Ej: *V1.62*
                snprintf(mensaje_bluetooth, sizeof(mensaje_bluetooth), "*V%.2f*", v_sensor_promedio);
                BleSendString(mensaje_bluetooth);
            }
        }

        vTaskDelay(INTERVALO_MUESTREO_MS / portTICK_PERIOD_MS);
    }
}

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

    // Crear la tarea del sensor en el scheduler de FreeRTOS
    xTaskCreate(&vTaskSensorMQ3, "SENSOR_MQ3", 4096, NULL, 5, NULL);
}