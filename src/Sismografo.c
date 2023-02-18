// Agregamos Bibliotecas
#include <stdio.h>
#include <string.h> 
#include <stdint.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
// Agregamos para el uso de bibliotecas de proyecto libopencm3
#include "clock.h"
#include "console.h"
#include "sdram.h"
#include "lcd-spi.h"
#include "gfx.h"

#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>


/* Funcion para imprimir numeros enteros a la consola 
Tomada de SPI- F4 */
int print_decimal(int);

/*
 * int len = print_decimal(int value)
 *
 * Very simple routine to print an integer as a decimal
 * number on the console.
 */
int
print_decimal(int num)
{
	int		ndx = 0;
	char	buf[10];
	int		len = 0;
	char	is_signed = 0;

	if (num < 0) {
		is_signed++;
		num = 0 - num;
	}
	buf[ndx++] = '\000';
	do {
		buf[ndx++] = (num % 10) + '0';
		num = num / 10;
	} while (num != 0);
	ndx--;
	if (is_signed != 0) {
		console_putc('-');
		len++;
	}
	while (buf[ndx] != '\000') {
		console_putc(buf[ndx--]);
		len++;
	}
	return len; /* number of characters printed */
}


/*funcion que configura el ADC (convertidor analogico-digital) en el microcontrolador*/
static void adc_setup(void)
{
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO0);
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO1);

	adc_power_off(ADC1);
	adc_disable_scan_mode(ADC1);
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_3CYC);

	adc_power_on(ADC1);

}

/* Funcion para lectura del valor analogico en el pin seleccionado por canal ADC1 */
static uint16_t read_adc_naiive(uint8_t channel)
{
	uint8_t channel_array[16];
	channel_array[0] = channel;
	adc_set_regular_sequence(ADC1, 1, channel_array);
	adc_start_conversion_regular(ADC1);
	while (!adc_eoc(ADC1));
	uint16_t reg16 = adc_read_regular(ADC1);
	return reg16;
}


int main(void)
{
	clock_setup();
	console_setup(115200);
	sdram_init();
	lcd_spi_init();
	gfx_init(lcd_draw_pixel, 240, 320);
	
	// arreglos de caracteres que almacena la lectura del eje X, Y, Z y nivel de bateria
	char print_x[5];       
	char print_y[5];        
	char print_z[5];
	char print_PILA[5];
	char print_ONOFF[3];
	
	// Valores iniciales para las variables leidas, se declaran para no contener datos basura
	int x = 110;             // variable inicial para imprimir lectura x
    	int y = 120;             // variable inicial para imprimir lectura y
    	int z = 130;	       // variable inicial para imprimir lectura z
	float nivel_bateria = 7;       // variable inicial para imprimir lectura z
	char * SerialUSB = "OFF";      // variable inicial para imprimir estado de serial/USB
	char data[10] = ""; 	       // Inicializa el arreglo "data" con una cadena vacia
	
	while (1) {
		/* formateando variables en cadenas de caracteres
		para  imprimirlas en LCD */

		// formateo para "x"
		sprintf(print_x, "%s", "X:");       // Escribir el string "X:" en la cadena print_x
		sprintf(data, "%d",  x);    // Convertir lectura.x a STR y almacenarlo en cadena data
		strcat(print_x, data);              // Agrega el contenido de la cadena data al final de la cadena print_x

		// formateo para "y"
		sprintf(print_y, "%s", "Y:");
		sprintf(data, "%d",  y);
		strcat(print_y, data);
		
		// formateo para "z"
		sprintf(print_z, "%s", "Z:");
		sprintf(data, "%d",  z);
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
		gfx_puts("SISMOGRAFO");                   // Imprime el texto
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
	}
}
