// Agregamos Bibliotecas de c
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>
#include <unistd.h>
// Agregamos para el uso de bibliotecas de proyecto libopencm3
#include "clock.h"
#include "console.h"
#include "sdram.h"
#include "lcd-spi.h"
#include "gfx.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/usart.h>

/*Parametros para el giroscopio tomados de ejemplos F3, spi.c  
libopencm3-examples/examples/stm32/f3/stm32f3-discovery/spi/spi.c */
#define GYR_RNW			(1 << 7)      /* Write when zero */
#define GYR_MNS			(1 << 6)      /* Multiple reads when 1 */
#define GYR_WHO_AM_I		0x0F      // direccion del registro de identificacion del dispositivo
#define GYR_OUT_TEMP		0x26      // direccion del registro de temperatura de salida
#define GYR_STATUS_REG		0x27      // direccion del registro de estado del dispositivo
#define GYR_CTRL_REG1		0x20      // direccion del registro de control 1 del dispositivo

/*para configurar el modo de operacion del dispositivo 
y activar las lecturas de los ejes X, Y y Z.*/
#define GYR_CTRL_REG1_PD	(1 << 3)
#define GYR_CTRL_REG1_XEN	(1 << 1)
#define GYR_CTRL_REG1_YEN	(1 << 0)
#define GYR_CTRL_REG1_ZEN	(1 << 2)

#define GYR_CTRL_REG1_BW_SHIFT	4   	//  frecuencia de corte del filtro de PB
#define GYR_CTRL_REG4		0x23    //  direccion del registro de control 4
#define GYR_CTRL_REG4_FS_SHIFT	4   	//  define el rango de medida del giroscopio

/* Se define las direcciones de los registros 
de salida de los ejes X, Y y Z */
#define GYR_OUT_X_L		0x28  // 40
#define GYR_OUT_X_H		0x29  // 41
#define GYR_OUT_Y_L		0x2A  // 42
#define GYR_OUT_Y_H		0x2B  // 43
#define GYR_OUT_Z_L		0x2C  // 44
#define GYR_OUT_Z_H		0x2D  // 45
/* Sensibilidad del giroscopio 
(x unidad digital,  0.00875 grados de cambio)*/ 
#define L3GD20_SENSITIVITY_250DPS  (0.00875F)  

//Se crea estructura giroscopio, almacena y manipular los datos de lectura
typedef struct Giroscopio  {
  int16_t x; // lectura de un eje x 16 bits
  int16_t y; // lectura de un eje y 16 bits
  int16_t z; // lectura de un eje z 16 bits
} gyro;


