#pragma once
#include <stdint.h>

#ifdef WIN32
#define NOINLINE
#else
#define NOINLINE __attribute__((noinline))
#endif

#ifndef __min
#define __min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef __max
#define __max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#define SAMPLE_RATE 44100
#define TUESDAYRATE 2000
#define SAMPLESPERTUESDAYUPDATE (SAMPLE_RATE/TUESDAYRATE)
#define MSEC(x) (x * (SAMPLE_RATE/1000))

#define TUESDAY_MAXBEAT_VAL 32
#define TUESDAY_MAXTPB_VAL 10
#define TUESDAY_MAXTICK (TUESDAY_MAXTPB_VAL * TUESDAY_MAXBEAT_VAL)

#define TUESDAY_GATES 6

#define GATE_TICK 4
#define GATE_CLOCK 5
#define GATE_BEAT 3
#define GATE_LOOP 2
#define GATE_ACCENT 1
#define GATE_GATE 0
#define GATE_MINGATETIME 10

#define TUESDAY_NOTEOFF -100
#define TUESDAY_SUBTICKRES 6
#define TUESDAY_DEFAULTGATELENGTH ((TUESDAY_SUBTICKRES * 3) / 4)


#define VERSIONBYTE 0x25
#define CALIBRATIONVERSIONBYTE 0x23

enum
{
	ticks_3,
	ticks_4,
	beat_3,
	beat_4,
	scale_3,
	scale_4,
	algo_3,
	algo_4,
	algo_1,
	algo_2,
	scale_1,
	scale_2,
	beat_1,
	beat_2,
	ticks_1,
	ticks_2,
	__TUESDAY_LED_COUNT,
	tick_out = __TUESDAY_LED_COUNT,
	beat_out,
	loop_out,
	accent_out,
	gate_out,
	__TUESDAY_RGB_LED_COUNT,
	algo_button_led = __TUESDAY_RGB_LED_COUNT,
	scale_button_led,
	beats_button_led,
	ticks_button_led,
	__TUESDAY_LED_COUNT_WITH_GATELEDS
};

class Tuesday_Tick_t
{
public:
	unsigned char vel;
	signed char note;
	unsigned char accent : 1;
	unsigned char slide : 2;
	unsigned char maxsubticklength : 4;
} ;

class Tuesday_PatternContainer
{
public:
	Tuesday_Tick_t Ticks[TUESDAY_MAXTICK];
	unsigned char TPB = 4;
	unsigned char Length = 16;
} ;

class Tuesday_RandomGen
{
public:
	long RandomMemory;
};


//4096 = 2.048v
/*2.5 * (2.048 * INP)/4096
		(x * 4096)
		/ 2.5 * 2.048
		 = inp*/

#define DAC_VOLT_UNCALIBRATED(x) ((int32_t)((409600 * ((int32_t)x)) / (int32_t)(512)))

// Vout =dat/(2^16)  *  2.5 

// (Vout / (2.5 * 4.3)) * 65536
#define DAC_VOLT(x, chan) (32768 +   ((x)*  (65536.0f/(2.5*4.3))) )
#define DAC_NOTE(x, chan) (DAC_VOLT(x/12.0, chan))

#define LED_GRADIENT_LEN 1024

extern uint32_t *LedGradient;

class LED
{
public:
	uint8_t R, G, B;
	uint8_t tR, tG, tB;
	uint16_t tGra, Gra;
	int32_t fallspeed = 1;
	int mode = 0;
	int TGroffs;
	int aGroffs;
	LED()
	{
		Fade(255, 255, 0);
	};
	void Fire()
	{
		mode = 1;
		Gra = LED_GRADIENT_LEN;
		tGra = (int)(LED_GRADIENT_LEN / 2);
	
	}
	void Off()
	{
		mode = 1;
		tGra = 0;
	}

	void Set(int r, int g, int b)
	{
		mode = 0;
		R = tR = r;
		G = tG = g;
		B = tB = b;
	}

	void Fade(int r, int g, int b)
	{
		mode = 0;
		tR = r;
		tG = g;
		tB = b;
	}

	void SetGrad(int gra)
	{
		mode = 1;
		tGra = Gra = gra;
	}

	void FadeGrad(int gra)
	{
		mode = 1;
		tGra = gra;
	}

