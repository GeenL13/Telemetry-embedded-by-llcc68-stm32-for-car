/*
 * Lora.c
 *
 *  Created on: Jan 24, 2026
 *      Author: Geen
 */


#include "llcc68_app.h"
#include "stm32f1xx_hal.h"

enum lora_state
{
	lora_state_ok,
	lora_state_error
};

