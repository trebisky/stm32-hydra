/* led.c
 * (c) Tom Trebisky  12-2-2020
 *
 * Unified LED driver for Hydra
 */

#include "hydra.h"

#ifdef CHIP_F411
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

	gpio_output_od_config ( LED_GPIO, LED_PIN );
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

/* THE END */