	void Update()
	{
		switch (mode)
		{
		case 0:

			if (tR > R)
			{
				R++;
			}
			else
			{
				if (tR < R) {
					R--;
					if (tR < R)R--;
				}
			}

			if (tG > G)
			{
				G++;
			}
			else
			{
				if (tG < G) {
					G--;
					if (tG < G)G--;
				}
			}

			if (tB > B)
			{
				B++;
			}
			else
			{
				if (tB < B) {
					B--;
					if (tB < B)B--;
				}
			}
			break;

		case 1:

			if (tGra > Gra)
			{
				Gra = __min(tGra, Gra + fallspeed);
			}
			else
			{
				if (tGra < Gra)
				{
					Gra = __max(tGra, Gra - fallspeed);
				}
			}
			if (TGroffs> aGroffs)
			{
				aGroffs = __min(TGroffs, aGroffs + fallspeed);
			}
			else
			{
				if (TGroffs < aGroffs)
				{
					aGroffs = __max(TGroffs, aGroffs - fallspeed);
				}
			}
			int idx = __max(0,__min(Gra + aGroffs, LED_GRADIENT_LEN-1) );
			G = (LedGradient[idx]>>24);
			R = (LedGradient[idx ]>>16)&0xff;
			B = (LedGradient[idx ]>>8)&0xff;
			break;
		}


	}
};

class Tuesday_PatternGen
{
public:
	uint8_t T;
	uint8_t ClockConnected;

	int32_t Gates[TUESDAY_GATES + 1];
	bool GatesBool[TUESDAY_GATES + 1];
	int32_t GatesGap[TUESDAY_GATES + 1];
	LED RStateLeds[__TUESDAY_LED_COUNT_WITH_GATELEDS];


//	uint16_t NoteOut;
//	uint16_t VelocityOut;

	int lastnote;
	int CoolDown;
	uint16_t TickOut;

	int32_t CVOut;
	int32_t CVOutTarget;
	int32_t NoteOutTarget;
	int32_t iNoteOut;
	int32_t CVOutDelta;
	int32_t CVOutCountDown;

	int Tick;
	int Measure;
	int NewTick;
	int DoReset;
	int countdownTick;
	//int countdownNote;
	int msecpertick;
	int msecsincelasttick;

	int clockup;
	int clockshad;
	int clockssincereset;

	int timesincelastclocktick;
	int clocktick;

	int directtick;
	int extclockssincereset[6];
	uint8_t extclockssinceresetcounter[6];
	int extclocksupsincereset;
	int extclocksdowsincereset;
	int lastclocksubdiv;


	// knobs
	uint8_t seed1;
	uint8_t seed2;
	uint8_t intensity;
	uint8_t tempo;

	uint8_t UIMode;
	uint8_t CalibTarget;
	uint8_t OptionSelect;
	uint8_t OptionIndex;
	struct Tuesday_PatternContainer CurrentPattern;

	uint8_t switchmode;
	uint8_t commitchange;
	uint8_t TicksPerMeasure = 1;
	int ticklengthscaler;
	uint8_t LastResetVal;
};

#define TUESDAY_MAXALGO 4
#define TUESDAY_MAXSCALE 4
#define TUESDAY_MAXBEAT 4
#define TUESDAY_MAXTPB 4

#define ALTERNATESCALES

typedef enum
{
#ifndef ALTERNATESCALES
	SCALE_MAJOR,
	SCALE_MINOR,
	SCALE_DORIAN,
	SCALE_BLUES,
	SCALE_PENTA,
	SCALE_12TONE,
	SCALE_MAJORTRIAD,
	SCALE_MINORTRIAD,

	SCALE_EGYPTIAN,
	SCALE_PHRYGIAN,
	SCALE_LOCRIAN,
	SCALE_OCTATONIC,
	SCALE_MELODICMINOR,
	SCALE_SCALE_MINORPENTA,
	SCALE_15,
	SCALE_16,
#else

	SCALE_MINOR,
	SCALE_MELODICMINOR,
	SCALE_MINORPENTA,
	SCALE_MINORTRIAD,
	SCALE_PHRYGIAN,
	SCALE_LOCRIAN,
	SCALE_OCTATONIC,
	SCALE_12TONE,
	SCALE_MAJOR,
	SCALE_DORIAN,
	SCALE_PENTA,
	SCALE_EGYPTIAN,
	SCALE_BLUES,
	SCALE_MAJORTRIAD,
	SCALE_15,
	SCALE_16,

#endif
	__SCALE_COUNT
} TUESDAY_SCALES;


typedef enum
{
	OCTAVELIMIT_OFF,
	OCTAVELIMIT_1,
	OCTAVELIMIT_2,
	OCTAVELIMIT_3,
} TUESDAY_OCTAVELIMIT;

typedef enum
{

	CLOCKMODE_RESETREGULAR = 0,
	CLOCKMODE_RESETINVERTED = 1,

	CLOCKMODE_RESET_BLOCKS_TICKS = 2,
	CLOCKMODE_RESET_TRIGGER_ONLY = 0,

	CLOCKMODE_DOWNSLOPE = 4,
	CLOCKMODE_UPSLOPE = 0

} TUESDAY_RESETFLAGS;


class Tuesday_Scale
{
public:
	uint8_t notes[12];
	uint8_t count;
};

 class Tuesday_Settings
{
public:
	uint32_t RandomSeed;

