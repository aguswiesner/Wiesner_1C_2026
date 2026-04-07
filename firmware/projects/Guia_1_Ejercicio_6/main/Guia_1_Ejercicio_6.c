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
 * | 25/03/2026 | Document creation		                         |
 *
 * @author Agustina Wiesner (agustina.wiesner@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <gpio_mcu.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
/*==================[macros and definitions]=================================*/

/*==================[internal data definition]===============================*/

/**
 * @brief Estructura de configuración GPIO
 */
typedef struct
{
	gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;

/**
 * @brief Mapeo BCD a GPIO: b0->GPIO_20, b1->GPIO_21, b2->GPIO_22, b3->GPIO_23
 */
static gpioConf_t bcd_gpio_map[4] = {
	{GPIO_20, GPIO_OUTPUT},
	{GPIO_21, GPIO_OUTPUT},
	{GPIO_22, GPIO_OUTPUT},
	{GPIO_23, GPIO_OUTPUT}
};

/**
 * @brief Mapeo de selección LCD: Dígito 1->GPIO_19 (centenas), Dígito 2->GPIO_18 (decenas), Dígito 3->GPIO_9 (unidades)
 */
static gpioConf_t digit_select_map[3] = {
	{GPIO_19, GPIO_OUTPUT},  // Dígito 1 (centenas)
	{GPIO_18, GPIO_OUTPUT},  // Dígito 2 (decenas)
	{GPIO_9,  GPIO_OUTPUT}   // Dígito 3 (unidades)
};

/*==================[internal functions declaration]=========================*/

/**
 * @brief Setea GPIOs según dígito BCD usando multiplicación lógica (AND)
 * @param bcd_digit Dígito BCD (0-15)
 * @param gpio_conf Vector gpioConf_t con configuración
 */
void bcd_to_gpio(uint8_t bcd_digit, gpioConf_t *gpio_conf);

/**
 * @brief Convierte número a arreglo BCD
 * @param data Número a convertir
 * @param digits Cantidad de dígitos
 * @param bcd_number Arreglo BCD resultante
 * @return 0 éxito, -1 parámetros inválidos, -2 overflow
 */
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number);

/**
 * @brief Muestra valor en display LCD multiplexado
 * @param data Valor a mostrar (0-999 para 3 dígitos)
 * @param digits Cantidad de dígitos (máximo 3)
 * @param bcd_gpio Vector gpioConf_t para bits BCD
 * @param digit_select_gpio Vector gpioConf_t para selección de dígitos
 * @return 0 éxito, -1 error
 */
int8_t display_number(uint32_t data, uint8_t digits, gpioConf_t *bcd_gpio, gpioConf_t *digit_select_gpio);

/*==================[external functions definition]==========================*/

/**
 * @brief Función principal - inicializa GPIOs y demuestra display
 */
void app_main(void)
{
	uint32_t test_values[] = {0, 1, 42, 123};
	uint8_t num_tests = sizeof(test_values) / sizeof(test_values[0]);

	// Configurar todos los GPIOs como salidas
	for (uint8_t i = 0; i < 4; i++)
	{
		GPIOInit(bcd_gpio_map[i].pin, GPIO_OUTPUT);
	}
	for (uint8_t i = 0; i < 3; i++)
	{
		GPIOInit(digit_select_map[i].pin, GPIO_OUTPUT);
	}

	printf("=== Display LCD Multiplexado ===\n");
	printf("Mostrando valores en display de 3 dígitos...\n\n");

	// Ciclo de demostración
	while (1)
	{
		for (uint8_t test_idx = 0; test_idx < num_tests; test_idx++)
		{
			uint32_t value = test_values[test_idx];
			printf("Mostrando: %lu\n", value);

			// Mostrar el valor por 2 segundos
			for (uint8_t sec = 0; sec < 20; sec++)  // 20 iteraciones * 100ms = 2s
			{
				display_number(value, 3, bcd_gpio_map, digit_select_map);
				vTaskDelay(100 / portTICK_PERIOD_MS);
			}
		}
	}
}

/*==================[internal functions definition]=========================*/

/**
 * @brief Implementación de la función bcd_to_gpio (del ejercicio 5)
 */
void bcd_to_gpio(uint8_t bcd_digit, gpioConf_t *gpio_conf)
{
	// Itera sobre los 4 bits del BCD

	for (uint8_t i = 0; i < 4; i++)
	{
		// Extrae el bit i usando multiplicación lógica (AND)
		// Desplaza el número BCD 'i' posiciones a la derecha y aplica AND con 0x01
		
		uint8_t bit_value = (bcd_digit >> i) & 0x01;
		
		// Setea el GPIO con el valor del bit extraído
		GPIOState (gpio_conf[i].pin, bit_value);
	}
}


/**
 * @brief Implementación de la función convertToBcdArray (del ejercicio 4)
 */
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number)
{
    if (bcd_number == NULL || digits == 0) {
        return -1; // parámetros inválidos
    }

    // Llenar con ceros por si data tiene menos dígitos que digits
    for (uint8_t i = 0; i < digits; i++) {
        bcd_number[i] = 0;
    }

    // Se llena de derecha a izquierda (digit menos significativo primero)
    for (int i = digits - 1; i >= 0; i--) {
        bcd_number[i] = data % 10;
        data /= 10;
        if (data == 0) {
            break;
        }
    }

    // Si queda data > 0, entonces no entró en el arreglo (overflow de dígitos)
    if (data != 0) {
        return -2;
    }

    return 0; // éxito
}

/**
 * @brief Implementación de la función display_number
 */
int8_t display_number(uint32_t data, uint8_t digits, gpioConf_t *bcd_gpio, gpioConf_t *digit_select_gpio)
{
	uint8_t bcd_digits[3];  // Máximo 3 dígitos para este display

	// Limitar a máximo 3 dígitos
	if (digits > 3) digits = 3;

	// Convertir el número a arreglo BCD
	int8_t result = convertToBcdArray(data, digits, bcd_digits);
	if (result != 0) {
		return result;  // Error en conversión
	}

	// Multiplexar los dígitos (mostrar uno a la vez)
	for (uint8_t digit_pos = 0; digit_pos < digits; digit_pos++)
	{
		// Apagar todos los dígitos primero
		for (uint8_t i = 0; i < 3; i++)
		{
			GPIOState(digit_select_gpio[i].pin, 0);
		}

		// Setear los bits BCD para este dígito
		bcd_to_gpio(bcd_digits[digit_pos], bcd_gpio);

		// Encender el dígito correspondiente
		// Nota: Los pines de selección son activos en bajo según la tabla
		// SEL_1, SEL_2, SEL_3 activan dígito 1, 2, 3 respectivamente
		GPIOState (digit_select_gpio[digit_pos].pin, 1);

		// Pequeño delay para estabilidad visual (ajustable según necesidades)
		vTaskDelay(5 / portTICK_PERIOD_MS);
	}

	return 0;  // éxito
}

/*==================[end of file]============================================*/