/*funcion configura una interfaz SPI para comunicarse con el giroscopio*/
static void spi_setup(void)
{
	/* Para habilitar relojes de perifericos 
	necesarios para la comunicacion SPI,*/
	rcc_periph_clock_enable(RCC_SPI5);
    	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_GPIOF);

    	/* configurando pin GPIO1, puerto GPIOC como out y en alto. 
	sera pin NSS (Slave Select) para controlar el giroscopio. */
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);
	gpio_set(GPIOC, GPIO1);

	/* configurando pines GPIO7, GPIO8 y GPIO9 del puerto GPIOF 
	como funciones alternativas para la interfaz SPI.*/
    	gpio_mode_setup(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO7 | GPIO8 | GPIO9);   
	gpio_set_af(GPIOF, GPIO_AF5, GPIO7 | GPIO8 | GPIO9);

    	/* configurado la interfaz SPI para comunicarse con el giroscopio,*/
    	spi_set_master_mode(SPI5);     				    // Configura el SPI para que opere en modo maestro.
	spi_set_baudrate_prescaler(SPI5, SPI_CR1_BR_FPCLK_DIV_64);  // Configura el prescaler para F-spi, factor div 64 respecto F fuente  
	spi_set_clock_polarity_0(SPI5);                             // Polaridad del reloj del SPI en 0
	spi_set_clock_phase_0(SPI5);                                // Fase del reloj del SPI en 0
	spi_set_full_duplex_mode(SPI5);                             // Modo de comunicacion duplex completo
	spi_set_unidirectional_mode(SPI5); 							/* bidirectional but in 3-wire */ 
	spi_enable_software_slave_management(SPI5);                 // Habilita la gestion de esclavo
	spi_send_msb_first(SPI5);                                   // Para direccion de transmision, bit mas significativo primero
	spi_set_nss_high(SPI5);                                     // Pin de seleccion de esclavo (NSS) en estado alto.
    	SPI_I2SCFGR(SPI5) &= ~SPI_I2SCFGR_I2SMOD;                   // Deshabilita el modo I2S del SPI
	spi_enable(SPI5);											// Habilita el protocolo

	/* Envio de comandos de configuracion especificos para el giroscopio
	habilita la medicion de datos en los ejes y ancho de banda del filtro*/
	// Se baja el pin del chip select para activar el giroscopio
	gpio_clear(GPIOC, GPIO1);
	// Se envia el registro de control del giroscopio por SPI
	spi_send(SPI5, GYR_CTRL_REG1); 
	// Se lee el byte de respuesta del giroscopio
	spi_read(SPI5);
	// Registro de control para activar los ejes y se establece filtro de 25 Hz
	spi_send(SPI5, GYR_CTRL_REG1_PD | GYR_CTRL_REG1_XEN | GYR_CTRL_REG1_YEN | GYR_CTRL_REG1_ZEN | (3 << GYR_CTRL_REG1_BW_SHIFT));
	// Se lee el byte de respuesta del giroscopio
	spi_read(SPI5);
	// Se levanta el pin del chip select para desactivar el giroscopio
	gpio_set(GPIOC, GPIO1); 
    
	// limpia el pin de chip select del giroscopio.
	gpio_clear(GPIOC, GPIO1); 
	// envia el comando para configurar el registro 4
	spi_send(SPI5, GYR_CTRL_REG4);
	//  recibe la respuesta del giroscopio.
	spi_read(SPI5);
	// envia el valor de la escala de medicion al registro 4
	spi_send(SPI5, (1 << GYR_CTRL_REG4_FS_SHIFT));
	// recibe la respuesta del giroscopio.
	spi_read(SPI5);
	//  establece el pin de chip select del giroscopio.
	gpio_set(GPIOC, GPIO1);
}

/*Funcion configura la USART1 para transmit*/
static void usart_setup(void)
{
	// Configura pin GPIO9, puerto A para uso como una funcion alternativa (AF) 
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);
	// Configura pin GPIO9, puerto A como AF, # 7, corresponde con la USART1 TX
	gpio_set_af(GPIOA, GPIO_AF7, GPIO9);
	// Velocidad de transmision a 115200 baudios
	usart_set_baudrate(USART1, 115200);
	// Configura # de bits de datos a 8 bits
	usart_set_databits(USART1, 8);
	// Configura el # de bits de parada a 1
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	// Configura la USART1 para transmitir datos
	usart_set_mode(USART1, USART_MODE_TX);
	// Configura la USART1 para no usar bits de paridad
	usart_set_parity(USART1, USART_PARITY_NONE);
	// Configura la USART1 para no usar control de flujo
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	// Habilita la USART1 
	usart_enable(USART1);
}

/*funcion que toma lectura de 3 dimensiones del giroscopio  x, y y z. 
 devuelve los valores en la estructura "gyro"*/
