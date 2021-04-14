#include <stdint.h>
#include <stdlib.h> /* random */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "ws2812.c"
#include "noise.c"

#define WIDTH               10
#define HEIGHT              10
#define PIXELS                (WIDTH * HEIGHT)
#define BYTES                 (3 * PIXELS)

typedef struct { uint8_t R, G, B; } Color;

/* graphics */
Color black = { 0, 0, 0 };
static double time = 0;

static uint8_t _pixels[BYTES];
static void pixel(uint8_t x, uint8_t y, Color *color);
static void hsl_to_rgb(double h, double s, double l, Color *c);
static double color_component(double temp1, double temp2, double temp3);

int main(void)
{
	/* 1 ms timer, prescaler 64 */
	TCCR0A = (1 << WGM01);
	TCCR0B = (1 << CS02) | (1 << CS00);
	TIMSK0 = (1 << OCIE0A);
	OCR0A = 156;
	asm volatile("sei");

	/*clear(&black);
	Color red = { 255, 0, 0 };
	pixel(0, 0, &red);*/

	while(1) ;
	return 0;
}

static void hsl_to_rgb(double h, double s, double l, Color *c)
{
	double tR = 0, tG = 0, tB = 0, temp1, temp2;
	if(l != 0)
	{
		if(s == 0)
		{
			tR = tG = tB = l;
		}
		else
		{
			if(l < 0.5)
			{
				temp2 = l * (1.0 + s);
			}
			else
			{
				temp2 = l + s - (l * s);
			}

			temp1 = 2.0 * l - temp2;
			tR = color_component(temp1, temp2, h + 1.0 / 3.0);
			tG = color_component(temp1, temp2, h);
			tB = color_component(temp1, temp2, h - 1.0 / 3.0);
		}
	}

	c->R = tR * 255;
	c->G = tG * 255;
	c->B = tB * 255;
}

static double color_component(double temp1, double temp2, double temp3)
{
	if(temp3 < 0.0)
	{
		temp3 += 1.0;
	}
	else if(temp3 > 1.0)
	{
		temp3 -= 1.0;
	}

	if(temp3 < 1.0 / 6.0)
	{
		return temp1 + (temp2 - temp1) * 6.0 * temp3;
	}
	else if(temp3 < 0.5)
	{
		return temp2;
	}
	else if(temp3 < 2.0 / 3.0)
	{
		return temp1 + ((temp2 - temp1) * ((2.0 / 3.0) - temp3) * 6.0);
	}
	else
	{
		return temp1;
	}
}

static void pixel(uint8_t x, uint8_t y, Color *color)
{
	if(x < WIDTH && y < HEIGHT)
	{
		uint16_t i;
		x = (WIDTH - 1) - x;
		y = (HEIGHT - 1) - y;
		i = 3 * ((y % 2)
			? ((WIDTH * y) + ((WIDTH - 1) - x)) : (WIDTH * y + x));

		_pixels[i] = color->G;
		_pixels[++i] = color->R;
		_pixels[++i] = color->B;
	}
}

ISR(TIMER0_COMPA_vect)
{
	double v;
	Color c;
	uint8_t x, y;
	for(x = 0; x < WIDTH; x++)
	{
		for(y = 0; y < HEIGHT; y++)
		{
			v = Noise((double)x / WIDTH, (double)y / HEIGHT, time);
			hsl_to_rgb(v, 1, 0.5, &c);
			pixel(x, y, &c);
		}
	}

	time += 0.01;
	ws2812(_pixels, BYTES);
}
