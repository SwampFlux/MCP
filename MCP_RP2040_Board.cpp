#include "MCP_RP2040_Board.h"
#include "MCP_Main.h"

#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/sync.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"

int togglebit(int bitidx, int baseval)
{
  return baseval ^= 1UL << bitidx;
}
#ifndef EMU
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
#endif

#include "MCP_RP2040_Board.h"

#include <math.h>

uint16_t ADC_2 = 0;
uint16_t ADC_3 = 0;

#ifndef EMU

PIO LED_pio = pio0;
uint LED_sm = 1;

uint adc_dma_chan;
dma_channel_config adc_dma_cfg;
uint adc_handled = 0;

#endif

int adc[4] = {0, 0, 0, 0};

uint32_t LEDDATA[MCP_LED_COUNT] = {0};

#ifndef WIN32
#include "ws2812.pio.h"
#define IS_RGBW false
#define NUM_PIXELS 90
#define WS2812_PIN 13

void LED_init()
{
  pio_add_program(LED_pio, &write_neopixel_program);
  write_neopixel_program_init(LED_pio, LED_sm, WS2812_PIN);
  // uint offset = pio_add_program(LED_pio, &ws2812_program);
  // ws2812_program_init(LED_pio, LED_sm, offset, WS2812_PIN, 800000, IS_RGBW);
}

uint16_t adc_capture_buf[8] = {0};
int curmux = 0;

void SetupADCMux(int newmux)
{
//  gpio_put_masked((1 << MUX_A) + (1 << MUX_B), newmux << MUX_A);
  curmux = newmux;
}

void CycleADCMux()
{
  adc[0] = adc[0] / 2 + adc_capture_buf[0] * 8;
  adc[1] = adc[1] / 2 + adc_capture_buf[1] * 8;
  adc[2] = adc[2] / 2 + adc_capture_buf[2] * 8;
  adc[3] = adc[3] / 2 + adc_capture_buf[3] * 8;
  SetupADCMux((curmux + 1) % 4);
}

void __no_inline_not_in_flash_func(adc_dma_handler)()
{
  adc_run(false);
  adc_fifo_drain();
  adc_select_input(0);
  CycleADCMux();
  dma_channel_acknowledge_irq0(adc_dma_chan);
  adc_handled++;
  /* adc_handled++;
   if (adc_handled >= 4)
   {
     adc_handled =0;
     adc_select_input(0);
   }*/
  dma_channel_set_write_addr(adc_dma_chan, &adc_capture_buf, true);

  adc_run(true);
}

void SetupADCSystem()
{
  adc_init();

  adc_set_round_robin(0xf);

  adc_gpio_init(26);
  adc_gpio_init(27);
  // adc_gpio_init(28);
  adc_gpio_init(29);
  adc_select_input(0);
  SetupADCMux(0);

  adc_fifo_setup(true, true, 1, false, false);

  adc_dma_chan = dma_claim_unused_channel(true);
  adc_dma_cfg = dma_channel_get_default_config(adc_dma_chan);

  channel_config_set_transfer_data_size(&adc_dma_cfg, DMA_SIZE_16);
  channel_config_set_read_increment(&adc_dma_cfg, false);
  channel_config_set_write_increment(&adc_dma_cfg, true);
  channel_config_set_dreq(&adc_dma_cfg, DREQ_ADC);

  dma_channel_configure(adc_dma_chan, &adc_dma_cfg, adc_capture_buf, &adc_hw->fifo, 4, false);

  float adc_targetfreq = 4000;
  float adc_div = 48000000 / adc_targetfreq;
  adc_set_clkdiv(adc_div);

  // Tell the DMA to raise IRQ lnine 0 when the channel finishes a block
  dma_channel_set_irq0_enabled(adc_dma_chan, true);

  // Configure the processor to run dma_handler() when DMA IRQ 0 is asserted
  irq_add_shared_handler(DMA_IRQ_0, adc_dma_handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
  irq_set_enabled(DMA_IRQ_0, true);

  adc_dma_handler();
  while (adc_handled < 30)
    sleep_ms(1);
}
#else
void SetupADCSystem()
{
}
#endif

// I2C reserves some addresses for special purposes. We exclude these from the scan.
// These are any addresses of the form 000 0xxx or 111 1xxx
bool reserved_addr(uint8_t addr)
{
  return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

#include <stdio.h>
#include "pico/stdlib.h"

void SetOut(int p)
{
  gpio_init(p);
  gpio_set_dir(p, GPIO_OUT);
}

void SetIn(int p, bool pullup)
{
  gpio_init(p);
  gpio_set_dir(p, GPIO_IN);
  if (pullup)
    gpio_pull_up(p);
}

void SetADC(int p)
{
  adc_gpio_init(p);
}

int LED_DMA_channel;
dma_channel_config LED_DMA_data;

void Setup_LED_DMA()
{

  LED_DMA_channel = dma_claim_unused_channel(true);
  LED_DMA_data = dma_channel_get_default_config(LED_DMA_channel);
  channel_config_set_transfer_data_size(&LED_DMA_data, DMA_SIZE_32);
  channel_config_set_read_increment(&LED_DMA_data, true);
  channel_config_set_write_increment(&LED_DMA_data, false);
  channel_config_set_dreq(&LED_DMA_data, pio_get_dreq(LED_pio, LED_sm, true)); // DREQ_PIO0_TX0

  dma_channel_configure(
      LED_DMA_channel,         // Channel to be configured
      &LED_DMA_data,           // The configuration we just created
      &LED_pio->txf[LED_sm],   // The initial write address
      LEDDATA,                 // The initial read address
      MCP_LED_COUNT, // Number of transfers; in this case each is 1 byte.
      false                    // Start immediately.
  );

  dma_channel_start(LED_DMA_channel);
}

void Fire_LED_DMA()
{

  dma_channel_set_read_addr(LED_DMA_channel, LEDDATA, true);
}

extern void BuildCustomGradient();
void ARGHHARD(void)
{
  printf("argh hardfault!");
}

void ARGHNMI(void)
{
  printf("argh NMI!");
}

void SetupBoard()
{
  exception_set_exclusive_handler(HARDFAULT_EXCEPTION, ARGHHARD);
  exception_set_exclusive_handler(NMI_EXCEPTION, ARGHNMI);

  stdio_init_all();

  DacSetup();
  DacSetBoth(32768,32768);
  SetOut(LEDSTRING);
  LED_init();

  Setup_LED_DMA();


  adc_init();

  SetADC(26);
  SetADC(27);
  SetADC(28);
  SetADC(29);
//  SetADC(ADC_0);
//  SetADC(ADC_1);

  adc_select_input(0);

  SetupADCSystem();
}

void RunBoardTest()
{
  // while(1)
  // {

  //}
}