gyro read_xyz(void);
gyro read_xyz(void)
{
	gyro lectura; 						// variable tipo "gyro", que retorna la funcion
	gpio_clear(GPIOC, GPIO1); 				// Desactiva el pin 1, puerto C, que es el chip select 
	spi_send(SPI5, GYR_WHO_AM_I | 0x80);    		// Envia instruccion al dispositivo para leer el registro "WHO_AM_I"
	spi_read(SPI5); 					// Lee el valor devuelto por el dispositivo
	spi_send(SPI5, 0);					// Envia un byte vacio al dispositivo para recibir los datos del registro
	spi_read(SPI5); 					// Lee los datos del registro
	gpio_set(GPIOC, GPIO1); 				// Reactiva el pin 1, puerto C, Fin de la comunicacion
	
	/* Se repiten los pasos pero ahora para lectura del registro "STATUS_REG"*/
	gpio_clear(GPIOC, GPIO1);
	spi_send(SPI5, GYR_STATUS_REG | GYR_RNW);
	spi_read(SPI5);
	spi_send(SPI5, 0);
	spi_read(SPI5);
	gpio_set(GPIOC, GPIO1);
	/* Se repiten los pasos pero ahora para lectura del registro "OUT_TEMP"*/
	gpio_clear(GPIOC, GPIO1);
	spi_send(SPI5, GYR_OUT_TEMP | GYR_RNW);
	spi_read(SPI5);
	spi_send(SPI5, 0);
	spi_read(SPI5);
	gpio_set(GPIOC, GPIO1);

	/* Se leen los valores de los registros de salida para las tres dimensiones del giroscopio*/

	/*Codigo para lectura de Parte baja del eje x)*/
	gpio_clear(GPIOC, GPIO1);                // pin GPIO1, puerto GPIOC, se establece en bajo
	spi_send(SPI5, GYR_OUT_X_L | GYR_RNW);   // Se envia por el bus SPI5 (registro GYR_OUT_X_L (-X) con la operacion con GYR_RNW) 
	spi_read(SPI5); 			 // lectura del registro enviado 
	spi_send(SPI5, 0); 			 // Se envia un byte nulo por el bus SPI5 (inicia lectura del registro)
	lectura.x = spi_read(SPI5);              // Se lee registro enviado previamente y se guarda en "lectura.x".
	gpio_set(GPIOC, GPIO1);                  // Se establece en alto pin GPIO1 del puerto GPIOC

	/*Codigo para lectura de Parte alta del eje x)*/
	gpio_clear(GPIOC, GPIO1);                // pin GPIO1, puerto GPIOC, se establece en bajo
	spi_send(SPI5, GYR_OUT_X_H | GYR_RNW);   // Se envia por el bus SPI5 (registro GYR_OUT_X_H (X) con la operacion con GYR_RNW)
	spi_read(SPI5);                          // lectura del registro enviado 
	spi_send(SPI5, 0);                       // Se envia un byte nulo por el bus SPI5 (inicia lectura del registro)
	lectura.x |=spi_read(SPI5) << 8; 	 /* valor completo lectura del eje x (corrimiento a la izquierda y OR bit a bit 
						    con valor almacenado previamente en "lectura.x")*/ 
	gpio_set(GPIOC, GPIO1);                  // Se establece en alto pin GPIO1 del puerto GPIOC


    	/* Se repiten los pasos pero ahora para dimension Y del giroscopio */
	/*Codigo para lectura de Parte baja del eje y)*/
	gpio_clear(GPIOC, GPIO1);
	spi_send(SPI5, GYR_OUT_Y_L | GYR_RNW);
	spi_read(SPI5);
	spi_send(SPI5, 0);
	lectura.y =spi_read(SPI5);
	gpio_set(GPIOC, GPIO1);
	/*Codigo para lectura de Parte alta del eje y)*/
	gpio_clear(GPIOC, GPIO1);
	spi_send(SPI5, GYR_OUT_Y_H | GYR_RNW);
	spi_read(SPI5);
	spi_send(SPI5, 0);
	lectura.y|=spi_read(SPI5) << 8;
	gpio_set(GPIOC, GPIO1);

	/* Se repiten los pasos pero ahora para dimension Z del giroscopio */
	/*Codigo para lectura de Parte baja del eje z)*/
	gpio_clear(GPIOC, GPIO1);
	spi_send(SPI5, GYR_OUT_Z_L | GYR_RNW);
	spi_read(SPI5);
	spi_send(SPI5, 0);
	lectura.z=spi_read(SPI5);
	gpio_set(GPIOC, GPIO1);
	/*Codigo para lectura de Parte alta del eje z)*/
	gpio_clear(GPIOC, GPIO1);
	spi_send(SPI5, GYR_OUT_Z_H | GYR_RNW);
	spi_read(SPI5);
	spi_send(SPI5, 0);
	lectura.z|=spi_read(SPI5) << 8;
	gpio_set(GPIOC, GPIO1);

	/*Escalando los valores brutos leidos del giroscopio
	  multiplicar los valores brutos de los ejes x, y y z por constante (sensibilidad del giroscopio)*/ 
	lectura.x = lectura.x*L3GD20_SENSITIVITY_250DPS;
    lectura.y = lectura.y*L3GD20_SENSITIVITY_250DPS;
    lectura.z = lectura.z*L3GD20_SENSITIVITY_250DPS;
	return lectura; // Devuelve valor de tipo "gyro"; contiene los valores escalados para los tres ejes del giroscopio.
}

