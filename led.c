/* led.c
 * (c) Tom Trebisky  12-2-2020
 *
 * Unified LED driver for Hydra
 */

#include "hydra.h"

#ifdef CHIP_F429
/* F429 discovery */
#define LED_GPIO	GPIOG
#define LED_PIN		13
#define LED_GREEN	13
#define LED_RED		14
#else
/* Black Pill */
#define LED_GPIO	GPIOC	/* PC13 */
#define LED_PIN		13	/* PC13 */
#endif

/* Just here for reference */
#ifdef CHIP_MAPLE
#define LED_GPIO	GPIOA	/* PA0 */
#define LED_PIN		0	/* PA0 */
#endif

#ifdef CHIP_F103
#define LED_GPIO	GPIOC	/* PC13 */
#define LED_PIN		13	/* PC13 */
#endif

void
led_init ( void )
{
	// gpio_mode ( LED_GPIO, LED_PIN, 1 );
	// gpio_otype ( LED_GPIO, LED_PIN, 1 );

	gpio_led_pin_setup ( LED_GPIO, LED_PIN );
#ifdef CHIP_F429
	gpio_led_pin_setup ( LED_GPIO, LED_RED );
	gpio_led_pin_setup ( LED_GPIO, LED_GREEN );
#endif
}

void
led_on ( void )
{
	gpio_bit ( LED_GPIO, LED_PIN, 1 );
}

void
led_off ( void )
{
	gpio_bit ( LED_GPIO, LED_PIN, 0 );
}

#ifdef CHIP_F429
void red_on ( void ) { gpio_bit ( LED_GPIO, LED_RED, 1 ); }
void red_off ( void ) { gpio_bit ( LED_GPIO, LED_RED, 0 ); }
void green_on ( void ) { gpio_bit ( LED_GPIO, LED_GREEN, 1 ); }
void green_off ( void ) { gpio_bit ( LED_GPIO, LED_GREEN, 0 ); }
#else
void red_on ( void ) { }
void red_off ( void ) { }
void green_on ( void ) { gpio_bit ( LED_GPIO, LED_PIN, 1 ); }
void green_off ( void ) { gpio_bit ( LED_GPIO, LED_PIN, 0 ); }
#endif

/* THE END */
