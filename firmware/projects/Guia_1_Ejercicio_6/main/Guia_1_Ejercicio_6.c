/*! @mainpage Guía 1 - Ejercicio 6: Display LCD Multiplexado con Codificación BCD
 *
 * @section genDesc Descripción General
 *
 * Este ejercicio implementa un controlador para display LCD multiplexado de 3 dígitos
 * utilizando codificación BCD (Binary Coded Decimal). El sistema convierte números
 * decimales en su representación BCD y los muestra en un display de 7 segmentos
 * multiplexado, donde cada dígito se enciende secuencialmente para crear la ilusión
 * de que todos los dígitos están encendidos simultáneamente.
 *
 * El programa demuestra el funcionamiento mostrando una secuencia de valores de prueba
 * (0, 1, 42, 123) en el display, manteniendo cada valor visible por 2 segundos.
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|   Descripción					|
 * |:--------------:|:--------------|:------------------------------|
 * | 	BCD_0	 	| 	GPIO_20	|	Bit 0 del código BCD			|
 * | 	BCD_1	 	| 	GPIO_21	|	Bit 1 del código BCD			|
 * | 	BCD_2	 	| 	GPIO_22	|	Bit 2 del código BCD			|
 * | 	BCD_3	 	| 	GPIO_23	|	Bit 3 del código BCD			|
 * | 	DIGIT_1	 	| 	GPIO_19	|	Selección dígito centenas (activo alto)	|
 * | 	DIGIT_2	 	| 	GPIO_18	|	Selección dígito decenas (activo alto)	|
 * | 	DIGIT_3	 	| 	GPIO_9	|	Selección dígito unidades (activo alto)	|
 *
 * @note Los pines de selección de dígitos están configurados como activos en alto.
 * Cuando un pin de selección está en alto, el dígito correspondiente se enciende.
 *
 * @section dependencies Dependencies
 *
 * - gpio_mcu.h: Control de GPIOs del ESP32
 * - FreeRTOS: Para delays y tareas del sistema operativo
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 25/03/2026 | Document creation		                         |
 * | 08/04/2026 | Improved Doxygen documentation and hardware connections |
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
 *
 * Esta estructura define la configuración necesaria para un pin GPIO,
 * incluyendo el número del pin y su dirección (entrada/salida).
 */
typedef struct
{
	gpio_t pin;			/*!< Número del pin GPIO */
	io_t dir;			/*!< Dirección del GPIO: GPIO_INPUT o GPIO_OUTPUT */
} gpioConf_t;

/**
 * @brief Mapeo de bits BCD a pines GPIO
 *
 * Array que mapea cada bit del código BCD a un pin GPIO específico.
 * Los bits se ordenan de menos significativo (b0) a más significativo (b3).
 *
 * @note Los pines utilizados son GPIO_20, GPIO_21, GPIO_22, GPIO_23
 */
static gpioConf_t bcd_gpio_map[4] = {
	{GPIO_20, GPIO_OUTPUT},	/*!< Bit 0 (LSB) -> GPIO_20 */
	{GPIO_21, GPIO_OUTPUT},	/*!< Bit 1 -> GPIO_21 */
	{GPIO_22, GPIO_OUTPUT},	/*!< Bit 2 -> GPIO_22 */
	{GPIO_23, GPIO_OUTPUT}	/*!< Bit 3 (MSB) -> GPIO_23 */
};

/**
 * @brief Mapeo de selección de dígitos del display LCD
 *
 * Array que mapea cada dígito del display a su pin de selección correspondiente.
 * El orden es centenas (índice 0), decenas (índice 1), unidades (índice 2).
 *
 * @note Los pines utilizados son GPIO_19, GPIO_18, GPIO_9
 */
static gpioConf_t digit_select_map[3] = {
	{GPIO_19, GPIO_OUTPUT},  /*!< Dígito 1 (centenas) -> GPIO_19 */
	{GPIO_18, GPIO_OUTPUT},  /*!< Dígito 2 (decenas) -> GPIO_18 */
	{GPIO_9,  GPIO_OUTPUT}   /*!< Dígito 3 (unidades) -> GPIO_9 */
};

/*==================[internal functions declaration]=========================*/

/**
 * @brief Configura los pines GPIO según un dígito BCD
 *
 * Esta función toma un valor BCD (0-15) y configura los pines GPIO correspondientes
 * para representar ese valor en código binario. Utiliza operaciones de desplazamiento
 * de bits y máscara AND para extraer cada bit individual.
 *
 * @param[in] bcd_digit Dígito BCD a representar (0-15, valores > 15 se truncan)
 * @param[in] gpio_conf Puntero al array de configuración GPIO (debe tener al menos 4 elementos)
 *
 * @note Implementa la lógica del Ejercicio 5
 * @note No retorna valor, modifica directamente los pines GPIO
 */
