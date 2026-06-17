#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>          
#include <math.h>            
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"

#define VELOCIDAD_BAUD_RATE    115200u
#define DURACION_MEDICION_MS   7000u
#define INTERVALO_MUESTREO_MS  100u     // Muestreo a 10 Hz (Cumple Criterio de Nyquist)
#define UMBRAL_ALCOHOL_MINIMO  0.05f    

static void vTaskSensorMQ3(void *pvParameters);

volatile float peso_persona = 70.0f;
volatile char sexo_persona = 'M';

static void vTaskSensorMQ3(void *pvParameters) {
    (void)pvParameters;
    char mensaje_uart[128];
    char buffer_recepcion[32];
    uint8_t indice_buffer = 0;
    
    bool medir = false;
    TickType_t medir_end_tick = 0;
    uint32_t acumulador_adc = 0;
    uint32_t cantidad_muestras = 0;

    for (;;) {
        uint16_t lectura_adc = 0;
        float tension = 0.0f;
        uint8_t byte_recibido = 0;
        TickType_t ahora = xTaskGetTickCount();

        if (UartReadByte(UART_PC, &byte_recibido)) {
            if (byte_recibido == '\n' || byte_recibido == '\r') {
                if (indice_buffer > 0) {
                    buffer_recepcion[indice_buffer] = '\0'; 
                    
                    float peso_leido = 0.0f;
                    char sexo_leido = ' ';
                    
                    if (sscanf(buffer_recepcion, "M,%f,%c", &peso_leido, &sexo_leido) == 2) {
                        peso_persona = peso_leido;
                        sexo_persona = sexo_leido;
                        
                        medir = true;
                        acumulador_adc = 0;
                        cantidad_muestras = 0;
                        medir_end_tick = ahora + pdMS_TO_TICKS(DURACION_MEDICION_MS);
                        
                        snprintf(mensaje_uart, sizeof(mensaje_uart), 
                                 "\r\n[DATOS ACEPTADOS] Peso: %.1f kg | Sexo: %c. INICIANDO ENSAYO...\r\n", 
                                 peso_persona, sexo_persona);
                        UartSendString(UART_PC, mensaje_uart);
                    }
                    indice_buffer = 0; 
                }
            } else {
                if (indice_buffer < sizeof(buffer_recepcion) - 1) {
                    buffer_recepcion[indice_buffer++] = byte_recibido;
                }
            }
        }

        if (medir) {
            if ((int32_t)(medir_end_tick - ahora) > 0) {
                AnalogInputReadSingle(CH1, &lectura_adc);
                acumulador_adc += lectura_adc;
                cantidad_muestras++;
                
                tension = (lectura_adc * 2) / 1000.0f;
                
                if (cantidad_muestras % 10 == 0) {
                    snprintf(mensaje_uart, sizeof(mensaje_uart),
                             "Soplado -> Muestra [%lu] | Tension Pin x2: %.2f V\r\n", 
                             (unsigned long)cantidad_muestras, tension);
                    UartSendString(UART_PC, mensaje_uart);
                }
            } else {
                medir = false;
                UartSendString(UART_PC, "MEDICION FINALIZADA. CALCULANDO...\r\n");

                // --- CALCULO DE PROMEDIOS ---
                float promedio_adc = (float)acumulador_adc / (float)cantidad_muestras;
                
                float v_pin_promedio = (promedio_adc * 3.3f) / 4095.0f;
                float v_sensor_promedio = v_pin_promedio * 2.0f;

                // Protecciones físicas de tensión
                if (v_sensor_promedio < 0.1f) v_sensor_promedio = 0.1f;
                if (v_sensor_promedio > 4.9f) v_sensor_promedio = 4.9f;

                // 1. Resistencia dinámica del gas
                float Rs = 1000.0f * ((5.0f - v_sensor_promedio) / v_sensor_promedio);

                // 2. Concentración en Aire (mg/L) - ¡CON PROTECCIÓN MATEMÁTICA!
                float alcohol_aire_mgl = 0.0000f;
                float Ro = 5463.0f;

                // Solo calculamos la potencia si el voltaje indica que realmente hay alcohol presente
                if (v_sensor_promedio > 0.2f) {
                    alcohol_aire_mgl = 0.4091f * powf((Rs / Ro), -1.497f);
                }

                // 3. Concentración en Sangre (g/L) 
                float alcohol_sangre_gl = alcohol_aire_mgl * 2.0f;

                // --- APLICACIÓN DE LA ZONA MUERTA ---
                if (alcohol_sangre_gl < UMBRAL_ALCOHOL_MINIMO) {
                    alcohol_sangre_gl = 0.0000f;
                }

                // 4. Tasa de eliminación hepática promedio por hora
                float beta = 0.15f; 

                // 5. Tiempo de Eliminación
                float horas_espera = alcohol_sangre_gl / beta;
                int minutos_totales_espera = (int)(horas_espera * 60.0f);

                // 6. Constante de Widmark
                float r_widmark = (sexo_persona == 'F' || sexo_persona == 'f') ? 0.6f : 0.7f;

                // 7. Gramos de alcohol en cuerpo
                float gramos_alcohol_cuerpo = alcohol_sangre_gl * peso_persona * r_widmark;

                // --- ENVÍO DE RESULTADOS SEGURO CON TIEMPOS DE ESPERA PARA LA UART ---
                UartSendString(UART_PC, "--------------------------------------------------------\r\n");
                vTaskDelay(20 / portTICK_PERIOD_MS); // Le da tiempo a la UART de respirar

                snprintf(mensaje_uart, sizeof(mensaje_uart), "VOLTAJE PROMEDIO DEL SENSOR: %.3f V\r\n", v_sensor_promedio);
                UartSendString(UART_PC, mensaje_uart);
                vTaskDelay(20 / portTICK_PERIOD_MS); 

                snprintf(mensaje_uart, sizeof(mensaje_uart), "ALCOHOL EN SANGRE: %.4f g/L\r\n", alcohol_sangre_gl);
                UartSendString(UART_PC, mensaje_uart);
                vTaskDelay(20 / portTICK_PERIOD_MS); 

                if (minutos_totales_espera > 0) {
                    int h = minutos_totales_espera / 60;
                    int m = minutos_totales_espera % 60;
                    snprintf(mensaje_uart, sizeof(mensaje_uart), "TIEMPO DE RECUPERACION: %d horas y %d minutos\r\n", h, m);
                    UartSendString(UART_PC, mensaje_uart);
                } else {
                    UartSendString(UART_PC, "TIEMPO REQUERIDO DE ESPERA: 0 minutos (Apto para conducir)\r\n");
                }
                vTaskDelay(20 / portTICK_PERIOD_MS); 
                
                UartSendString(UART_PC, "--------------------------------------------------------\r\n\n");
            }
        }

        vTaskDelay(INTERVALO_MUESTREO_MS / portTICK_PERIOD_MS);
    }
}

void app_main(void) {
    serial_config_t uart_pc = {
        .port = UART_PC,
        .baud_rate = VELOCIDAD_BAUD_RATE,
        .func_p = NULL,
        .param_p = NULL
    };
    UartInit(&uart_pc);

    analog_input_config_t configuracion_adc = {
        .input = CH1,
        .mode = ADC_SINGLE,
        .func_p = NULL,
        .param_p = NULL,
        .sample_frec = 0u
    };
    AnalogInputInit(&configuracion_adc);

    xTaskCreate(&vTaskSensorMQ3, "SENSOR_MQ3", 4096, NULL, 5, NULL);

    UartSendString(UART_PC, "SISTEMA INICIALIZADO. Envíe la cadena (ej: M,75,M) para iniciar el ensayo...\r\n");
}