/* Funcion que configura los pines de los puertos GPIOG y GPIOA para uso en el microcontrolador */
static void gpio_setup(void)
{
	// Se habilitan los relojes de los puertos GPIOG y GPIOA.
	rcc_periph_clock_enable(RCC_GPIOG);
	rcc_periph_clock_enable(RCC_GPIOA);

	// Se configura pin GPIO0, puerto GPIOA, como entrada de drenaje abierto, sin R de pull-up/down
	gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);
	// Se configura pin GPIO13 del puerto GPIOG como salida con modo push-pull y sin R de pull-up/down
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13);
	// Se configura el pin GPIO14, puerto GPIOG como salida con modo push-pull y sin R de pull-up/down
	gpio_mode_setup(GPIOG, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO14);
}

/* Funcion para imprimir numeros enteros a la consola 
Tomada de SPI- F4 */
int print_decimal(int);
int print_decimal(int num)
{
	int	ndx = 0; 		// Inicializa una variable indice
	char	buf[10];		// Inicializa un buffer para los caracteres del entero
	int	len = 0;            	// Inicializa un contador para el numero de caracteres impresos
	char	is_signed = 0;      	// Inicializa una bandera para indicar si el entero es negativo

	/* Comprueba si el entero es negativo */
	if (num < 0) {              
		is_signed++;            // Establece la bandera
		num = 0 - num;          // Convierte el entero a su valor absoluto
	}
	buf[ndx++] = '\000';        		// Define primer caracter del buffer como nulo
	do {                                 	// ciclo do-while.
		buf[ndx++] = (num % 10) + '0';  // Extrae cada digito del entero y lo agrega al buffer
		num = num / 10;                 // Dividir entre 10 para obtener la siguiente cifra decimal
	} 
	
	while (num != 0);                    // Repite el proceso hasta que el entero sea 0
	ndx--;                               // Decrementa la variable indice
	if (is_signed != 0) {                // Si el entero es negativo, imprime un signo menos
		console_putc('-');           // Imprimir el signo -
		len++;                       // Incrementa el contador
	}
	while (buf[ndx] != '\000') {         // Imprime los caracteres del buffer en orden inverso
		console_putc(buf[ndx--]);    // Imprimir el caracter almacenado en posicion ndx del buffer 
		len++;                       // Incrementa el contador
	}
	return len;  			     // Retorna el numero de caracteres impresos
}

/*funcion que configura el ADC (convertidor analogico-digital) en el microcontrolador*/
static void adc_setup(void)
{
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO1);   // pin PA1 (GPIO1) como entrada analogica
	adc_power_off(ADC1); 						   // apaga el ADC1
	adc_disable_scan_mode(ADC1);					   // deshabilita modo de escaneo en el ADC1.
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_3CYC);      // tiempo de muestreo en 3 ciclos de reloj para canales
	adc_power_on(ADC1);                                                // ADC1 encendido, listo para tomar muestras

}

/* Funcion para lectura del valor analogico en el pin seleccionado por canal ADC1 */
static uint16_t read_adc_naiive(uint8_t channel)
{
	uint8_t channel_array[16];   			    // Se crea un arreglo de 16 elementos
	channel_array[0] = channel;                         // Se asigna el valor de channel al primer elemento arreglo
	adc_set_regular_sequence(ADC1, 1, channel_array);   // Configuracion secuencia del ADC1 para leer canal especificado por channel
	adc_start_conversion_regular(ADC1);                 // Se inicia la conversion de la secuencia regular del ADC1
	while (!adc_eoc(ADC1));                             // Se espera que la conversion se complete con funcion adc_eoc
	uint16_t reg16 = adc_read_regular(ADC1);            // Para leer resultado de la conversion de la secuencia  del ADC1
	return reg16;                                       // Devuelve el resultado de la conversion como entero sin signo de 16 bit
}

