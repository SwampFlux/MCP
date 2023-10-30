#include "MCP_RP2040_Board.h"

#include "pico/time.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "hardware/exception.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"



#include "timerint.pio.h"


PIO Timer_pio = pio0;
uint Timer_sm = 2;

extern void DoTimeThing();
void timer_isr()
{
    
    pio_interrupt_clear(Timer_pio, 0);
    irq_clear(PIO0_IRQ_0);
    DoTimeThing();
}

void SetupTimerInt()
{
    irq_set_exclusive_handler(PIO0_IRQ_0, timer_isr);
    irq_set_enabled(PIO0_IRQ_0, true);

    pio_set_irq0_source_enabled(Timer_pio, pis_interrupt0, true);

    uint offset = pio_add_program(Timer_pio, &timerint_program);

    pio_sm_config c =timerint_program_get_default_config(offset);
    float clk_div = 133000000.0f/(float)(SAMPLE_RATE);
    
    sm_config_set_clkdiv(&c, clk_div);
    sm_config_set_out_shift(&c, false, false, 24);

    pio_sm_init(Timer_pio, Timer_sm, offset, &c);
    pio_sm_set_enabled(Timer_pio, Timer_sm, true);


}