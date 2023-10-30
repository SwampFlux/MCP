#pragma once
#ifndef MCPRP2040BOARD
#define MCPRP2040BOARD

#include <stdint.h>

#define LEDSTRING 13
#define DAC_SDI 0
#define DAC_SCK 1
#define DAC_CS 2

extern int adc[4];

//int debounce (int gpio, debounce_t *b);
#define RGB32(R,G,B) (uint32_t)((((int)(G))<<24)+(((int)(R))<<16) + (((int)(B))<<8))

void SetupDACs();
void DacSetup();
void DacSetBoth(uint16_t value0, uint16_t value1);
void SetupBoard();
void RunBoardTest();
void SetOut(int p);
void SetIn(int p, bool PullUp = false);

#define MCP_LED_COUNT 90
#define SAMPLE_RATE 44100
extern uint32_t LEDDATA[MCP_LED_COUNT];
void Fire_LED_DMA();
void SetupTimerInt();

#endif