/*Funcion main */
int main(void)
{	
	/* Declaracion de variables y estructuras */
	gyro lectura;              // Estructura con valores de lectura de los tres ejes 
	float nivel_bateria;       // variable para almacenar el nivel de bateria   
	char * SerialUSB = "OFF";  // variable inicial para imprimir estado de serial/USB
	char data[10] = ""; 	   // Inicializa el arreglo "data" con una cadena vacia
	
	// arreglos de caracteres que almacena la lectura del eje X, Y, Z y nivel de bateria
	char print_x[5];       
	char print_y[5];        
	char print_z[5];
	char print_PILA[5];
	char print_ONOFF[3];
	// Valores iniciales para las variables leidas, se declaran para no contener datos basura
	lectura.x = 0;
    lectura.y = 0;
    lectura.z = 0;
	nivel_bateria = 0;
	bool enviar = false; 			// Variable para indicar si se envian los datos
	uint16_t input_adc0; 			// Variable para almacenar el valor de entrada del ADC0
	
	/* llamado de las funciones de configuracion de los perifericos */
	console_setup(115200);                  // Configuracion de la consola 
	clock_setup();                          // Configuracion del reloj 
	rcc_periph_clock_enable(RCC_USART1);    // Habilitacion del reloj del periferico USART1
	rcc_periph_clock_enable(RCC_ADC1);      // Habilitacion del reloj del periferico ADC1
	gpio_setup();                           // Configuracion de los pines del microcontrolador
	adc_setup();                            // Configuracion del ADC1
	sdram_init();				// Inicializacion de la memoria SDRAM
	usart_setup();				// Configuracion del periferico USART1
	spi_setup();				// Configuracion del bus SPI
	lcd_spi_init();				// Inicializacion del controlador de la pantalla LCD
	gfx_init(lcd_draw_pixel, 240, 320);     // Inicializacion de libreria grafica para LCD

	while (1) {
		
		/* formateando variables en cadenas de caracteres
		para  imprimirlas en LCD */

		// formateo para "lectura.x"
		sprintf(print_x, "%s", "X:");       // Escribir el string "X:" en la cadena print_x
		sprintf(data, "%d",  lectura.x);    // Convertir lectura.x a STR y almacenarlo en cadena data
		strcat(print_x, data);              // Agrega el contenido de la cadena data al final de la cadena print_x

		// formateo para "lectura.y"
		sprintf(print_y, "%s", "Y:");
		sprintf(data, "%d",  lectura.y);
		strcat(print_y, data);
		
		// formateo para "lectura.z"
		sprintf(print_z, "%s", "Z:");
		sprintf(data, "%d",  lectura.z);
		strcat(print_z, data);
		
		// formateo para "nivel_bateria"
		sprintf(print_PILA, "%s", "");
		sprintf(data, "%f",  nivel_bateria);
		strcat(print_PILA, data);

		// formateo para "Serial/USB"
		sprintf(print_ONOFF, "%s", "");
		sprintf(data, "%s",  SerialUSB);
		strcat(print_ONOFF, data);

		/* Bloque para visualizar informacion en la pantalla LCD */
		gfx_fillScreen(LCD_BLACK);     		  // llenar la pantalla con color negro.
		gfx_setTextColor(LCD_WHITE, LCD_BLACK);   // Establece el color de fondo a negro y el color del texto en verde.
		//Desplagar en pantalla
		gfx_setTextSize(2);                       // Size del texto
		gfx_setCursor(5, 15);                     // posiciona el cursor en la pantalla
		gfx_puts("SISMOGRAFO");                   // Immprime el texto
		gfx_setTextColor(LCD_BLUE, LCD_BLACK);
		gfx_setCursor(180, 15);
		gfx_puts("UCR");
		gfx_setTextColor(LCD_YELLOW, LCD_BLACK);
		gfx_setCursor(80, 50);
		gfx_puts("Ejes");
		gfx_setTextSize(1.8);
		gfx_setCursor(70, 75);
		gfx_puts("Giroscopio");

		gfx_setTextColor(LCD_GREEN, LCD_BLACK);
		gfx_setTextSize(2);
		gfx_setCursor(80, 100);
		gfx_puts(print_x);                        // Imprime contenido de variable
		gfx_setCursor(80, 130);
		gfx_puts(print_y);
		gfx_setCursor(80, 160);
		gfx_puts(print_z);

		gfx_setTextColor(LCD_YELLOW, LCD_BLACK);
		gfx_setCursor(55, 185);
		gfx_puts("Bateria:");
		gfx_setTextColor(LCD_GREEN, LCD_BLACK);
		gfx_setCursor(25, 215);
		gfx_puts(print_PILA);
		gfx_setCursor(180, 215);
		gfx_puts("V");
		gfx_setTextColor(LCD_YELLOW, LCD_BLACK);
		gfx_setCursor(5, 255);
		gfx_puts("Serial/USB:");
       	gfx_setTextColor(LCD_GREEN, LCD_BLACK);
		gfx_setCursor(180, 255);
		gfx_puts(SerialUSB);   

		/* llamado para mostrar los cambios en la pantalla fisica */
		lcd_show_frame();                 
		gpio_clear(GPIOC, GPIO1);  		// apaga el LED del pin PC1, indica datos siendo enviados
		lectura = read_xyz();      		// lee los datos del giroscopio
		gpio_set(GPIOC, GPIO1);                 // enciende el LED para indicar que transmision ha finalizado
		input_adc0 = read_adc_naiive(1);        // Se lee el valor del ADC del canal 1 y se guarda en la variable input_adc0.
		
		/* Calculo del nivel de bateria segun valor del ADC
		8.64 se utiliza para convertir el valor del ADC a voltios asumiendo 3.3V / resolucion de 12 bits
		Constante 510 es valor arbitrario elegido para ajustar la escala a bateria 8.64 V, 
		que inicia con ese voltaje. */ 
		nivel_bateria = ((input_adc0*8.64)/510);  // para lectura del valor ADC y calcular el nivel de bateria actual
	
		/* Para enviar las lectura del giroscopio y el nivel de bateria 
		por puerto serie (USART1)*/
		
		//  Sucede si variable booleana enviar es verdadera
		if (enviar)                               
		{
			/*  LLamada funcion print_decimal para imprimir la lecturas 
			ejes y nivel de bateria */
			SerialUSB = "ON";            // Indica si puerto funciona
			print_decimal(lectura.x);             
			console_puts("\t");
       	 		print_decimal(lectura.y);
			console_puts("\t");
        		print_decimal(lectura.z); 
			console_puts("\t");
			print_decimal(nivel_bateria); 
			console_puts("\n");
			// Toggle del pin 13, puerto G, parpadeo indica envio exitoso 
			gpio_toggle(GPIOG, GPIO13);     
		}
		//  Sucede si variable booleana enviar es falsa
		else{                                     
			SerialUSB = "OFF";        // Indica si puerto no funciona
			// apagando el LED del pin 13, puerto G
			gpio_clear(GPIOG, GPIO13);             
		}

		/* Verifica si el nivel de bateria es menor a 7 */
		if (nivel_bateria<7)
		{   
			// Toggle del pin 14, puerto G, enciende led, indica bateria baja
			gpio_toggle(GPIOG, GPIO14);
		}

		else gpio_clear(GPIOG, GPIO14); // Apaga led pin 14, puerto G
		

		/* Si se detecta que GPIO0 esta en un estado logico alto (1) */  
		if (gpio_get(GPIOA, GPIO0)) {      
			// Si enviar es verdadero, se cambia a falso y apaga el LED conectado al pin 13
			if (enviar) {
				enviar = false;
				gpio_clear(GPIOG, GPIO13);
			}
			// Si enviar es falso, se cambia a verdadero y se enciende el LED conectado al pin 13
			else enviar = true;
		}

		/* Implementacion para delay con ciclo for*/
		int i;
		for (i = 0; i < 80000; i++)    /* Wait a bit. */
			__asm__("nop");
		
	}
	return 0;
}
