
#include "MCP_RP2040_Board.h"

#ifndef EMU
#include "pico/time.h"
#include "pico/bootrom.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/exception.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#endif
#include <math.h>

int watchdog = 0;

void CheckButtons()
{
	//	AlgoButton = debounce(BUTTON_ALGO, &algosw_state);
	//	ScaleButton = debounce(BUTTON_SCALE, &scalesw_state);
}

void StartupSequence()
{
}

int LedFPStimer = 0;
float tt = 0;
bool __no_inline_not_in_flash_func(ledtimer)(struct repeating_timer *t)
{
    tt += 0.1f;
	if(adc[2]>0x8000)
	{
		adc[2]=0x8000;
	}
    float Bmult = (65535.0f - adc[2]) * (20.0f/65535.0f)-(10.0f);
    float Gmult = (65535.0f - adc[3]) * (20.0f/65535.0f);
	float Rmult = adc[0] * (20.0f/65535.0f);
    for (int i = 0; i < MCP_LED_COUNT; i++)
    {
        //LEDDATA[i] = RGB32(sinf(i * 0.21 + tt) * 127 + 127, sinf(i * 0.2 + tt) * 127 + 127, sinf(i * 0.214 + tt) * 127 + 127);

        LEDDATA[i] = RGB32(sinf(i * 0.21 + tt*2.2) * Rmult + Rmult , sinf(i * 0.2 + tt*1.123) * Gmult + Gmult , sinf(i * 0.214 + tt*0.7) * Bmult  + Bmult);
    }
    Fire_LED_DMA();
    return true;
}

void core1_synthentry()
{
	struct repeating_timer timer;
	add_repeating_timer_ms(30, ledtimer, NULL, &timer);
	while (true)
	{

		tight_loop_contents();
	}
}
bool __no_inline_not_in_flash_func(repeating_timer_callback)(struct repeating_timer *t)
{

	watchdog = 0;

	CheckButtons();
	return true;
}

int const usecpersample = 1000000 / SAMPLE_RATE;
extern int wsmain();
extern struct Tuesday_RandomGen MainRandom;
extern long oldseed;
extern int tickssincecommit;

void DoTimeThing()
{
	repeating_timer_callback(0);
}

int main()
{
	// return wsmain();
	SetupBoard();
	StartupSequence();
	for (int i = 0; i < 20; i++)
		repeating_timer_callback(0);
	multicore_launch_core1(core1_synthentry);

	SetupTimerInt();

	int maxwatchdog = 0;

	while (1)
	{

		if (watchdog > maxwatchdog)
			maxwatchdog = watchdog;
		if (watchdog > 10000)
		{
			//	printf("gotcha!");
			watchdog = 0;
			maxwatchdog = 0;
		}

		tight_loop_contents();
	}
}