void bcd_to_gpio(uint8_t bcd_digit, gpioConf_t *gpio_conf);

/**
 * @brief Convierte un número decimal a su representación en arreglo BCD
 *
 * Esta función descompone un número decimal en sus dígitos individuales y los
 * almacena en un arreglo. El arreglo resultante contiene los dígitos en orden
 * de unidades, decenas, centenas, etc. (de derecha a izquierda).
 *
 * @param[in] data Número decimal a convertir (0-999 para 3 dígitos)
 * @param[in] digits Número de dígitos del arreglo resultante (máximo 3)
 * @param[out] bcd_number Puntero al arreglo donde se almacenarán los dígitos BCD
 *
 * @return int8_t Código de resultado:
 *         - 0: Conversión exitosa
 *         - -1: Parámetros inválidos (bcd_number NULL o digits = 0)
 *         - -2: Overflow (el número requiere más dígitos que los especificados)
 *
 * @note Implementa la lógica del Ejercicio 4
 * @note Los dígitos se almacenan de derecha a izquierda (unidades primero)
 */
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number);

/**
 * @brief Muestra un valor numérico en un display LCD multiplexado
 *
 * Esta función implementa el algoritmo de multiplexación para mostrar un número
 * en un display LCD de múltiples dígitos. Convierte el número a BCD, luego
 * enciende cada dígito secuencialmente con un pequeño delay para crear la
 * ilusión de que todos los dígitos están encendidos simultáneamente.
 *
 * @param[in] data Valor numérico a mostrar (0-999 para display de 3 dígitos)
 * @param[in] digits Número de dígitos a utilizar (máximo 3, se limita automáticamente)
 * @param[in] bcd_gpio Puntero al array de configuración GPIO para bits BCD
 * @param[in] digit_select_gpio Puntero al array de configuración GPIO para selección de dígitos
 *
 * @return int8_t Código de resultado:
 *         - 0: Visualización exitosa
 *         - -1: Error en la conversión BCD
 *         - -2: Overflow en la conversión BCD
 *
 * @note La función incluye delays internos para el multiplexado
 * @note Los pines de selección de dígitos son activos en alto
 */
int8_t display_number(uint32_t data, uint8_t digits, gpioConf_t *bcd_gpio, gpioConf_t *digit_select_gpio);

/*==================[external functions definition]==========================*/

/**
 * @brief Función principal del programa - Punto de entrada de la aplicación ESP-IDF
 *
 * Esta función inicializa todos los pines GPIO necesarios para el display LCD,
 * configura el sistema y ejecuta un ciclo de demostración que muestra diferentes
 * valores numéricos en el display multiplexado.
 *
 * El programa muestra una secuencia de valores de prueba (0, 1, 42, 123) en un
 * bucle infinito, manteniendo cada valor visible por 2 segundos antes de pasar
 * al siguiente.
 *
 * @note Esta función nunca retorna en un sistema embebido con FreeRTOS
 * @note Utiliza la tarea por defecto de ESP-IDF (app_main)
 */