	uint8_t tpboptions[TUESDAY_MAXTPB];
	uint8_t beatoptions[TUESDAY_MAXBEAT];
	uint8_t scale[TUESDAY_MAXSCALE];
	uint8_t algooptions[TUESDAY_MAXALGO];

	Tuesday_Scale scales[__SCALE_COUNT];

	uint8_t ClockSubDivMode;
	uint8_t OctaveLimiter;
	uint8_t ClockMode;
} ;

class Tuesday_Params
{
public:
	// buttons
	uint8_t tpbopt = 0;
	uint8_t beatopt = 0;
	uint8_t scale = 0;
	uint8_t algo= 0;
};

class Tuesday_EEPROM
{
public:
	uint8_t Version;
	Tuesday_Params Params;
};


typedef enum {
	UI_STARTUP,
	UI_NORMAL,
	UI_CALIBRATION,
	UI_SELECTOPTION,
	UI_GLOBALSETTINGS,
	__TUESDAY_UIMODE_COUNT
} TUESDAY_UIMODE;

typedef enum {
	CALIBRATION_NOTARGET,
	CALIBRATION_VEL,
	CALIBRATION_NOTE,
	__TUESDAY_CALIBRATION_SETTING_COUNT
} TUESDAY_CALIBRATION_SETTING;

typedef enum {
	OPTION_ALGO,
	OPTION_SCALE,
	OPTION_BEATS,
	OPTION_TPB,
	__TUESDAY_OPTION_SETTING_COUNT
} TUESDAY_OPTION_SETTING;

typedef enum
{
	CLOCKSUBDIV_4,
	CLOCKSUBDIV_8,
	CLOCKSUBDIV_16,
	CLOCKSUBDIV_24PPQN
} TUESDAY_CLOCKSUBDIVISION_SETTING;

#define EEPROM_OPTIONBASE 4
#define EEPROM_SETTINGSBASE 256
#define EEPROM_CALIBRATIONBASE 512


extern int CalibrateAdjust(int input);
extern void Tuesday_Init(Tuesday_PatternGen* T);
extern void Tuesday_Clock(Tuesday_PatternGen* P, Tuesday_Settings* S, Tuesday_Params* Par, int ClockVal);
extern void Tuesday_ExtClock(Tuesday_PatternGen* P, Tuesday_Params* Params, Tuesday_Settings* S, int state);
extern void Tuesday_Reset(Tuesday_PatternGen* T, Tuesday_Settings* s, int Val);
extern void Tuesday_Tick(Tuesday_PatternGen* T, Tuesday_Params* P);
extern void Tuesday_TimerTick(Tuesday_PatternGen* T, Tuesday_Params* P);
extern void Tuesday_ValidateParams(Tuesday_Params* P);
extern void Tuesday_MainLoop(Tuesday_PatternGen* T, Tuesday_Settings* settings, Tuesday_Params* params);
extern void Tuesday_SwitchToOptionMode(Tuesday_PatternGen* T, int mode, int startoption);
extern void Tuesday_SetupLeds(Tuesday_PatternGen* T, Tuesday_Settings* settings, Tuesday_Params* params);
extern void Tuesday_ValidateSettings(Tuesday_Settings* S);
extern void Tuesday_LoadSettings(Tuesday_Settings* S, Tuesday_Params* P);
extern void Tuesday_LoadDefaults(Tuesday_Settings* S, Tuesday_Params* P);
extern void Tuesday_Generate(Tuesday_PatternGen* T, Tuesday_Params* P, Tuesday_Settings* S);
extern void Tuesday_RandomSeed(Tuesday_RandomGen* R, unsigned int seed);
extern void Tuesday_SetupClockSubdivision(Tuesday_PatternGen* P, Tuesday_Settings* S, Tuesday_Params* Par);

extern void Tuesday_Goa(Tuesday_PatternContainer* T, Tuesday_RandomGen* R, int Length);
extern void Tuesday_Flat(Tuesday_PatternContainer* T, Tuesday_RandomGen* R, int Length);
extern void Tuesday_Psych(Tuesday_PatternContainer* T, Tuesday_RandomGen* R, int Length);
extern void Tuesday_Zeph(Tuesday_PatternContainer* T, Tuesday_RandomGen* R, int stepsperbeat, int beats, int fullcycles);

extern void DoClock(int state);
extern void doTick();
extern void Tuesday_ToggleSlope(Tuesday_Settings* Settings);
extern void Tuesday_ToggleReset(Tuesday_Settings* Settings);
extern void Tuesday_ToggleResetPattern(Tuesday_Settings* Settings);

#define DEBOUNCETIME 10

class debounce_t
{
public: 
    int timeon = 0;
    int timeoff = DEBOUNCETIME;
    int pressed;
    int released;
    int longpressed;
} ;

	
int islongpress(debounce_t *state);
int pressed(debounce_t *state);

enum
{
    UNCERTAIN,
    PRESSED,
    RELEASED,
    DOWN,
    UP
};



