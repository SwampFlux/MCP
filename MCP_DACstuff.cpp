#include "NextTuesday_RP2040_Board.h"

#include "pico/time.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "hardware/exception.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "spi.pio.h"
#include "DAC80501.h"

enum
{
    LSBFIRST,
    MSBFIRST
};

// #define NOPIO

PIO dac_pio = pio1;
uint dac_sm = 0;

static inline void DAC_init(PIO pio, uint sm, uint data_pin, uint clk_pin, uint cs_pin1, uint cs_pin2, float clk_div)
{
#ifdef NOPIO
  SetOut(DAC_DAT);
  SetOut(DAC_SCK);
  SetOut(DAC_CS_NOTE);
  SetOut(DAC_CS_VEL);
#else
    uint offset = pio_add_program(pio, &spi_on_pio_program);

    pio_gpio_init(pio, data_pin);
    pio_gpio_init(pio, clk_pin);
    pio_gpio_init(pio, cs_pin1);
    pio_gpio_init(pio, cs_pin2);
    pio_sm_set_consecutive_pindirs(pio, sm, data_pin, 1, true);
    pio_sm_set_consecutive_pindirs(pio, sm, clk_pin, 3, true);
    pio_sm_config c = spi_on_pio_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, clk_pin);
    sm_config_set_out_pins(&c, data_pin, 1);
    // sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_clkdiv(&c, clk_div);
    sm_config_set_out_shift(&c, false, false, 24);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
#endif
};

static inline void dac_put_32(PIO pio, uint sm, uint32_t x)
{
    while (pio_sm_is_tx_fifo_full(pio, sm))
        ;
    *(volatile uint32_t *)&pio->txf[sm] = x;
}


void NOP()
{
    asm volatile("nop \n nop \n nop");
}

void shiftOut(uint8_t bitOrder, uint8_t val)
{
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        if (bitOrder == LSBFIRST)
            gpio_put(DAC_DAT, !!(val & (1 << i)));
        else
            gpio_put(DAC_DAT, !!(val & (1 << (7 - i))));
        NOP();
        NOP();
        gpio_put(DAC_SCK, true);
        NOP();
        NOP();
        gpio_put(DAC_SCK, false);
    }
}

void SendSPIByte(uint8_t c)
{
    gpio_put(DAC_DAT, ((c & 0x80) > 0) ? true : false);
    gpio_put(DAC_SCK, true);
    sleep_us(1);
    gpio_put(DAC_SCK, false);
    gpio_put(DAC_DAT, ((c & 0x40) > 0) ? true : false);
    gpio_put(DAC_SCK, true);
    sleep_us(1);
    gpio_put(DAC_SCK, false);
    gpio_put(DAC_DAT, ((c & 0x20) > 0) ? true : false);
    gpio_put(DAC_SCK, true);
    sleep_us(1);
    gpio_put(DAC_SCK, false);
    gpio_put(DAC_DAT, ((c & 0x10) > 0) ? true : false);
    gpio_put(DAC_SCK, true);
    sleep_us(1);
    gpio_put(DAC_SCK, false);
    gpio_put(DAC_DAT, ((c & 0x08) > 0) ? true : false);
    gpio_put(DAC_SCK, true);
    sleep_us(1);
    gpio_put(DAC_SCK, false);
    gpio_put(DAC_DAT, ((c & 0x04) > 0) ? true : false);
    gpio_put(DAC_SCK, true);
    sleep_us(1);
    gpio_put(DAC_SCK, false);
    gpio_put(DAC_DAT, ((c & 0x02) > 0) ? true : false);
    gpio_put(DAC_SCK, true);
    sleep_us(1);
    gpio_put(DAC_SCK, false);
    gpio_put(DAC_DAT, ((c & 0x01) > 0) ? true : false);
    gpio_put(DAC_SCK, true);
    sleep_us(1);
    gpio_put(DAC_SCK, false);
}

void DacSend32(uint32_t x)
{
    while (pio_sm_is_tx_fifo_full(dac_pio, dac_sm))
    {
    };
    *(volatile uint32_t *)&dac_pio->txf[dac_sm] = x;
}

void SendCommand(int CSPin, DACCommand *c)
{
    //  shiftOut(MSBFIRST, c->Bytes[0]);
    //  shiftOut(MSBFIRST, c->Bytes[1]);
    // shiftOut(MSBFIRST, c->Bytes[2]);

#ifdef NOPIO
    gpio_put(CSPin, false);
    NOP();
    shiftOut(MSBFIRST, c->Bytes[0]);
    shiftOut(MSBFIRST, c->Bytes[1]);
    shiftOut(MSBFIRST, c->Bytes[2]);
    NOP();
    gpio_put(CSPin, true);
#else
    DacSend32((c->Bytes[0] << 24) + (c->Bytes[1] << 16) + (c->Bytes[2] << 8));
    // DacSend32((c->Bytes[0] << 16) + (c->Bytes[1] << 8) + (c->Bytes[2] << 0));
#endif
    
    //  gpio_put(CSPin, true);
}

DACCommand DataCommand;
void DacSetup()
{
    DAC_init(dac_pio, dac_sm, DAC_DAT, DAC_SCK, DAC_CS_NOTE, DAC_CS_VEL, 1);
    DataCommand.DAC_DATA(0);
    DACCommand Com[10];
    Com[0].GAIN(true, false);
    //  Com[1].CONFIG(true,true);
    Com[1].CONFIG(false, false);
    for (int i = 0; i < 2; i++)
    {
        SendCommand(DAC_CS_NOTE, &Com[i]);
        SendCommand(DAC_CS_VEL, &Com[i]);
    }
}

void DacSetBoth(uint16_t value0, uint16_t value1)
{
    DataCommand.UpdateData(value0);
    SendCommand(DAC_CS_NOTE, &DataCommand);
    DataCommand.UpdateData(value1);
    SendCommand(DAC_CS_VEL, &DataCommand);
}