void app_main(void)
{
	/** @brief Valores de prueba para demostrar el funcionamiento del display */
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
 *
 * @details Esta función itera sobre los 4 bits menos significativos del valor BCD
 * y configura cada pin GPIO correspondiente al estado de ese bit (0 o 1).
 * Utiliza operaciones de desplazamiento de bits para extraer cada bit individual.
 *
 * Algoritmo:
 * 1. Para cada bit i de 0 a 3:
 *    a. Desplaza el dígito BCD i posiciones a la derecha
 *    b. Aplica máscara AND con 0x01 para obtener el valor del bit
 *    c. Configura el pin GPIO correspondiente con ese valor
 *
 * @param[in] bcd_digit Dígito BCD a representar (0-15)
 * @param[in] gpio_conf Puntero al array de configuración GPIO
 *
 * @note Los valores > 15 se truncan automáticamente (solo se usan 4 bits)
 * @note Esta implementación reutiliza la lógica del Ejercicio 5
 */
void bcd_to_gpio(uint8_t bcd_digit, gpioConf_t *gpio_conf)
{
	// Itera sobre los 4 bits del BCD (de LSB a MSB)
	for (uint8_t i = 0; i < 4; i++)
	{
		// Extrae el bit i usando desplazamiento y máscara AND
		// Desplaza el número BCD 'i' posiciones a la derecha y aplica AND con 0x01
		uint8_t bit_value = (bcd_digit >> i) & 0x01;

		// Setea el GPIO correspondiente con el valor del bit extraído
		GPIOState(gpio_conf[i].pin, bit_value);
	}
}


/**
 * @brief Implementación de la función convertToBcdArray (del ejercicio 4)
 *
 * @details Esta función convierte un número decimal en su representación BCD,
 * descomponiéndolo en dígitos individuales. El algoritmo procesa el número
 * de derecha a izquierda (unidades primero), utilizando el operador módulo (%)
 * para obtener cada dígito y división entera para continuar con el siguiente.
 *
 * Algoritmo:
 * 1. Validar parámetros de entrada
 * 2. Inicializar el arreglo con ceros
 * 3. Extraer dígitos de derecha a izquierda usando % 10 y / 10
 * 4. Verificar si el número cabe en los dígitos especificados
 *
 * @param[in] data Número decimal a convertir
 * @param[in] digits Número de dígitos del arreglo resultante
 * @param[out] bcd_number Arreglo donde se almacenan los dígitos (unidades en índice 0)
 *
 * @return int8_t Código de resultado
 *
 * @note Esta implementación reutiliza la lógica del Ejercicio 4
 * @note El arreglo se llena de derecha a izquierda (bcd_number[0] = unidades)
 */
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number)
{
	// Validar parámetros de entrada
	if (bcd_number == NULL || digits == 0) {
		return -1; // parámetros inválidos
	}

	// Llenar con ceros por si data tiene menos dígitos que digits
	for (uint8_t i = 0; i < digits; i++) {
		bcd_number[i] = 0;
	}

	// Se llena de derecha a izquierda (dígito menos significativo primero)
	// El índice 0 contiene las unidades, índice 1 las decenas, etc.
	for (int i = digits - 1; i >= 0; i--) {
		bcd_number[i] = data % 10;  // Extraer dígito menos significativo
		data /= 10;                 // Remover dígito procesado
		if (data == 0) {
			break;  // No hay más dígitos que procesar
		}
	}

	// Si queda data > 0, entonces no entró en el arreglo (overflow de dígitos)
	if (data != 0) {
		return -2;  // Overflow: número demasiado grande para los dígitos especificados
	}

	return 0; // éxito
}

/**
 * @brief Implementación de la función display_number
 *
 * @details Esta función implementa el algoritmo completo de multiplexación para
 * display LCD. Convierte un número decimal a BCD, luego enciende cada dígito
 * del display de forma secuencial con delays apropiados para crear la ilusión
 * de visualización simultánea.
 *
 * Algoritmo de multiplexación:
 * 1. Convertir el número a arreglo BCD
 * 2. Para cada posición de dígito:
 *    a. Apagar todos los dígitos
 *    b. Configurar los bits BCD para el dígito actual
 *    c. Encender el dígito correspondiente
 *    d. Esperar tiempo de multiplexación (5ms)
 *
 * @param[in] data Valor numérico a mostrar (0-999 para 3 dígitos)
 * @param[in] digits Número de dígitos a utilizar
 * @param[in] bcd_gpio Array de configuración GPIO para bits BCD
 * @param[in] digit_select_gpio Array de configuración GPIO para selección de dígitos
 *
 * @return int8_t Código de resultado (0 = éxito, negativo = error)
 *
 * @note El tiempo de multiplexación (5ms) es crítico para la estabilidad visual
 * @note Los pines de selección son activos en alto según la implementación
 * @note Esta función bloquea durante el tiempo de multiplexación completo
 */
int8_t display_number(uint32_t data, uint8_t digits, gpioConf_t *bcd_gpio, gpioConf_t *digit_select_gpio)
{
	uint8_t bcd_digits[3];  // Máximo 3 dígitos para este display

	// Limitar a máximo 3 dígitos para evitar problemas de hardware
	if (digits > 3) digits = 3;

	// Convertir el número a arreglo BCD
	int8_t result = convertToBcdArray(data, digits, bcd_digits);
	if (result != 0) {
		return result;  // Error en conversión
	}

	// Multiplexar los dígitos (mostrar uno a la vez)
	for (uint8_t digit_pos = 0; digit_pos < digits; digit_pos++)
	{
		// Apagar todos los dígitos primero (estado seguro)
		for (uint8_t i = 0; i < 3; i++)
		{
			GPIOState(digit_select_gpio[i].pin, 0);
		}

		// Setear los bits BCD para este dígito específico
		bcd_to_gpio(bcd_digits[digit_pos], bcd_gpio);

		// Encender el dígito correspondiente
		// Nota: Los pines de selección son activos en alto según la implementación
		// SEL_1, SEL_2, SEL_3 activan dígito 1, 2, 3 respectivamente
		GPIOState(digit_select_gpio[digit_pos].pin, 1);

		// Pequeño delay para estabilidad visual (ajustable según necesidades)
		// 5ms por dígito permite refresco completo en ~15ms (frecuencia ~67Hz)
		vTaskDelay(5 / portTICK_PERIOD_MS);
	}

	return 0;  // éxito
}

/*==================[end of file]============================================*/