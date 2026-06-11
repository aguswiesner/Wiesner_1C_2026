#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "analog_io_mcu.h"
#include "uart_mcu.h"

#define VELOCIDAD_BAUD_RATE    115200u
#define DURACION_MEDICION_MS   7000u
#define INTERVALO_MUESTREO_MS  400u

static void vTaskSensorMQ3(void *pvParameters);

static void vTaskSensorMQ3(void *pvParameters) {
    (void)pvParameters;
    char mensaje_uart[128];
    bool medir = false;
    TickType_t medir_end_tick = 0;

    for (;;) {
        uint16_t lectura_adc = 0;
        float tension = 0.0f;
        uint8_t tecla = 0;
        TickType_t ahora = xTaskGetTickCount();

        if (UartReadByte(UART_PC, &tecla)) {
            if (tecla == 'M' || tecla == 'm') {
                medir = true;
                medir_end_tick = ahora + pdMS_TO_TICKS(DURACION_MEDICION_MS);
                UartSendString(UART_PC, "INICIANDO MEDICION POR 7 SEGUNDOS...\r\n");
            }
        }

        if (medir) {
            if ((int32_t)(medir_end_tick - ahora) > 0) {
                AnalogInputReadSingle(CH1, &lectura_adc);
                tension = (lectura_adc * 2) / 1000.0f;
                snprintf(mensaje_uart, sizeof(mensaje_uart),
                         "MQ-3 CH1 -> Tension: %.2f V\r\n", tension);
                UartSendString(UART_PC, mensaje_uart);
            } else {
                medir = false;
                UartSendString(UART_PC, "MEDICION FINALIZADA\r\n");
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

    UartSendString(UART_PC, "SISTEMA LISTO. Leyendo MQ-3 en CH1 y enviando datos por serial...\r\n");
}