/**
 * @file control.c	device control for the LAR QuickTOCxy9 device - LAR market component
 *
 * (c) 2008 - 2010 LAR Process Analysers AG - All rights reserved.
 *
 * @author C.Vogt
 *
 * $Id: $ $URL: $
 **/

/**
+ added edinburgh, aide, ziroxZR5
+ removed simulate actions
+ FIX: needle sensor
+ using new position names
+ 6 streams
 **/
#include "tmp/version.h"
#include "tmp/control-item.h"
extern mktItem_t item0071[]; // needed for components since market 0.71 AFTER tmp/gui-item.h
extern mktSubscription_t *subscription0071; // needed for some components since market 0.71 AFTER tmp/gui-item.h
#include "market-statemachine.h"
#include "market-timer.h"
#include "market-debug.h"
#include "market-translation.h"
#include <time.h>

// include standard header files




#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#define mkIpropagate(it) (mkIisChanged(it)&&mkIisInitialized(it))
#define mkIsetChanged(it,v) do{if(mkIget(it)!=v) mkIset(it,v);}while(0)

#define WITH_HIGH_STIRRER_CURRENT	/* on y axis avoids furnace catching on x might avoid stopps (experimental) */
//#define WE_HAVE_A_PERFORMANCE_PROBLEM /* workaround uses 100 sec timeout for positioning errors (instead of 10 sec) */
//#define WITH_GARBAGE_COLLECTION_ON_INACTIVATING	/* to test the duration and best point in time */
// #define WITHOUT_CHECK	/* allows the user to disable warming up phase and broken sensors etc. */
// #define WITHOUT_NODE     /* for PC simulation to avoid node checking on your development system */
// #define WITHOUT_AUTOSTART	/* for security reasons we disable it in OEE */

#define MKT_NODE_CONTROL_DIGITAL_1
#define MKT_NODE_CONTROL_DIGITAL_2
#define MKT_NODE_CONTROL_DIGITAL_3

#define MKT_NODE_CONTROL_MOTOR_1
#define MKT_NODE_CONTROL_MOTOR_2
#define MKT_NODE_CONTROL_MOTOR_3
#define MKT_NODE_CONTROL_MOTOR_4

#ifdef WE_HAVE_A_PERFORMANCE_PROBLEM
#warning WE_HAVE_A_PERFORMANCE_PROBLEM and use 30 sec timeout for positioning errors (instead of 10 sec)
// to avoid performance problems, e.g. E2148 "POSxDRAIN after analysis" due to calpar saving,
// try:
//   make GLOBAL_CFLAGS="-D WE_HAVE_A_PERFORMANCE_PROBLEM" rebuild install
#endif
static int debugMode = 0;
#define debug_out(s) do{if ( 1&debugMode ) printf("%s",s); } while(0)

#ifdef WITHOUT_NODE
#warning compiling WITHOUT_NODE for debug purposes
// to avoid node checking on your development system, try:
//   make GLOBAL_CFLAGS="-D WITHOUT_NODE -D WITHOUT_CHECK" rebuild install
#endif

#ifdef WITHOUT_CHECK
#warning compiling WITHOUT_CHECK of needle, selftest, warming and friends using parameter___measure_while_airflowerror
// to avoid checking, try:
//   make GLOBAL_CFLAGS="-D WITHOUT_CHECK" rebuild install
#endif

#ifdef WITH_GARBAGE_COLLECTION_ON_INACTIVATING
#warning WITH_GARBAGE_COLLECTION_ON_INACTIVATING we can start garbage collection with the button
// try:
//   make GLOBAL_CFLAGS="-D WITH_GARBAGE_COLLECTION_ON_INACTIVATING" rebuild install
#endif

#ifdef WITHOUT_AUTOSTART
#warning WITHOUT_AUTOSTART  for security reasons we disable autostart it in OEE
// try:
//   make GLOBAL_CFLAGS="-D WITHOUT_AUTOSTART" rebuild install
#endif

// #define

static uint32_t digitalOutShadow = 0;		///< shadow - low word for DigitalOut1_16bit, high word for DigitalOut2_16bit register
static uint32_t digital3OutShadow = 0;		///< shadow - low word for DigitalOut3_16bit


//     &1 Digitalausgang 0	OUT 0	PL4/8	Y1 - Feuchteventil
//     &2 Digitalausgang 1	OUT 1	PL4/7	FURNACE - DC-motor open, Injektionsport
//     &4 Digitalausgang 2	OUT 2	PL4/6	FURNACE - DC-motor close, Injektionsport
//     &8 Digitalausgang 3	OUT 3	PL4/5	Y1Y1 - Injektionsventil (==Spritzenventil)
//    &16 Digitalausgang 4	OUT 4	PL4/4	Y2Y1 - TIC Ventil 01, Y2Y2 - TIC Ventil 02
//    &32 Digitalausgang 5	OUT 5	PL4/3	notyet Y4Y6; Homogenisator
//    &64 Digitalausgang 6	OUT 6	PL4/2	K1 Sch�tz (Ofencontroller)
//   &128 Digitalausgang 7	OUT 7	PL4/1	GP3 - Probenpumpe Strom2

//   &256 Digitalausgang 10	OUT 10	PL7/8	Relais 1
//   &512 Digitalausgang 11	OUT 11	PL7/7	Relais 2
//  &1024 Digitalausgang 12	OUT 12	PL7/6	Relais 3
//  &2048 Digitalausgang 13	OUT 13	PL7/5	Relais 4
//  &4096 Digitalausgang 14	OUT 14	PL7/4	Y5Y2 - Toc-Direkt-Ventil
//  &8192 Digitalausgang 15	OUT 15 	PL7/3	GP1 - Kondensatpumpe
// &16384 Digitalausgang 16	OUT 16	PL7/2	GP2 - Probenpumpe Strom1
// &32768 Digitalausgang 17	OUT 17	PL7/1	Lebenszeichen Software


#define DOPPELMOTOR_VERSION3_PRODUCT_KEY  262144


#define IS_DOPPELMOTOR_VERSION_3(n)     ( mkIget(node_Doppelmotor##n##__ProductCode)==DOPPELMOTOR_VERSION3_PRODUCT_KEY )


/**
 * set or reset a bit of the 16 bit digital out port
 **/
void digitalOut(int bit,int on)
{
	on &= 1;
	digitalOutShadow = digitalOutShadow & ~(1<<bit) | (on<<bit);
}

void digital3Out(int bit,int on)
{
	on &= 1;
	digital3OutShadow = digital3OutShadow & ~(1<<bit) | (on<<bit);
}


// Note: REMOTE_CONTROL bit(0) is done by measurement

// FURNACE - check open / closed / ok / dead
#define FURNACE_IS_OPEN_bit (1)		/* Y6S1 */
#define FURNACE_IS_OPEN ((1<<FURNACE_IS_OPEN_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?1:0)
//#define simulate_FURNACE_IS_OPEN(s) do{printf(" control simulate_FURNACE_IS_OPEN - %s\n",s);mkIset(node_Digital1__DigitalIn1_16bit,1<<FURNACE_IS_OPEN_bit);}while(0)

#define FURNACE_IS_CLOSED_bit (2)	/* Y6S2 */
#define FURNACE_IS_CLOSED ((1<<FURNACE_IS_CLOSED_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?1:0)
//#define simulate_FURNACE_IS_CLOSED(s) do{printf(" control simulate_FURNACE_IS_CLOSED - %s\n",s);mkIset(node_Digital1__DigitalIn1_16bit,1<<FURNACE_IS_CLOSED_bit);}while(0)

// DONE: off means "deviation" (on means "ok")
#define FURNACE_IS_OUT_OF_RANGE_bit (7)		/* N2 */
#define FURNACE_IS_OUT_OF_RANGE ((1<<FURNACE_IS_OUT_OF_RANGE_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?0:1)

// DONE: off means "emergency" (on means "ok")
#define FURNACE_IS_DEAD_bit (8)		/* N2 */
#define FURNACE_IS_DEAD ( (0==mkIisNull(node_Digital1__DigitalIn1_16bit))&&((1<<FURNACE_IS_DEAD_bit)&mkIget(node_Digital1__DigitalIn1_16bit))?0:1 )
//#define simulate_FURNACE_IS_DEAD(s) do{printf(" control simulate_FURNACE_IS_DEAD - %s\n",s);mkIset(node_Digital1__DigitalIn1_16bit,1<<FURNACE_IS_DEAD_bit);}while(0)



// Y1 - Feuchteventil
#define Y1_bit (0)
#define Y1_STD digitalOut(Y1_bit,0)
#define Y1 digitalOut(Y1_bit,1)

// LOOP Block On - injection Off - wait
#define LOOP_BLOCK_bit (1)
#define LOOP_BLOCK_WAIT do{digitalOut(LOOP_BLOCK_bit,0);}while(0)
#define LOOP_BLOCK_ONLINE do{digitalOut(LOOP_BLOCK_bit,1);}while(0)

// Spülventil/ Kalibration / Einzelmessung -NO , Spüling NC
//#define Y3Y2_bit (2)
//#define Y3Y2_NO do{digitalOut(Y3Y2_bit,0);digitalOut(Y3Y2_bit,0);}while(0)
//#define Y3Y2_NC do{digitalOut(Y3Y2_bit,0);digitalOut(Y3Y2_bit,1);}while(0)
// Gaskalibrierung Injection Ventil
#define Y4Y5_bit (3)
#define Y4Y5_NO do{digitalOut(Y4Y5_bit,0);digitalOut(Y4Y5_bit,0);}while(0)
#define Y4Y5_NC do{digitalOut(Y4Y5_bit,0);digitalOut(Y4Y5_bit,1);}while(0)
// Gaskalibrierung outlet Ventil
#define Y4Y6_bit (4)
#define Y4Y6_NO do{digitalOut(Y4Y6_bit,0);digitalOut(Y4Y6_bit,0);}while(0)
#define Y4Y6_NC do{digitalOut(Y4Y6_bit,0);digitalOut(Y4Y6_bit,1);}while(0)
// TIC ventil
#define Y4Y7_bit (23)
#define Y4Y7_NO do{digitalOut(Y4Y7_bit,0);digitalOut(Y4Y7_bit,0);}while(0)
#define Y4Y7_NC do{digitalOut(Y4Y7_bit,0);digitalOut(Y4Y7_bit,1);}while(0)

// Probestrom 1 Ventil
#define Y3Y1_bit (5)
#define Y3Y1_NO do{digitalOut(Y3Y1_bit,0);digitalOut(Y3Y1_bit,0);}while(0)
#define Y3Y1_NC do{digitalOut(Y3Y1_bit,0);digitalOut(Y3Y1_bit,1);}while(0)
// Probestrom 2 Ventil
#define Y3Y2_bit (17)
#define Y3Y2_NO do{digitalOut(Y3Y2_bit,0);digitalOut(Y3Y2_bit,0);}while(0)
#define Y3Y2_NC do{digitalOut(Y3Y2_bit,0);digitalOut(Y3Y2_bit,1);}while(0)
// Probestrom 3 Ventil
#define Y3Y3_bit (22)
#define Y3Y3_NO do{digitalOut(Y3Y3_bit,0);digitalOut(Y3Y3_bit,0);}while(0)
#define Y3Y3_NC do{digitalOut(Y3Y3_bit,0);digitalOut(Y3Y3_bit,1);}while(0)
// Probestrom 4 Ventil
#define Y3Y4_bit (0)
#define Y3Y4_NO do{digital3Out(Y3Y4_bit,0);digital3Out(Y3Y4_bit,0);}while(0)
#define Y3Y4_NC do{digital3Out(Y3Y4_bit,0);digital3Out(Y3Y4_bit,1);}while(0)
// Probestrom 5 Ventil
#define Y3Y5_bit (1)
#define Y3Y5_NO do{digital3Out(Y3Y5_bit,0);digital3Out(Y3Y5_bit,0);}while(0)
#define Y3Y5_NC do{digital3Out(Y3Y5_bit,0);digital3Out(Y3Y5_bit,1);}while(0)
// Probestrom 6 Ventil
#define Y3Y6_bit (2)
#define Y3Y6_NO do{digital3Out(Y3Y6_bit,0);digital3Out(Y3Y6_bit,0);}while(0)
#define Y3Y6_NC do{digital3Out(Y3Y6_bit,0);digital3Out(Y3Y6_bit,1);}while(0)


//Control ventil
#define Y3Y9_bit (2)
#define Y3Y9_NO do{digitalOut(Y3Y9_bit,0);digitalOut(Y3Y9_bit,0);}while(0)
#define Y3Y9_NC do{digitalOut(Y3Y9_bit,0);digitalOut(Y3Y9_bit,1);}while(0)

//Spülewasser
#define Y3Y10_bit (12)
#define Y3Y10_NO do{digitalOut(Y3Y10_bit,0);digitalOut(Y3Y10_bit,0);}while(0)
#define Y3Y10_NC do{digitalOut(Y3Y10_bit,0);digitalOut(Y3Y10_bit,1);}while(0)

//Einseuerung .....
// Einseuerung ST1
#define Y5Y1_bit (24)
#define Y5Y1_NO do{digitalOut(Y5Y1_bit,0);digitalOut(Y5Y1_bit,0);}while(0)
#define Y5Y1_NC do{digitalOut(Y5Y1_bit,0);digitalOut(Y5Y1_bit,1);}while(0)

// Einseuerung ST2
#define Y5Y2_bit (25)
#define Y5Y2_NO do{digitalOut(Y5Y2_bit,0);digitalOut(Y5Y2_bit,0);}while(0)
#define Y5Y2_NC do{digitalOut(Y5Y2_bit,0);digitalOut(Y5Y2_bit,1);}while(0)

// Einseuerung ST3
#define Y5Y3_bit (26)
#define Y5Y3_NO do{digitalOut(Y5Y3_bit,0);digitalOut(Y5Y3_bit,0);}while(0)
#define Y5Y3_NC do{digitalOut(Y5Y3_bit,0);digitalOut(Y5Y3_bit,1);}while(0)

// Einseuerung ST4
#define Y5Y4_bit (3)
#define Y5Y4_NO do{digital3Out(Y5Y4_bit,0);digital3Out(Y5Y4_bit,0);}while(0)
#define Y5Y4_NC do{digital3Out(Y5Y4_bit,0);digital3Out(Y5Y4_bit,1);}while(0)

// Einseuerung ST5
#define Y5Y5_bit (4)
#define Y5Y5_NO do{digital3Out(Y5Y5_bit,0);digital3Out(Y5Y5_bit,0);}while(0)
#define Y5Y5_NC do{digital3Out(Y5Y5_bit,0);digital3Out(Y5Y5_bit,1);}while(0)

// Einseuerung ST6
#define Y5Y6_bit (5)
#define Y5Y6_NO do{digital3Out(Y5Y6_bit,0);digital3Out(Y5Y6_bit,0);}while(0)
#define Y5Y6_NC do{digital3Out(Y5Y6_bit,0);digital3Out(Y5Y6_bit,1);}while(0)

// Einseuerung
#define Y5Y9_bit (27)
#define Y5Y9_NO do{digitalOut(Y5Y9_bit,0);digitalOut(Y5Y9_bit,0);}while(0)
#define Y5Y9_NC do{digitalOut(Y5Y9_bit,0);digitalOut(Y5Y9_bit,1);}while(0)

//TOC strip GAZ
#define Y6Y1_bit (16)
#define Y6Y1_NO do{digitalOut(Y6Y1_bit,0);digitalOut(Y6Y1_bit,0);}while(0)
#define Y6Y1_NC do{digitalOut(Y6Y1_bit,0);digitalOut(Y6Y1_bit,1);}while(0)



// GP1 - Condensate pump
#define GP1_bit (13)
#define GP1_STD digitalOut(GP1_bit,1)
#define GP1 GP1_STD
#define GP1_INACTIVE digitalOut(GP1_bit,0)
#define GP1_FULL GP1

// GP12 - Sample pump stream1
#define GP12_bit (14)
#define GP12_STD digitalOut(GP12_bit,0)
#define GP12 digitalOut(GP12_bit,1)
#define GP12_INACTIVE GP12_STD
#define GP12_FULL GP12

// GP13 - Sample pump stream2
#define GP13_bit (7)
#define GP13_STD digitalOut(GP13_bit,0)
#define GP13 digitalOut(GP13_bit,1)
#define GP13_INACTIVE GP13_STD
#define GP13_FULL GP13

// GP14 - Sample pump stream3
#define GP14_bit (18)
#define GP14_STD digitalOut(GP14_bit,0)
#define GP14 digitalOut(GP14_bit,1)
#define GP14_INACTIVE GP14_STD
#define GP14_FULL GP14

// GP15 - Sample pump stream4
#define GP15_bit (19)
#define GP15_STD digitalOut(GP15_bit,0)
#define GP15 digitalOut(GP15_bit,1)
#define GP15_INACTIVE GP15_STD
#define GP15_FULL GP15

// GP16 - Sample pump stream5
#define GP16_bit (20)
#define GP16_STD digitalOut(GP16_bit,0)
#define GP16 digitalOut(GP16_bit,1)
#define GP16_INACTIVE GP16_STD
#define GP16_FULL GP16

// GP17 - Sample pump stream6
#define GP17_bit (21)
#define GP17_STD digitalOut(GP17_bit,0)
#define GP17 digitalOut(GP17_bit,1)
#define GP17_INACTIVE GP17_STD
#define GP17_FULL GP17

// Furnace On / Off
#define FURNACE_ON_bit (6)
#define FURNACE_ON do{digitalOut(FURNACE_ON_bit,1);}while(0)
#define FURNACE_OFF do{digitalOut(FURNACE_ON_bit,0);}while(0)


#define COOLER_IS_OUT_OF_RANGE_bit (4)			/* EC */
//#define COOLER_IS_OUT_OF_RANGE ((1<<COOLER_IS_OUT_OF_RANGE_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?1:0)
// inverted clv2011-05-17:
#define COOLER_IS_OUT_OF_RANGE ((1<<COOLER_IS_OUT_OF_RANGE_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?0:1)

#define FLUID1_IS_bit (5)
//#define FLUID1_IS ((1<<FLUID1_IS_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?1:0)
// inverted asmolkov2013-07-08:
#define FLUID1_IS ((1<<FLUID1_IS_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?0:1)

#define FLUID2_IS_bit (6)
//#define FLUID2_IS ((1<<FLUID2_IS_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?1:0)
// inverted asmolkov2013-07-08:
#define FLUID2_IS ((1<<FLUID2_IS_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?0:1)

#define FLUID3_IS_bit (16)
//#define FLUID1_IS ((1<<FLUID1_IS_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?1:0)
// inverted asmolkov2013-07-08:
#define FLUID3_IS ((1<<FLUID3_IS_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?0:1)

#define FLUID4_IS_bit (17)
//#define FLUID2_IS ((1<<FLUID2_IS_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?1:0)
// inverted asmolkov2013-07-08:
#define FLUID4_IS ((1<<FLUID4_IS_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?0:1)

#define FLUID5_IS_bit (18)
//#define FLUID1_IS ((1<<FLUID1_IS_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?1:0)
// inverted asmolkov2013-07-08:
#define FLUID5_IS ((1<<FLUID5_IS_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?0:1)

#define FLUID6_IS_bit (19)
//#define FLUID2_IS ((1<<FLUID2_IS_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?1:0)
// inverted asmolkov2013-07-08:
#define FLUID6_IS ((1<<FLUID6_IS_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?0:1)


#define DOOR_OPEN_IS_bit (13)
#define DOOR_OPEN_IS ((1<<DOOR_OPEN_IS_bit)&mkIget(node_Digital1__DigitalIn1_16bit)?0:1)


#define SPEED_GP10_OFF (0) /* if 0 does not work stable, try 1 but be aware that the pump turns a bit! */

#define GP10_SPEED           ((uint32_t ) ( mkIget(parameter__Motor1_PumpSpeed)) )
#define GP10_SPEED_SLOW      ((uint32_t ) ( mkIget(parameter__Motor1_PumpSpeedSlow)) )
#define GP10_SPEED_FAST      ((uint32_t ) ( mkIget(parameter__Motor1_PumpSpeedFast)) ) //0 - 215

//mkIset(control_Motor1__ConstantKd,1); \
//mkIset(control_Motor1__ConstantKi,1); \
//mkIset(control_Motor1__ConstantKp,1); \

#define GP10_STD do{ \
mkIset(control_Motor1__Pump1Mode,0); \
mkIset(control_Motor1__Pump1Left,0); \
mkIset(control_Motor1__Pump1IntervalTime,0); \
mkIset(control_Motor1__Pump1IntervalPulse,0); \
mkIset(control_Motor1__Pump1, 0); \
mkIset(control_Motor1__Pump1Speed, SPEED_GP10_OFF ); \
}while(0)
#define GP10_INACTIVE do{ \
		GP10_STD; \
		mkIset(control_Motor1__Pump1,0); \
}while(0)
#define GP10_ON do{ \
		GP10_STD; \
mkIset(control_Motor1__Pump1,1); \
mkIset(control_Motor1__Pump1Speed,GP10_SPEED); \
}while(0)

#define GP10_SLOW do{ \
		GP10_STD; \
mkIset(control_Motor1__Pump1,1); \
mkIset(control_Motor1__Pump1Speed,GP10_SPEED_SLOW); \
}while(0)

#define GP10_FAST do{ \
		GP10_STD; \
mkIset(control_Motor1__Pump1,1); \
mkIset(control_Motor1__Pump1Speed,GP10_SPEED_FAST); \
}while(0)



//
// state machine actions
//
// Control timers
#define TIMER (MKiP_control_subscription__timer)
#define startTimer(ms) (mktStartTimer(TIMER,ms))
#define timeout() (mktTimeout(TIMER))
#define stopTimer() (mktStopTimer(TIMER))

#define TIMER_DELAY (MKiP_control_subscription__timerDelay)
#define startTimer_Delay(ms) (mktStartTimer(TIMER_DELAY,ms))
#define timeout_Delay() (mktTimeout(TIMER_DELAY))
#define stopTimer_Delay() (mktStopTimer(TIMER_DELAY))

#define TIMER_JS          MKiP_control_subscription__timerJS
#define startTimerJS(ms)  mktStartTimer (TIMER_JS, ms)
#define timeoutJS         mktTimeout    (TIMER_JS)
#define stopTimerJS       mktStopTimer  (TIMER_JS)

#define TIMER_Analysing          MKiP_control_subscription__timerAnalysing
#define startTimerAnalysing(ms)  mktStartTimer (TIMER_Analysing, ms)
#define timeoutAnalysing()       mktTimeout    (TIMER_Analysing)
#define stopTimerAnalysing()     mktStopTimer  (TIMER_Analysing)

#define TIMER_ERROR (MKiP_control_subscription__timerError)
#define startTimerError(ms) (mktStartTimer(TIMER_ERROR,ms))
#define timeoutError() (mktTimeout(TIMER_ERROR))
#define stopTimerError() (mktStopTimer(TIMER_ERROR))

#define TIMERDF_MEM (MKiP_control_subscription__timerDfMem)
#define startTimerDfMem() (mktStartTimerDf(TIMERDF_MEM))
#define timeoutDfMem()  (mktTimeoutDf(TIMERDF_MEM,1))
#define stopTimerDfMem() (mktStopTimerDf(TIMERDF_MEM))
#define goDfMem(st) ;if(entry){startTimerDfMem();} else if(exit){stopTimerDfMem();} else if(timeoutDfMem()){go(st,state);}

#define TIMERAT_AUTO_CALIBRATION (MKiP_control_subscription__timerAt)
#define startTimerAt() (mktStartTimerAt(TIMERAT_AUTO_CALIBRATION))
#define timeoutAt()    (mktTimeoutAt(TIMERAT_AUTO_CALIBRATION))
#define stopTimerAt()  (mktStopTimerAt(TIMERAT_AUTO_CALIBRATION))
#define doneTimerAt()  (mktDoneTimerAt(TIMERAT_AUTO_CALIBRATION))


// @TODO move to market-statemachine.h:
// @TODO: trial error transmission type3 to fix error gone problem - find cameError goneError
#define debug_out_state(s,str) do{debug_out(" ");debug_out(state_machine_description);debug_out(str);debug_out("state:");debug_out(s);debug_out("\n");}while(0)
#define goMs_error(st,ms) ;if(entry){startTimerError(ms);} else if(exit){stopTimerError();} else if(timeoutError()){cameError();debug_out(" timeout error -");go(st,state);}
#define goIf_error(st,cond) else if ( cond ) {cameError();go(st,state);}

#define ms_startTimer(ms) ;if(entry){startTimer(ms);} else if(exit){;} else if(timeout()){;}
#define go_timeout(st) ;if(entry){;} else if(exit){;} else if(timeout()){stopTimer();go(st,state);}

#define ms_startTimer_Delay(ms) ;if(entry){startTimer_Delay(ms);} else if(exit){;} else if(timeout_Delay()){;}
#define go_timeout_Delay(st) ;if(entry){;} else if(exit){;} else if(timeout_Delay()){stopTimer_Delay();go(st,state);}

#define JS_startTimer(ms) ;if(entry){startTimerJS(ms);} else if(exit){;} else if(timeoutJS()){;}
#define go_timeoutJS(st) ;if(entry){;} else if(exit){;} else if(timeoutJS()){stopTimerJS();go(st,state);}

#define Analysing_startTimer(ms) ;if(entry){startTimerAnalysing(ms);} else if(exit){;} else if(timeoutAnalysing()){;}
#define go_timeoutAnalysing(st) ;if(entry){;} else if(exit){;} else if(timeoutAnalysing()){stopTimerAnalysing();go(st,state);}


// FIX let state machine execute lines after ifDo :
#define ifDo(cond,prog) else if ( cond ) {prog;};if(entry){;} else if(exit){;}

//
// state machine variables
//
#define RETRY 			         (mkIget(control_debug__RETRY))
#define RETRY_INJECTION_SYSTEM   (mkIget(control_debug__RETRY_INJECTION_SYSTEM))
#define RETRY_LOOP_INJECTION     (mkIget(control_debug__RETRY_LOOP_INJECTION))

#define SET_RETRY(v) 	do{mkIset(control_debug__RETRY,(v));}while(0)
#define SET_RETRY_INJECTION_SYSTEM(v) 	do{mkIset(control_debug__RETRY_INJECTION_SYSTEM,(v));}while(0)
#define SET_RETRY_LOOP_INJECTION(v) 	do{mkIset(control_debug__RETRY_LOOP_INJECTION,(v));}while(0)

#define IS_HOLD_POS              (mkIget(control_debug__hold_pos_done))
#define SET_HOLD_POS_DONE(v)     do{mkIset(control_debug__hold_pos_done,(v));}while(0)
#define AUTOCALIBRATION          (mkIget(control_debug__AUTOCALIBRATION))
#define SET_AUTOCALIBRATION(v)   do{mkIset(control_debug__AUTOCALIBRATION,(v));}while(0)

#define COMPARE                  (mkIget(control_mode__COMPARE))
#define SET_COMPARE(v)           do{mkIset(control_mode__COMPARE,(v));}while(0)

#define GO_TO_HOLD               (mkIget(control_debug__GO_TO_HOLD))
#define SET_GO_TO_HOLD(v)	do{mkIset(control_debug__GO_TO_HOLD,(v));}while(0)
#define CALIBRATION		(mkIget(control_mode__CALIBRATION))
#define CALIBRATION_SIGNAL (mkIget(control_mode__CALIBRATION_SIGNAL))
#define SET_CALIBRATION_SIGNAL(signal) do{mkIset(control_mode__CALIBRATION_SIGNAL,(signal));}while(0)
#define TIC 			(mkIget(control_mode__TIC))
#define SET_TIC(v) 		do{mkIset(control_mode__TIC,(v));}while(0)
#define STREAM 			(mkIget(control_mode__STREAM))
#define STREAM_OLD		(mkIget(control_debug__OLD_STREAM))
#define AUTOCAL_STREAM  (mkIget(control_debug__AUTOCAL_STREAM))
#define SET_AUTOCAL_STREAM(v)	do{mkIset(control_debug__AUTOCAL_STREAM,(v));}while(0)
#define SOLUTION    	(mkIget(control_mode__SOLUTION))
#define SET_SOLUTION(v)	do{mkIset(control_mode__SOLUTION,(v));}while(0)
#define IS_NEW_SOLUTION (mkIget(control_debug__CHANGE_SOLUTION))
#define SET_NEW_SOLUTION(v)	do{mkIset(control_debug__CHANGE_SOLUTION,(v));}while(0)
#define SET_CONTROL_SOLUTION_USER(v) do{mkIset(control___ChangeSolution,(v));}while(0)
#define IS_STREAM_ERROR ( (1>mkIget(control_mode__STREAM))||((mkIget(control_mode__STREAM))>mkIget(parameter___streamCount)))
#define SET_MULTIPLE_MEASUREMENT(v) do{mkIset(control_mode__MULTIPLE_MEASUREMENT,(v));}while(0)
#define MULTIPLE_MEASUREMENT (mkIget(control_mode__MULTIPLE_MEASUREMENT))
#define IS_SINGLE_STREAM_ERROR ( (1>mkIget(parameter___single_stream))||((mkIget(parameter___single_stream))>mkIget(parameter___streamCount)))
#define SINGLE_MEASUREMENT (mkIget(control_mode__SINGLE_MEASUREMENT))
#define SINGLE_MEASUREMENT_SAMPLE (mkIget(parameter___opt_sampleVesselSingle))
#define SET_SINGLE_MEASUREMENT(v) do{mkIset(control_mode__SINGLE_MEASUREMENT,(v));}while(0)
#define IS_GAS_CALIBRATION          (mkIget(parameter___calibration_type)==1)
#define VALIDIERUNG                 (mkIget(control_mode__VALIDIERUNG)==1)
#define SET_VALIDIERUNG(v) do{mkIset(control_mode__VALIDIERUNG,(v));}while(0)
#define ONLY_GASFLOW          (mkIget(control_debug__ONLY_GASFLOW))
#define SET_ONLY_GASFLOW(v)do{mkIset(control_debug__ONLY_GASFLOW,(v));}while(0)

#define MESSBEREICH_UMSCHALTEN 		 (mkIget(control_debug__MESSBEREICH_UMSCHALTEN))
#define SET_MESSBEREICH_UMSCHALTEN(v)do{mkIset(control_debug__MESSBEREICH_UMSCHALTEN,(v));}while(0)
#define MESSBEREICH_STREAM           (mkIget(control_debug__MESSBEREICH_STREAM))
#define SET_MESSBEREICH_STREAM(v)    do{ mkIset(control_debug__MESSBEREICH_STREAM,(v)); }while(0)

#define IS_RINSING 			(mkIget(control_debug__RINSING))
#define SET_RINSING(v)do{mkIset(control_debug__RINSING,(v));printf("TESTC_SET_RINSING %d ",mkIget(control_debug__RINSING));}while(0)

#define PRINT_RINSING do{printf("TESTC_RINSING %d ",mkIget(control_debug__RINSING));}while(0)

#define GAS_CLEANUP_REPL 		 (mkIget(control_debug__GAS_CLEANUP_REPLICATES))
#define SET_GAS_CLEANUP_REPL(v)do{mkIset(control_debug__GAS_CLEANUP_REPLICATES,(v));}while(0)

#define GAS_CLEANUP_STATUS 		 (mkIget(control_debug__GAS_CLEANUP_STATUS))
#define SET_GAS_CLEANUP_STATUS(v)do{mkIset(control_debug__GAS_CLEANUP_STATUS,(v));}while(0)

#define SET_STREAM(v) 	do{mkIset(control_mode__STREAM,(v)); }while(0)
#define SET_OLD_STREAM(v) do{mkIset(control_debug__OLD_STREAM,(v)); }while(0)

#define HS_RINSING            (mkIget(control_debug__HSRinsing))
#define SET_HS_RINSING(v)do{mkIset(control_debug__HSRinsing,(v));}while(0)

mktBoolean
pre_rinsing ( int stream )
{
	switch ( stream )
	{
	case 6:   return mkIget(parameter___pre_rinsing6);
	case 5:   return mkIget(parameter___pre_rinsing5);
	case 4:   return mkIget(parameter___pre_rinsing4);
	case 3:   return mkIget(parameter___pre_rinsing3);
	case 2:   return mkIget(parameter___pre_rinsing2);
	case 1:   return mkIget(parameter___pre_rinsing1);
	default:  return mkIget(parameter___pre_rinsing1);
	}
}

#define IS_PRE_RINSING  (pre_rinsing(STREAM))


uint32_t
pre_rinsing_time ( int stream )
{
	switch ( stream )
	{
	case 6:   return mkIget(parameter___pre_rinsing_time6);
	case 5:   return mkIget(parameter___pre_rinsing_time5);
	case 4:   return mkIget(parameter___pre_rinsing_time4);
	case 3:   return mkIget(parameter___pre_rinsing_time3);
	case 2:   return mkIget(parameter___pre_rinsing_time2);
	case 1:   return mkIget(parameter___pre_rinsing_time1);
	default:  return mkIget(parameter___pre_rinsing_time1);
	}
}


#define PRE_RINSING_TIME (pre_rinsing_time(STREAM))

mktBoolean
is_rinsing ( int stream )
{
	switch ( stream )
	{
	case 6:   return mkIget(parameter___rinsing6);
	case 5:   return mkIget(parameter___rinsing5);
	case 4:   return mkIget(parameter___rinsing4);
	case 3:   return mkIget(parameter___rinsing3);
	case 2:   return mkIget(parameter___rinsing2);
	case 1:   return mkIget(parameter___rinsing1);
	default:  return mkIget(parameter___rinsing1);
	}
}

#define IS_RINSING_ON (is_rinsing(STREAM))

uint32_t
rinsing_time ( int stream )
{
	switch ( stream )
	{
	case 6:   return mkIget(parameter___rinsing_time6);
	case 5:   return mkIget(parameter___rinsing_time5);
	case 4:   return mkIget(parameter___rinsing_time4);
	case 3:   return mkIget(parameter___rinsing_time3);
	case 2:   return mkIget(parameter___rinsing_time2);
	case 1:   return mkIget(parameter___rinsing_time1);
	default:  return mkIget(parameter___rinsing_time1);
	}
}


#define RINSING_TIME (rinsing_time(STREAM))

#define TUBING_CAL_WAY (mkIget(parameter___opt_sampleVesselSingle) == 0)


void TUBING_OFF ( int stream )
{
	Y3Y1_NO;Y3Y2_NO;Y3Y3_NO;Y3Y4_NO;Y3Y5_NO;Y3Y6_NO;
	if(CALIBRATION) Y3Y9_NO;
	else if( mkIget(parameter___opt_sampleVesselSingle) == 0 && SINGLE_MEASUREMENT)Y3Y9_NO;
}


/*void RINSING_ON( )
{
	Y3Y1_NO;Y3Y2_NO;Y3Y3_NO;Y3Y4_NO;Y3Y5_NO;Y3Y6_NO;Y3Y7_NO;
	Y3Y1_NC;Y3Y2_NO;
}*/

void TUBING_ON ( int stream )
{
	Y3Y1_NO;Y3Y2_NO;Y3Y3_NO;Y3Y4_NO;Y3Y5_NO;Y3Y6_NO;
	if( mkIget(parameter___measuring_range_switching) != 0 && !CALIBRATION && !SINGLE_MEASUREMENT )
	{
		if(stream == 1 || stream ==2)
		{
			Y3Y1_NC;
			return ;
		}
	}
	if(CALIBRATION  )
	{
		Y3Y9_NC;
		return;
	}
	if(mkIget(parameter___opt_sampleVesselSingle) == 0 && SINGLE_MEASUREMENT)
	{
		Y3Y9_NC;
		return;
	}
	switch ( stream )
	{
	case 6:  Y3Y6_NC; break;
	case 5:  Y3Y5_NC; break;
	case 4:  Y3Y4_NC; break;
	case 3:  Y3Y3_NC; break;
	case 2:  Y3Y2_NC; break;
	case 1:  Y3Y1_NC; break;
	default: Y3Y1_NC; break;
	}
}

void ACID_OFF ( int stream )
{
	Y5Y1_NO;Y5Y2_NO;Y5Y3_NO;Y5Y4_NO;Y5Y5_NO;Y5Y6_NO;

}

void ACID_ON ( int stream )
{
	Y5Y1_NO;Y5Y2_NO;Y5Y3_NO;Y5Y4_NO;Y5Y5_NO;Y5Y6_NO;
	if( mkIget(parameter___measuring_range_switching) != 0 && !CALIBRATION && !SINGLE_MEASUREMENT )
	{
		if(stream == 1 || stream ==2)
		{
			Y5Y1_NC;
			return ;
		}
	}
	if(CALIBRATION  )
	{
		//Y3Y1_NC;Y3Y2_NO;
		return;
	}
	/*if(mkIget(parameter___opt_sampleVesselSingle) == 0 && SINGLE_MEASUREMENT)
	{
		Y3Y1_NC;Y3Y2_NO;
		return;
	}*/
	switch ( stream )
	{
	case 6:  Y5Y6_NC; break;
	case 5:  Y5Y5_NC; break;
	case 4:  Y5Y4_NC; break;
	case 3:  Y5Y3_NC; break;
	case 2:  Y5Y2_NC; break;
	case 1:  Y5Y1_NC; break;
	default: Y5Y1_NC; break;
	}
}


#define SINGLE_MEASUREMENT_WITH_CAL ((mkIget(parameter___opt_sampleVesselSingle) == 0 && SINGLE_MEASUREMENT))


void
LOOP_PREPARE_SOLUTION()
{
	GP10_ON;
}


void GP_INACTIVE (int stream)
{
	//FIX : Beim messbereichumschaltung wird immer erste Pumpe ausgeschaltet.
	if( mkIget(parameter___measuring_range_switching) != 0 && !CALIBRATION && !SINGLE_MEASUREMENT )
	{
		if(stream == 1 || stream ==2)
		{
			GP12_INACTIVE;
			return ;
		}
	}
	switch ( stream )
	{
	case 6:  GP17_INACTIVE; break;
	case 5:  GP16_INACTIVE; break;
	case 4:  GP15_INACTIVE; break;
	case 3:  GP14_INACTIVE; break;
	case 2:  GP13_INACTIVE; break;
	case 1:  GP12_INACTIVE; break;
	default: GP12_INACTIVE; break;	// unknown streams always use stream 1
	}

}

//
// state machine extensions for the XY device
//
void GP_FULL (int stream)
{
	if( mkIget(parameter___measuring_range_switching) != 0 && !CALIBRATION && !SINGLE_MEASUREMENT )
	{
		if(stream == 1 || stream ==2)
		{
			GP12_FULL;
			return ;
		}
	}
	switch ( stream )
	{
		case 6:  GP17_FULL; break;
		case 5:  GP16_FULL; break;
		case 4:  GP15_FULL; break;
		case 3:  GP14_FULL; break;
		case 2:  GP13_FULL; break;
		case 1:  GP12_FULL; break;
		default: GP12_FULL; break;	// unknown streams always use stream 1
	}
}


int32_t METHOD (int stream)
{
	switch ( stream )
	{	// missing streams position of last existing stream
		case 6:  return (mkIget(parameter___opt_stream6_method));
		case 5:  return (mkIget(parameter___opt_stream5_method));
		case 4:  return (mkIget(parameter___opt_stream4_method));
		case 3:  return (mkIget(parameter___opt_stream3_method));
		case 2:  return (mkIget(parameter___opt_stream2_method));
		case 1:  return (mkIget(parameter___opt_stream1_method));
		default: return (0);	// unknown streams always has method 'none'
	}
}

int32_t IS_CODo_METHOD (int stream)
{
	switch ( stream )
	{	// missing streams position of last existing stream
		case 6:  return (mkIget(parameter___opt_stream6_codo));
		case 5:  return (mkIget(parameter___opt_stream5_codo));
		case 4:  return (mkIget(parameter___opt_stream4_codo));
		case 3:  return (mkIget(parameter___opt_stream3_codo));
		case 2:  return (mkIget(parameter___opt_stream2_codo));
		case 1:  return (mkIget(parameter___opt_stream1_codo));
		default: return (0);	// unknown streams always has method 'none'
	}
}

#define NEEDS_TIC		            (1==METHOD(STREAM)||4==METHOD(STREAM))
#define TIC_ONLY(STREAM)            (4==METHOD(STREAM))
#define TOC_DIRECT_METHOD           (2==METHOD(STREAM))
#define TOC_DIFF_METHOD             (1==METHOD(STREAM))
#define TC_METHOD                   (3==METHOD(STREAM))
#define CODo_METHOD                 (IS_CODo_METHOD(STREAM))

//#define INTERNE_TOC_DIRECT_VENTIL   do{ if( (1==STREAM)&&(TOC_DIRECT_METHOD))Y5Y2; }while(0)

int32_t solution (void)		// which calibration solution is running
{
// @TODO: support more than one solution enabled in control and calibration
	if ( mkIget(calpar___Solution1_Start) )  return (1);
	if ( mkIget(calpar___Solution2_Start) )  return (2);
	if ( mkIget(calpar___Solution3_Start) )  return (3);
	if ( mkIget(calpar___Solution4_Start) )  return (4);
	if ( mkIget(calpar___Solution5_Start) )  return (5);
	if ( mkIget(calpar___Solution6_Start) )  return (6);
	if ( mkIget(calpar___Solution7_Start) )  return (7);
	if ( mkIget(calpar___Solution8_Start) )  return (8);
	if ( mkIget(calpar___Solution9_Start) )  return (9);
	if ( mkIget(calpar___Solution10_Start) ) return (10);
	return (0);	// no solution selected at all
}

bool32_t STREAM_FOR_SOLUTION (int stream, int solution)	// shall we calibrate this stream with this solution?
{
	switch ( solution )
	{
		case 1:  switch ( stream )
		{
			case 1:  return mkIget(calpar___Solution1Stream1_Start);
			case 2:  return mkIget(calpar___Solution1Stream2_Start);
			case 3:  return mkIget(calpar___Solution1Stream3_Start);
			case 4:  return mkIget(calpar___Solution1Stream4_Start);
			case 5:  return mkIget(calpar___Solution1Stream5_Start);
			case 6:  return mkIget(calpar___Solution1Stream6_Start);
			default: return(0);
		}
		case 2:  switch ( stream )
		{
			case 1:  return mkIget(calpar___Solution2Stream1_Start);
			case 2:  return mkIget(calpar___Solution2Stream2_Start);
			case 3:  return mkIget(calpar___Solution2Stream3_Start);
			case 4:  return mkIget(calpar___Solution2Stream4_Start);
			case 5:  return mkIget(calpar___Solution2Stream5_Start);
			case 6:  return mkIget(calpar___Solution2Stream6_Start);
			default: return(0);
		}
		case 3:  switch ( stream )
		{
			case 1:  return mkIget(calpar___Solution3Stream1_Start);
			case 2:  return mkIget(calpar___Solution3Stream2_Start);
			case 3:  return mkIget(calpar___Solution3Stream3_Start);
			case 4:  return mkIget(calpar___Solution3Stream4_Start);
			case 5:  return mkIget(calpar___Solution3Stream5_Start);
			case 6:  return mkIget(calpar___Solution3Stream6_Start);
			default: return(0);
		}
		case 4:  switch ( stream )
		{
			case 1:  return mkIget(calpar___Solution4Stream1_Start);
			case 2:  return mkIget(calpar___Solution4Stream2_Start);
			case 3:  return mkIget(calpar___Solution4Stream3_Start);
			case 4:  return mkIget(calpar___Solution4Stream4_Start);
			case 5:  return mkIget(calpar___Solution4Stream5_Start);
			case 6:  return mkIget(calpar___Solution4Stream6_Start);
			default: return(0);
		}
		case 5:  switch ( stream )
		{
			case 1:  return mkIget(calpar___Solution5Stream1_Start);
			case 2:  return mkIget(calpar___Solution5Stream2_Start);
			case 3:  return mkIget(calpar___Solution5Stream3_Start);
			case 4:  return mkIget(calpar___Solution5Stream4_Start);
			case 5:  return mkIget(calpar___Solution5Stream5_Start);
			case 6:  return mkIget(calpar___Solution5Stream6_Start);
			default: return(0);
		}
		case 6:  switch ( stream )
		{
			case 1:  return mkIget(calpar___Solution6Stream1_Start);
			case 2:  return mkIget(calpar___Solution6Stream2_Start);
			case 3:  return mkIget(calpar___Solution6Stream3_Start);
			case 4:  return mkIget(calpar___Solution6Stream4_Start);
			case 5:  return mkIget(calpar___Solution6Stream5_Start);
			case 6:  return mkIget(calpar___Solution6Stream6_Start);
			default: return(0);
		}
		case 7:  switch ( stream )
		{
			case 1:  return mkIget(calpar___Solution7Stream1_Start);
			case 2:  return mkIget(calpar___Solution7Stream2_Start);
			case 3:  return mkIget(calpar___Solution7Stream3_Start);
			case 4:  return mkIget(calpar___Solution7Stream4_Start);
			case 5:  return mkIget(calpar___Solution7Stream5_Start);
			case 6:  return mkIget(calpar___Solution7Stream6_Start);
			default: return(0);
		}
		case 8:  switch ( stream )
		{
			case 1:  return mkIget(calpar___Solution8Stream1_Start);
			case 2:  return mkIget(calpar___Solution8Stream2_Start);
			case 3:  return mkIget(calpar___Solution8Stream3_Start);
			case 4:  return mkIget(calpar___Solution8Stream4_Start);
			case 5:  return mkIget(calpar___Solution8Stream5_Start);
			case 6:  return mkIget(calpar___Solution8Stream6_Start);
			default: return(0);
		}
		case 9:  switch ( stream )
		{
			case 1:  return mkIget(calpar___Solution9Stream1_Start);
			case 2:  return mkIget(calpar___Solution9Stream2_Start);
			case 3:  return mkIget(calpar___Solution9Stream3_Start);
			case 4:  return mkIget(calpar___Solution9Stream4_Start);
			case 5:  return mkIget(calpar___Solution9Stream5_Start);
			case 6:  return mkIget(calpar___Solution9Stream6_Start);
			default: return(0);
		}
		case 10:  switch ( stream )
		{
			case 1:  return mkIget(calpar___Solution10Stream1_Start);
			case 2:  return mkIget(calpar___Solution10Stream2_Start);
			case 3:  return mkIget(calpar___Solution10Stream3_Start);
			case 4:  return mkIget(calpar___Solution10Stream4_Start);
			case 5:  return mkIget(calpar___Solution10Stream5_Start);
			case 6:  return mkIget(calpar___Solution10Stream6_Start);
			default: return(0);
		}
		default: return(0);
	}
}


int32_t NEXT_SOLUTION (int solution)
{
	for(solution++;solution < 11;solution++)
	{
		switch(solution)
		{
			case 1 :if ( mkIget(calpar___Solution1_Start) ) {SET_SOLUTION(1);return (1);}break;
			case 2 :if ( mkIget(calpar___Solution2_Start) ) {SET_SOLUTION(2);return (2);}break;
			case 3 :if ( mkIget(calpar___Solution3_Start) ) {SET_SOLUTION(3);return (3);}break;
			case 4 :if ( mkIget(calpar___Solution4_Start) ) {SET_SOLUTION(4);return (4);}break;
			case 5 :if ( mkIget(calpar___Solution5_Start) ) {SET_SOLUTION(5);return (5);}break;
			case 6 :if ( mkIget(calpar___Solution6_Start) ) {SET_SOLUTION(6);return (6);}break;
			case 7 :if ( mkIget(calpar___Solution7_Start) ) {SET_SOLUTION(7);return (7);}break;
			case 8 :if ( mkIget(calpar___Solution8_Start) ) {SET_SOLUTION(8);return (8);}break;
			case 9 :if ( mkIget(calpar___Solution9_Start) ) {SET_SOLUTION(9);return (9);}break;
			case 10:if ( mkIget(calpar___Solution10_Start)) {SET_SOLUTION(10);return (10);}break;
			default:break;
		}
	}
	SET_SOLUTION(0);
	return 0;
}

int32_t STREAM_OCCURRED_LIMIT_MAX(int stream)
{
	switch(stream)
	{
	case 1:mkt_debug("history___limit_stream1_max == %d",mkIget(history___limit_stream1_max));return mkIget(history___limit_stream1_max);
	case 2:mkt_debug("history___limit_stream2_max == %d",mkIget(history___limit_stream2_max));return mkIget(history___limit_stream2_max);
	case 3:mkt_debug("history___limit_stream3_max == %d",mkIget(history___limit_stream3_max));return mkIget(history___limit_stream3_max);
	case 4:mkt_debug("history___limit_stream4_max == %d",mkIget(history___limit_stream4_max));return mkIget(history___limit_stream4_max);
	case 5:mkt_debug("history___limit_stream5_max == %d",mkIget(history___limit_stream5_max));return mkIget(history___limit_stream5_max);
	case 6:mkt_debug("history___limit_stream6_max == %d",mkIget(history___limit_stream6_max));return mkIget(history___limit_stream6_max);
	}
	return 1;
}
int32_t STREAM_OCCURRED_LIMIT_MIN(int stream)
{
	switch(stream)
	{
	case 1:mkt_debug("history___limit_stream1_min == %d",mkIget(history___limit_stream1_min));return mkIget(history___limit_stream1_min);
	case 2:mkt_debug("history___limit_stream2_min == %d",mkIget(history___limit_stream2_min));return mkIget(history___limit_stream2_min);
	case 3:mkt_debug("history___limit_stream3_min == %d",mkIget(history___limit_stream3_min));return mkIget(history___limit_stream3_min);
	case 4:mkt_debug("history___limit_stream4_min == %d",mkIget(history___limit_stream4_min));return mkIget(history___limit_stream4_min);
	case 5:mkt_debug("history___limit_stream5_min == %d",mkIget(history___limit_stream5_min));return mkIget(history___limit_stream5_min);
	case 6:mkt_debug("history___limit_stream6_min == %d",mkIget(history___limit_stream6_min));return mkIget(history___limit_stream6_min);
	}
	return 1;
}


int
get_measuring_range_switching()
{
	//TEST:
	mkt_debug("range_switching test ");
	if(!STREAM_OCCURRED_LIMIT_MAX(1))
	{
			//TEST:	mkt_debug(" return 1");
			if(MESSBEREICH_STREAM != 1){ SET_MESSBEREICH_UMSCHALTEN(1); }
			SET_MESSBEREICH_STREAM(1);
			return 1;
	}
	else if(!STREAM_OCCURRED_LIMIT_MIN(2))
	{
			//TEST:	mkt_debug(" return 2");
		if(MESSBEREICH_STREAM != 2) {  SET_MESSBEREICH_UMSCHALTEN(1); }
		SET_MESSBEREICH_STREAM(2);
		return 2;
	}
	else
	{
		mkt_debug(" MESSBEREICH_STREAM %d",MESSBEREICH_STREAM);
		switch(MESSBEREICH_STREAM)
		{
		case 1 :
			SET_MESSBEREICH_STREAM(2);
			SET_MESSBEREICH_UMSCHALTEN(1);
			return 2;
		case 2:
			SET_MESSBEREICH_STREAM(1);
			SET_MESSBEREICH_UMSCHALTEN(1);
			return 1;
		default :
			SET_MESSBEREICH_STREAM(1);
			SET_MESSBEREICH_UMSCHALTEN(1);
			return 1;
		}
		return 0;
	}
	return 0;
}

int32_t NEXT_STREAM (int stream)
{
	if (CALIBRATION)
	{   int ret;
		for (ret=1;ret<7; ret++) if(STREAM_FOR_SOLUTION(ret,SOLUTION)) return(ret);
		return(0);	// all streams calibrated, so return 0
	}
	else if(VALIDIERUNG)
	{
		return 1;
	}
	else
	{
		if( mkIget(parameter___measuring_range_switching) != 0 && !CALIBRATION && !SINGLE_MEASUREMENT )
		{
			SET_MESSBEREICH_UMSCHALTEN(0);
			//TEST:
			int st = 1 + stream%6;
			mkt_debug("NEXT_STREAM measuring_range %d stream" , st);

			if(st < 3)
			{
				int mstr =0;
				if( (mstr = get_measuring_range_switching())  )
				{
					if(MESSBEREICH_UMSCHALTEN){ return MESSBEREICH_STREAM; }
					else if(MESSBEREICH_STREAM == st){  return st; }
					else if(st == 1 && MESSBEREICH_STREAM == 2 ){ return 2; }
				}
			}
			switch (  st )
			{	// missing streams use the next existing stream
			case 1:
			case 2:
			case 3:  if ( mkIget(parameter___opt_stream3_method) && mkIget(parameter___streamCount)>=3  ) return (3);
			case 4:  if ( mkIget(parameter___opt_stream4_method) && mkIget(parameter___streamCount)>=4  ) return (4);
			case 5:  if ( mkIget(parameter___opt_stream5_method) && mkIget(parameter___streamCount)>=5  ) return (5);
			case 6:  if ( mkIget(parameter___opt_stream6_method) && mkIget(parameter___streamCount)>=6  ) return (6);
			default: /*TESTmkt_debug("NEXT_STREAM == 0"); */return (0);	// device without stre
			}
		}
		else
		{
			switch ( 1 + stream%6 )
			{	// missing streams use the next existing stream
			case 1:  if ( mkIget(parameter___opt_stream1_method) ) return (1);
			case 2:  if ( mkIget(parameter___opt_stream2_method) && mkIget(parameter___streamCount)>=2  ) return (2);
			case 3:  if ( mkIget(parameter___opt_stream3_method) && mkIget(parameter___streamCount)>=3  ) return (3);
			case 4:  if ( mkIget(parameter___opt_stream4_method) && mkIget(parameter___streamCount)>=4  ) return (4);
			case 5:  if ( mkIget(parameter___opt_stream5_method) && mkIget(parameter___streamCount)>=5  ) return (5);
			case 6:  if ( mkIget(parameter___opt_stream6_method) && mkIget(parameter___streamCount)>=6  ) return (6);
			default: return (1);	// device without streams always uses stream1
			}
		}
	}
	return(1);	// I'm not really sure, if case 6 uses the default above
}


#define SET_TIC_IS_ONLY do{ if(TIC_ONLY(STREAM))SET_TIC(1);}while(0)

#define FIRST_STREAM  NEXT_STREAM(0)

#define SET_FIRST_STREAM_IS_NULL do{ if(STREAM == 0)SET_STREAM(1);}while(0)
#define SINGLE_STREAM (mkIget(parameter___single_stream))

#define SET_NEXT_SIGNAL_STREAM do{ printf(" control does SET_NEXT_SIGNAL_STREAM\n"); mkIset(control_error__E1850,0);\
	if (CALIBRATION) { if (0==TIC) SET_NEXT_CALIBRATION_SIGNAL_STREAM; } \
	else if (!SINGLE_MEASUREMENT){ SET_STREAM(NEXT_STREAM(STREAM));}}while(0)

#define SET_OLD_SIGNAL_STREAM do{ printf(" control does SET_OLD_SIGNAL_STREAM\n"); \
		if (CALIBRATION) { if (0==TIC) SET_NEXT_CALIBRATION_SIGNAL_STREAM; } \
		else if (!SINGLE_MEASUREMENT){ SET_STREAM(STREAM_OLD);}}while(0)

#define SET_ACTIVE_STREAM do{ mkIset(control_active__STREAM,STREAM);}while(0)
#define SET_NEXT_CALIBRATION_SIGNAL_STREAM do{set_next_calibration_signal_stream(STREAM);}while(0)

int32_t
set_next_calibration_signal_stream (int stream)
{
	mkIset(control_mode__CALIBRATION_SIGNAL, 1+mkIget(control_mode__CALIBRATION_SIGNAL));
	if (mkIget(control_mode__CALIBRATION_SIGNAL) > mkIget(parameter___number_of_replicates_cal))
	{
		if(!AUTOCALIBRATION)
		{
			mkIset(control_mode__CALIBRATION_SIGNAL, 1);
			if(NEXT_SOLUTION(SOLUTION))
			{
				SET_NEW_SOLUTION(1);
			}
			SET_STREAM(NEXT_STREAM(0));
		}
		else
		{
			mkIset(control_mode__CALIBRATION_SIGNAL, 1);
			NEXT_SOLUTION(SOLUTION);
			SET_STREAM(0);
		}
	}
}

#define NEXT_RUN do{/*mkt_trace(" control does NEXT_RUN\n");*/ \
	SET_RETRY(0);SET_MULTIPLE_MEASUREMENT(0);if((0==TIC) && NEEDS_TIC) SET_TIC(1);else{ SET_TIC(0);} \
	if(CALIBRATION || WAITING_FOR_VALUE ) SET_MULTIPLE_MEASUREMENT(1); \
	}while(0)



// @TODO: check Z-Max 1680,
// @TODO: ... and limit user volume parameters to allowed range




//#define simulate_Xsensor(s) do{printf(" control simulate_Xsensor - %s\n",s);mkIset(control_Motor1__CommandGoToPos,0);}while(0)
//#define simulate_Ysensor(s) do{printf(" control simulate_Ysensor - %s\n",s);mkIset(control_Motor2__CommandGoToPos,0);}while(0)
//#define simulate_Zsensor(s) do{printf(" control simulate_Zsensor - %s\n",s);Z_std;mkIset(control_Motor3__CommandGoToPos,0);}while(0)
//#define simulate_Vsensor(s) do{printf(" control simulate_Vsensor - %s\n",s);Z_std;mkIset(control_Motor3__CommandGoToPos,0);}while(0)
/*
#define stateYneedle(no,pos,ok,err) \
	state(no," -- stateY?_needle_" QUOTEME(no), Y_std;mkIset(control_Motor2__CommandGoToPos,pos),0) \
	goMs_error(err,POS_TIMEOUT) \
    goIf( 90, 0==WARMING_UP_DONE && !GO_TO_HOLD) \
	goIf(1000,1000==control) \
	goIf(ok,0==NEEDLE_IS && pos==mkIget(node_Motor2__CommandGoToPos))
#define stateFurnaceOpen(no,ok,err) \
	state(no," -- stateFo_FurnaceOpen_" QUOTEME(no), FURNACE_OPEN,0) \
	goMs_error(err,POS_TIMEOUT) \
    goIf( 90, 0==WARMING_UP_DONE && !GO_TO_HOLD) \
	goIf(1000,1000==control) \
	goIf(ok,FURNACE_IS_OPEN)
#define stateFurnaceClose(no,ok,err) \
	state(no," -- stateFc_FurnaceClose_" QUOTEME(no), FURNACE_CLOSE,0) \
	goMs_error(err,POS_TIMEOUT) \
    goIf( 90, 0==WARMING_UP_DONE && !GO_TO_HOLD) \
	goIf(1000,1000==control) \
	goIf(ok,FURNACE_IS_CLOSED)

#define stateTicOpen(no,ok,err) \
	state(no," -- stateTo_TicOpen_" QUOTEME(no), TIC_OPEN,0) \
	goMs_error(err,POS_TIMEOUT) \
    goIf( 90, 0==WARMING_UP_DONE && !GO_TO_HOLD) \
	goIf(1000,1000==control) \
	goIf(ok,TIC_IS_OPEN)
#define stateTicClose(no,ok,err) \
	state(no," -- stateTc_TicClose_" QUOTEME(no), TIC_CLOSE,0) \
	goMs_error(err,POS_TIMEOUT) \
    goIf( 90, 0==WARMING_UP_DONE && !GO_TO_HOLD) \
	goIf(1000,1000==control) \
	goIf(ok,TIC_IS_CLOSED)

#define stateZflush(no,pos,ok,err) \
	state(no," -- stateZ_" QUOTEME(no), Y1Y1;printf(" control Y1Y1\n");Z_std;mkIset(control_Motor3__CommandGoToPos,pos),Z_std;Y1Y1_STD;printf(" control Y1Y1_STD\n");) \
	goMs_error(err,POS_TIMEOUT) \
	goIf(1000,1000==control) \
    goIf( 90, 0==WARMING_UP_DONE && !GO_TO_HOLD) \
	goIf(ok,pos==mkIget(node_Motor3__CommandGoToPos))

#define stateError(no,ok) \
		state(no, " - error retry", LOG_POSITION_DEVIATION;ALL_BREAK;ALL_STD;SET_RETRY(1+RETRY);SET_RINSING(1), 0) \
		goIf( 90, 0==WARMING_UP_DONE) \
		goIf(1000, 1000==control) \
		goIf(1000, RETRY>3) \
		goIf(ok,true) \


#define stateRinsingNeedlePos(no,ok,err)\
		state(no, " - go rinsing pos_" QUOTEME(no),0,0) \
		goIf( 90, 0==WARMING_UP_DONE && !GO_TO_HOLD) \
		goIf(no+1, true)              \
		stateY(no+1, POSyHOLD, no+2, err)\
		stateX(no+2, POSxDRAIN(STREAM), no+3, err) \
		stateY(no+3, POSyDRAIN(STREAM), ok, err)

#define stateFlushNeedle(no,ok,err)\
		state(no, " - rinsing needle_" QUOTEME(no),0,0) \
		goIf( 90, 0==WARMING_UP_DONE && !GO_TO_HOLD)        \
		goIf(no+1, true)                     \
		stateVsensor(no+1,0,no+2,err)        \
		stateZflush(no+2,posZrinse,no+3,err) \
		stateV(no+3,posZhold,ok,err)
*/


int
not_nul_result_rinsing_progress(int count )
{
	if(count==RETRY_INJECTION_SYSTEM) return 90;
	if(count<RETRY_INJECTION_SYSTEM)  return 100;
	printf("RINSING_NIDLE_SET_PROGRESS count = %d RETRY_INJECTION_SYSTEM = %d",count,RETRY_INJECTION_SYSTEM);
	double val = 20.+100./((double)(count-RETRY_INJECTION_SYSTEM));
	if(val <= 0.0)return 100;
	return (int)val;
}

/*#define stateEnjectionSystemFlush(no,ok,err)\
		state(no," -- stateEnjectionSystemFlush" QUOTEME(no), ,0) \*/


int32_t VOL (int stream)
{
	if (0==mkIget(control_mode__TIC))
	{
		return mkIget(parameter___volTcStream1);
	}
	else
	{
		return mkIget(parameter___volTcStream1);
	}
}

int32_t VOLmax ( int stream)
{
	if (0==mkIget(control_mode__TIC))
	 switch ( stream )
	{	// missing streams use volume of last existing stream
		case 6:  if ( mkIget(parameter___opt_stream6_method) ) return mkIget(parameter___volTcMaxStream6);
		case 5:  if ( mkIget(parameter___opt_stream5_method) ) return mkIget(parameter___volTcMaxStream5);
		case 4:  if ( mkIget(parameter___opt_stream4_method) ) return mkIget(parameter___volTcMaxStream4);
		case 3:  if ( mkIget(parameter___opt_stream3_method) ) return mkIget(parameter___volTcMaxStream3);
		case 2:  if ( mkIget(parameter___opt_stream2_method) ) return mkIget(parameter___volTcMaxStream2);
		case 1:  if ( mkIget(parameter___opt_stream1_method) ) return mkIget(parameter___volTcMaxStream1);
		default: return mkIget(parameter___volTcStream1);	// unknown streams always use volume of stream 1
	} else switch ( stream ) // TIC
	{	// missing streams use volume of last existing stream
		case 6:  if ( mkIget(parameter___opt_stream6_method) ) return mkIget(parameter___volTicMaxStream6);
		case 5:  if ( mkIget(parameter___opt_stream5_method) ) return mkIget(parameter___volTicMaxStream5);
		case 4:  if ( mkIget(parameter___opt_stream4_method) ) return mkIget(parameter___volTicMaxStream4);
		case 3:  if ( mkIget(parameter___opt_stream3_method) ) return mkIget(parameter___volTicMaxStream3);
		case 2:  if ( mkIget(parameter___opt_stream2_method) ) return mkIget(parameter___volTicMaxStream2);
		case 1:  if ( mkIget(parameter___opt_stream1_method) ) return mkIget(parameter___volTicMaxStream1);
		default: return mkIget(parameter___volTcStream1);	// unknown streams always use volume of stream 1
	}
}

int32_t
solution_RENTRY(int solution )
{
	switch ( solution )
	{
	case 1:  return mkIget(parameter___calibration_injections_solution1);
	case 2:  return mkIget(parameter___calibration_injections_solution2);
	case 3:  return mkIget(parameter___calibration_injections_solution3);
	case 4:  return mkIget(parameter___calibration_injections_solution4);
	case 5:  return mkIget(parameter___calibration_injections_solution5);
	case 6:  return mkIget(parameter___calibration_injections_solution6);
	case 7:  return mkIget(parameter___calibration_injections_solution7);
	case 8:  return mkIget(parameter___calibration_injections_solution8);
	case 9:  return mkIget(parameter___calibration_injections_solution9);
	case 10: return mkIget(parameter___calibration_injections_solution10);
	default: return mkIget(parameter___calibration_injections_solution1);
	}
}

#define FILL_GAS do{ if(!ONLY_GASFLOW){Y4Y5_NC;Y4Y6_NC;} }while(0)



int
loop_injection_done ()
{
	SET_ONLY_GASFLOW(0);
	if(IS_GAS_CALIBRATION && CALIBRATION)
	{

		if(solution_RENTRY(SOLUTION) == 0
				&& SOLUTION==1
				&& RETRY_LOOP_INJECTION == 1)
		{
			SET_ONLY_GASFLOW(1);
			return 0;
		}
		else
			return solution_RENTRY(SOLUTION) < RETRY_LOOP_INJECTION;
	}
	else if (HS_RINSING)
	{

	}
	else
	{
		return VOL(STREAM)*RETRY_LOOP_INJECTION > VOLmax(STREAM)&&RETRY_LOOP_INJECTION!=1;
	}
}

int
loop_injection_HS_rinsing_done() { return (mkIget(parameter___volHScount) < RETRY_LOOP_INJECTION); }

#define HS_RINSING_INJECTION_DONE (loop_injection_HS_rinsing_done())

#define TIMER_AI (MKiP_control_debug__timer_ai)
#define startTimer_ai(ms) (mktStartTimer(TIMER_AI,ms))
#define timeout_ai() (mktTimeout(TIMER_AI))
#define stopTimer_ai() (mktStopTimer(TIMER_AI))

void
analyse_injection_set_error ( int isError )
{
	switch (STREAM)
	{
	case 1 : // check E1841 "injection error stream 1"
		if (mkIget(control_error__E1841) != isError) mkIset(control_error__E1841  ,  isError); break;
	case 2 : // check E1842 "injection error stream 2"
		if (mkIget(control_error__E1842) != isError) mkIset(control_error__E1842  ,  isError); break;
	case 3 : // check E1843 "injection error stream 3"
		if (mkIget(control_error__E1843) != isError) mkIset(control_error__E1843  ,  isError); break;
	case 4 : // check E1844 "injection error stream 4"
		if (mkIget(control_error__E1844) != isError) mkIset(control_error__E1844  ,  isError); break;
	case 5 : // check E1845 "injection error stream 5"
		if (mkIget(control_error__E1845) != isError) mkIset(control_error__E1845  ,  isError); break;
	case 6 : // check E1846 "injection error stream 6"
		if (mkIget(control_error__E1846) != isError) mkIset(control_error__E1846  ,  isError); break;
	default : // unknown stream always generate  E1841 "injection error stream 1"
		if (mkIget(control_error__E1841) != isError) mkIset(control_error__E1841  ,  isError); break;
	}
}

int
anylyse_injection_done()
{
	int isDone = mkIget(scale_sensor__airflow1) > (1 + mkIget(parameter___injection_error_threshold)/100.0) * mkIget(formula_smooth__airflow1);
	/*mkt_trace("isDone(%d) =  airflow1(%f)> (1+injection_error_threshold(%f)/100)*smooth__airflow1(%f)",
			isDone,mkIget(scale_sensor__airflow1),mkIget(parameter___injection_error_threshold),mkIget(formula_smooth__airflow1));

	mkt_trace( "IS_GAS_CALIBRATION(%d) || TIC(%d)",(IS_GAS_CALIBRATION&&CALIBRATION) ,TIC);*/
	if( (IS_GAS_CALIBRATION&&CALIBRATION) || TIC ) return 1;

	// check injection error
	// error, if biggest airflow OUT during injection <= 1.3 * smoothed value during justification
	// (until 2012-06-16: error, if smallest airflow IN during injection >= 0.5 * smoothed value during justification)
	// 	smoothed value is in formula_smooth__airflow
	// 	biggest (until 2011-06-16: smallest) value is in formula_last__airflow
	// Check new logic
	//mkt_trace("Analyze results");
	if( isDone ){analyse_injection_set_error(0);return 1;}
	if(timeout_ai())
	{
		analyse_injection_set_error(1);
		return 1;
	}
	return 0;
}
float32_t
AC_DEVIATION (int stream )
{
	switch ( stream )
	{	// missing streams use volume of last existing stream
	case 6: return mkIget(parameter___autocalibration_deviation_stream6);
	case 5: return mkIget(parameter___autocalibration_deviation_stream5);
	case 4: return mkIget(parameter___autocalibration_deviation_stream4);
	case 3: return mkIget(parameter___autocalibration_deviation_stream3);
	case 2: return mkIget(parameter___autocalibration_deviation_stream2);
	case 1: return mkIget(parameter___autocalibration_deviation_stream1);
	default: return mkIget(parameter___autocalibration_deviation_stream1);	// unknown streams always use volume of stream 1
	}
}


int32_t RinsingCount (int stream)
{
	switch ( stream )
	{	// missing streams position of last existing stream
	/* not for LOOP 	case 6:  return mkIget(parameter____rinsingCount6);
		case 5:  return mkIget(parameter____rinsingCount5);
		case 4:  return mkIget(parameter____rinsingCount4);
		case 3:  return mkIget(parameter____rinsingCount3);
		case 2:  return mkIget(parameter____rinsingCount2);
		case 1:  return mkIget(parameter____rinsingCount1);
		default: return mkIget(parameter____rinsingCount1);	// unknown streams always use vessel 1*/
	}
	return 1;
}

#define NEED_DILUTION (IsPOSxDILUTION(STREAM))


uint32_t
sample_vessel_filling ( int stream )
{
	switch ( stream )
	{	// missing streams position of last existing stream
	case 6:  return (uint32_t) mkIget(parameter___sample_vessel_filling6);
	case 5:  return (uint32_t) mkIget(parameter___sample_vessel_filling5);
	case 4:  return (uint32_t) mkIget(parameter___sample_vessel_filling4);
	case 3:  return (uint32_t) mkIget(parameter___sample_vessel_filling3);
	case 2:  return (uint32_t) mkIget(parameter___sample_vessel_filling2);
	case 1:  return (uint32_t) mkIget(parameter___sample_vessel_filling1);
	default: return (uint32_t) mkIget(parameter___sample_vessel_filling1);	// unknown streams always use vessel 1
	}
}
uint32_t
prepare_loop_system ( int stream )
{
	switch ( stream )
	{	// missing streams position of last existing stream
	case 6:  return (uint32_t) mkIget(parameter___LoopPrepareTime6);
	case 5:  return (uint32_t) mkIget(parameter___LoopPrepareTime5);
	case 4:  return (uint32_t) mkIget(parameter___LoopPrepareTime4);
	case 3:  return (uint32_t) mkIget(parameter___LoopPrepareTime3);
	case 2:  return (uint32_t) mkIget(parameter___LoopPrepareTime2);
	case 1:  return (uint32_t) mkIget(parameter___LoopPrepareTime1);
	default: return (uint32_t) mkIget(parameter___LoopPrepareTime1);	// unknown streams always use vessel 1
	}
}
uint32_t
fill_loop_system_time( int stream )
{
	switch ( stream )
	{	// missing streams position of last existing stream
		case 6:  return (uint32_t) mkIget(parameter___LoopFillTime6);
		case 5:  return (uint32_t) mkIget(parameter___LoopFillTime5);
		case 4:  return (uint32_t) mkIget(parameter___LoopFillTime4);
		case 3:  return (uint32_t) mkIget(parameter___LoopFillTime3);
		case 2:  return (uint32_t) mkIget(parameter___LoopFillTime2);
		case 1:  return (uint32_t) mkIget(parameter___LoopFillTime1);
		default: return (uint32_t) mkIget(parameter___LoopFillTime1);	// unknown streams always use vessel 1
	}
}

uint32_t
injection_loop_time ( int stream )
{
	switch ( stream )
	{	// missing streams position of last existing stream
	case 6:  return (uint32_t) mkIget(parameter___LoopInjectionTime6);
	case 5:  return (uint32_t) mkIget(parameter___LoopInjectionTime5);
	case 4:  return (uint32_t) mkIget(parameter___LoopInjectionTime4);
	case 3:  return (uint32_t) mkIget(parameter___LoopInjectionTime3);
	case 2:  return (uint32_t) mkIget(parameter___LoopInjectionTime2);
	case 1:  return (uint32_t) mkIget(parameter___LoopInjectionTime1);
	default: return (uint32_t) mkIget(parameter___LoopInjectionTime1);	// unknown streams always use vessel 1
	}
}

uint32_t
fill_acid_time ( int stream )
{
	switch ( stream )
	{	// missing streams position of last existing stream
	case 6:  return (uint32_t) mkIget(parameter___fill_acid6);
	case 5:  return (uint32_t) mkIget(parameter___fill_acid5);
	case 4:  return (uint32_t) mkIget(parameter___fill_acid4);
	case 3:  return (uint32_t) mkIget(parameter___fill_acid3);
	case 2:  return (uint32_t) mkIget(parameter___fill_acid2);
	case 1:  return (uint32_t) mkIget(parameter___fill_acid1);
	default: return (uint32_t) mkIget(parameter___fill_acid1);	// unknown streams always use vessel 1
	}
}

int
control_STRIPPING( int stream )
{
	switch ( stream )
	{	// missing streams position of last existing stream
	case 6:  return  mkIget(parameter___stripping_time6);
	case 5:  return  mkIget(parameter___stripping_time5);
	case 4:  return  mkIget(parameter___stripping_time4);
	case 3:  return  mkIget(parameter___stripping_time3);
	case 2:  return  mkIget(parameter___stripping_time2);
	case 1:  return  mkIget(parameter___stripping_time1);
	default: return  mkIget(parameter___stripping_time1);	// unknown streams always use vessel 1
	}
}

uint32_t
waite_befor_injection_time ( int stream )
{
	switch ( stream )
		{	// missing streams position of last existing stream
		case 6:  return (uint32_t) mkIget(parameter___wait_injection6);
		case 5:  return (uint32_t) mkIget(parameter___wait_injection5);
		case 4:  return (uint32_t) mkIget(parameter___wait_injection4);
		case 3:  return (uint32_t) mkIget(parameter___wait_injection3);
		case 2:  return (uint32_t) mkIget(parameter___wait_injection2);
		case 1:  return (uint32_t) mkIget(parameter___wait_injection1);
		default: return (uint32_t) mkIget(parameter___wait_injection1);	// unknown streams always use vessel 1
		}
}

uint32_t
rinsing_loop_time ( int stream )
{
	switch ( stream )
	{	// missing streams position of last existing stream
	case 6:  return (uint32_t) mkIget(parameter___LoopRinseTime6);
	case 5:  return (uint32_t) mkIget(parameter___LoopRinseTime5);
	case 4:  return (uint32_t) mkIget(parameter___LoopRinseTime4);
	case 3:  return (uint32_t) mkIget(parameter___LoopRinseTime3);
	case 2:  return (uint32_t) mkIget(parameter___LoopRinseTime2);
	case 1:  return (uint32_t) mkIget(parameter___LoopRinseTime1);
	default: return (uint32_t) mkIget(parameter___LoopRinseTime1);	// unknown streams always use vessel 1
	}
}

int
control_delay_TC ( int stream )
{
	switch ( stream )
	{	// missing streams position of last existing stream
	case 6:  return  mkIget(parameter___delay_tc6);
	case 5:  return  mkIget(parameter___delay_tc5);
	case 4:  return  mkIget(parameter___delay_tc4);
	case 3:  return  mkIget(parameter___delay_tc3);
	case 2:  return  mkIget(parameter___delay_tc2);
	case 1:  return  mkIget(parameter___delay_tc1);
	default: return  mkIget(parameter___delay_tc1);	// unknown streams always use vessel 1
	}
}
int
control_delay_TIC ( int stream )
{
	switch ( stream )
	{	// missing streams position of last existing stream
	case 6:  return  mkIget(parameter___delay_tic6);
	case 5:  return  mkIget(parameter___delay_tic5);
	case 4:  return  mkIget(parameter___delay_tic4);
	case 3:  return  mkIget(parameter___delay_tic3);
	case 2:  return  mkIget(parameter___delay_tic2);
	case 1:  return  mkIget(parameter___delay_tic1);
	default: return  mkIget(parameter___delay_tic1);	// unknown streams always use vessel 1
	}
}


uint32_t
purge_time_gasmeasurement ( int stream )
{
	return (uint32_t) mkIget(parameter___purge_time_gas_measurement);

}

uint32_t
cleanup_repetitions ( int stream )
{
	return (uint32_t) mkIget(parameter___cleanup_value_replications);
}

uint32_t
cleanup_time ( int stream )
{
	return (uint32_t) mkIget(parameter___cleanup_value_time);
}



//#define delay mkIget(parameter___delay_tc)
//#define delay_tic mkIget(parameter___delay_tic)
#define DELAY (TIC ?  control_delay_TIC(STREAM) : control_delay_TC(STREAM) )
#define stripping_wait control_STRIPPING(STREAM)

#define nullpunktnahmezeit mkIget(parameter___nullpunktnahmezeit)
//#define t_stop_max mkIget(parameter___t_stop_max_tc)
//#define t_stop_max_tic mkIget(parameter___t_stop_max_tic)
//#define T_STOP_MAX (TIC ? mkIget(parameter___t_stop_max_tic) : mkIget(parameter___t_stop_max_tc))


uint32_t get_stream_analysis_interval(int stream)
{
	switch(stream)
	{
	case 1: return mkIget(parameter___analysis_interval1);
	case 2: return mkIget(parameter___analysis_interval2);
	case 3: return mkIget(parameter___analysis_interval3);
	case 4: return mkIget(parameter___analysis_interval4);
	case 5: return mkIget(parameter___analysis_interval5);
	case 6: return mkIget(parameter___analysis_interval6);
	default: return mkIget(parameter___analysis_interval1);
	}
	return -1;
}

int32_t get_stream_analysis_interval_timeOut(int stream,int restart)
{
	switch(stream)
	{
	case 1: return mktTimeoutDf((MKiP_control_debug__stream1TimerDf),restart);
	case 2: return mktTimeoutDf((MKiP_control_debug__stream2TimerDf),restart);
	case 3: return mktTimeoutDf((MKiP_control_debug__stream3TimerDf),restart);
	case 4: return mktTimeoutDf((MKiP_control_debug__stream4TimerDf),restart);
	case 5: return mktTimeoutDf((MKiP_control_debug__stream5TimerDf),restart);
	case 6: return mktTimeoutDf((MKiP_control_debug__stream6TimerDf),restart);
	default: return mktTimeoutDf((MKiP_control_debug__stream1TimerDf),restart);
	}
}
uint32_t get_stream_analysis_interval_timer(int stream)
{
	switch(stream)
	{
	case 1: return mkIget(control_debug__stream1TimerDf);
	case 2: return mkIget(control_debug__stream2TimerDf);
	case 3: return mkIget(control_debug__stream3TimerDf);
	case 4: return mkIget(control_debug__stream4TimerDf);
	case 5: return mkIget(control_debug__stream5TimerDf);
	case 6: return mkIget(control_debug__stream6TimerDf);
	default: return mkIget(control_debug__stream1TimerDf);
	}
}

int32_t set_stream_analysis_interval_timer(int stream,uint32_t sec)
{
	switch(stream)
	{
	case 1: mkIset( control_debug__stream1TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__stream1TimerDf);
			break;
	case 2: mkIset( control_debug__stream2TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__stream2TimerDf);
			break;
	case 3: mkIset( control_debug__stream3TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__stream3TimerDf);
			break;
	case 4: mkIset( control_debug__stream4TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__stream4TimerDf);
			break;
	case 5: mkIset( control_debug__stream5TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__stream5TimerDf);
			break;
	case 6: mkIset( control_debug__stream6TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__stream6TimerDf);
			break;
	default:mkIset( control_debug__stream1TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__stream1TimerDf);
			break;
	}

}
void init_timer(int stream)
{
	uint32_t sec=0;

	switch ( get_stream_analysis_interval(stream) )
	{
		case 0: /* fastest test mode */	sec=0; break;
		case 1: /* remote control	 */	sec=0; break;
		case 2: /* 1 min */	sec=60; break;
		case 3: /* 2 min */	sec=2*60; break;
		case 4: /* 3 min */	sec=3*60; break;
		case 5: /* 4 min */	sec=4*60; break;
		case 6: /* 5 min */	sec=5*60; break;
		case 7: /* 6 min */	sec=6*60; break;
		case 8: /* 10 min */	sec=10*60; break;
		case 9: /* 12 min */	sec=12*60; break;
		case 10: /* 15 min */	sec=15*60; break;
		case 11: /* 20 min */	sec=20*60; break;
		case 12: /* 30 min */	sec=30*60; break;
		case 13: /* 1 h */	sec=1*60*60; break;
		case 14: /* 2 h */	sec=2*60*60; break;
		case 15: /* 3 h */	sec=3*60*60; break;
		case 16: /* 4 h */	sec=4*60*60; break;
		case 17: /* 6 h */	sec=6*60*60; break;
		case 18: /* 8 h */	sec=8*60*60; break;
		case 19: /* 12 h */	sec=12*60*60; break;
		case 20: /* 24 h */	sec=24*60*60; break;
		default: sec=60; break; // FIX #21 Messintervall 24 h funktioniert nicht
	}
	set_stream_analysis_interval_timer(stream,sec);
}

// Aotocalibration timers

uint32_t get_stream_autocalibration_interval(int stream)
{
	switch(stream)
	{
	case 1: return mkIget(parameter___autocalibration_stream1);
	case 2: return mkIget(parameter___autocalibration_stream2);
	case 3: return mkIget(parameter___autocalibration_stream3);
	case 4: return mkIget(parameter___autocalibration_stream4);
	case 5: return mkIget(parameter___autocalibration_stream5);
	case 6: return mkIget(parameter___autocalibration_stream6);
	default: return mkIget(parameter___autocalibration_stream1);
	}
	return -1;
}



int32_t get_stream_autocalibration_timeOut(int stream,int restart)
{
	switch(stream)
	{
	case 1: return mktTimeoutDf(MKiP_control_debug__autocalibration1TimerDf,restart);
	case 2: return mktTimeoutDf(MKiP_control_debug__autocalibration2TimerDf,restart);
	case 3: return mktTimeoutDf(MKiP_control_debug__autocalibration3TimerDf,restart);
	case 4: return mktTimeoutDf(MKiP_control_debug__autocalibration4TimerDf,restart);
	case 5: return mktTimeoutDf(MKiP_control_debug__autocalibration5TimerDf,restart);
	case 6: return mktTimeoutDf(MKiP_control_debug__autocalibration6TimerDf,restart);
	default: return mktTimeoutDf(MKiP_control_debug__autocalibration1TimerDf,restart);
	}
}
uint32_t get_stream_autocalibration_timer(int stream)
{
	switch(stream)
	{
	case 1: return mkIget(control_debug__autocalibration1TimerDf);
	case 2: return mkIget(control_debug__autocalibration2TimerDf);
	case 3: return mkIget(control_debug__autocalibration3TimerDf);
	case 4: return mkIget(control_debug__autocalibration4TimerDf);
	case 5: return mkIget(control_debug__autocalibration5TimerDf);
	case 6: return mkIget(control_debug__autocalibration6TimerDf);
	default: return mkIget(control_debug__autocalibration1TimerDf);
	}
}

int32_t set_stream_autocalibration_timer(int stream,uint32_t sec)
{
	switch(stream)
	{
	case 1: mkIset( control_debug__autocalibration1TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__autocalibration1TimerDf);
			break;
	case 2: mkIset( control_debug__autocalibration2TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__autocalibration2TimerDf);
			break;
	case 3: mkIset( control_debug__autocalibration3TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__autocalibration3TimerDf);
			break;
	case 4: mkIset( control_debug__autocalibration4TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__autocalibration4TimerDf);
			break;
	case 5: mkIset( control_debug__autocalibration5TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__autocalibration5TimerDf);
			break;
	case 6: mkIset( control_debug__autocalibration6TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__autocalibration6TimerDf);
			break;
	default:mkIset( control_debug__autocalibration1TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__autocalibration1TimerDf);
			break;
	}

}


void init_autocalibration_timer(int stream)
{
	uint32_t sec=0;
	switch (get_stream_autocalibration_interval(stream) )
	{
		case 1: /* 1  d */	sec=24*60*60;    break;
		case 2: /* 2  d */	sec=2*24*60*60;  break;
		case 3: /* 3  d */	sec=3*24*60*60;  break;
		case 4: /* 4  d */	sec=4*24*60*60;  break;
		case 5: /* 5  d */	sec=5*24*60*60;  break;
		case 6: /* 6  d */	sec=6*24*60*60;  break;
		case 7: /* 7  d */	sec=7*24*60*60;  break;
		case 8: /* 14 d */	sec=14*24*60*60; break;
		default:           sec=0;    break;
	}
	set_stream_autocalibration_timer(stream,sec);
}
uint32_t get_stream_check_interval(int stream)
{
	switch(stream)
	{
	case 1: return mkIget(parameter___check_stream1);
	case 2: return mkIget(parameter___check_stream2);
	case 3: return mkIget(parameter___check_stream3);
	case 4: return mkIget(parameter___check_stream4);
	case 5: return mkIget(parameter___check_stream5);
	case 6: return mkIget(parameter___check_stream6);
	default: return mkIget(parameter___check_stream1);
	}
	return 0;
}

int32_t get_stream_check_timeOut(int stream,int restart)
{
	switch(stream)
	{
	case 1: return mktTimeoutDf((MKiP_control_debug__checkStream1TimerDf),restart);
	case 2: return mktTimeoutDf((MKiP_control_debug__checkStream2TimerDf),restart);
	case 3: return mktTimeoutDf((MKiP_control_debug__checkStream3TimerDf),restart);
	case 4: return mktTimeoutDf((MKiP_control_debug__checkStream4TimerDf),restart);
	case 5: return mktTimeoutDf((MKiP_control_debug__checkStream5TimerDf),restart);
	case 6: return mktTimeoutDf((MKiP_control_debug__checkStream6TimerDf),restart);
	default: return 0;
	}
}
uint32_t get_stream_check_timer(int stream)
{
	switch(stream)
	{
	case 1: return mkIget(control_debug__checkStream1TimerDf);
	case 2: return mkIget(control_debug__checkStream2TimerDf);
	case 3: return mkIget(control_debug__checkStream3TimerDf);
	case 4: return mkIget(control_debug__checkStream4TimerDf);
	case 5: return mkIget(control_debug__checkStream5TimerDf);
	case 6: return mkIget(control_debug__checkStream6TimerDf);
	default: return mkIget(control_debug__checkStream1TimerDf);
	}
}

int32_t set_stream_check_timer(int stream,uint32_t sec)
{
	switch(stream)
	{
	case 1: mkIset( control_debug__checkStream1TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__checkStream1TimerDf);
			break;
	case 2: mkIset( control_debug__checkStream2TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__checkStream2TimerDf);
			break;
	case 3: mkIset( control_debug__checkStream3TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__checkStream3TimerDf);
			break;
	case 4: mkIset( control_debug__checkStream4TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__checkStream4TimerDf);
			break;
	case 5: mkIset( control_debug__checkStream5TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__checkStream5TimerDf);
			break;
	case 6: mkIset( control_debug__checkStream6TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__checkStream6TimerDf);
			break;
	default:mkIset( control_debug__checkStream1TimerDf, sec );
			mktStartTimerDf(MKiP_control_debug__checkStream1TimerDf);
			break;
	}
	return 0;

}


void init_check_timer(int stream)
{
	uint32_t sec=0;
	switch (get_stream_check_interval(stream) )
	{
	    case 1: /* 1  d */	sec=24*60*60;              break;
		case 2: /* 2  d */	sec=(uint32_t)24*60*60/2;  break;
		case 3: /* 3  d */	sec=(uint32_t)24*60*60/3;  break;
		case 4: /* 4  d */	sec=(uint32_t)24*60*60/4;  break; //FIX : 24*60*60/4;
		default:            sec=0;                     break;
	}
	set_stream_check_timer(stream,sec);
}

int32_t get_HS_rinsing_timeOut(int restart)
{
	return mktTimeoutDf((MKiP_control_debug__HS_rinsing_timer),restart);

}

static void
init_HS_rinsing_timer()
{
	uint32_t sec=0;
	switch (mkIget(parameter___HS_rinsing_interval) )
	{
	    case 0: /* 1  d */	sec=0;        break;
	    case 1: /* 30 min */sec=3*60;    break;
	 	case 2: /* 1 h */	sec=1*60*60;  break;
	 	case 3: /* 2 h */	sec=2*60*60;  break;
	 	case 4: /* 3 h */	sec=3*60*60;  break;
	 	case 5: /* 4 h */	sec=4*60*60;  break;
	 	case 6: /* 6 h */	sec=6*60*60;  break;
	 	case 7: /* 8 h */	sec=8*60*60;  break;
	 	case 8: /* 12 h */	sec=12*60*60; break;
	    case 9: /* 24 h */	sec=24*60*60; break;
		default:            sec=0;        break;
	}
	mkIset( control_debug__HS_rinsing_timer, sec );
	mktStartTimerDf(MKiP_control_debug__HS_rinsing_timer);
}


void AUTOCALIBRATION_TIMER_RESTART()
{
	int i= 0;
	for(i=1;i<=mkIget(parameter___streamCount);i++){ init_autocalibration_timer(i); }
}
void CHECK_TIMER_RESTART()
{
	int i= 0;
	for(i=1;i<=mkIget(parameter___streamCount);i++){ init_check_timer(i); }
}
void MEASUREMENTS_TIMER_RESTART()
{
	int i= 0;
	for(i=1;i<=mkIget(parameter___streamCount);i++){ init_timer(i); }
}

#define CONTROL_TIMER_RESTART do{printf(" control timer restart\n");\
		MEASUREMENTS_TIMER_RESTART();    \
		AUTOCALIBRATION_TIMER_RESTART(); \
		CHECK_TIMER_RESTART(); init_HS_rinsing_timer();  \
		}while(0)

uint32_t ALL_STREAM_TIME_OUT()
{
	int i= 0;
	int ret = 0;
	for(i=1;i<=mkIget(parameter___streamCount);i++)ret+=get_stream_analysis_interval_timeOut(i,0);
	return ret;
}

uint32_t ALL_CHECK_TIME_OUT()
{
	int i= 0;
	int ret = 0;
	for(i=1;i<=mkIget(parameter___streamCount);i++)ret+=get_stream_check_timeOut(i,0);
	return ret;
}
uint32_t ALL_AUTOCALIBRATION_TIME_OUT()
{
	int i= 0;
	int ret = 0;
	for(i=1;i<=mkIget(parameter___streamCount);i++)ret+=get_stream_autocalibration_timeOut(i,0);
	return ret;
}

int  start_online_stream()
{
	int stream;
	for(stream=1;stream<=mkIget(parameter___streamCount);stream++)
	{
		if ( 1<get_stream_analysis_interval(stream))
		{
			SET_STREAM(stream);
			SET_ACTIVE_STREAM;
			return 1;
		}
	}
	return 0;
}

#define IS_START_ONLINE_STREAM start_online_stream()


int time_out(int stream)
{
	if(stream==0) return 0;
	if ( 0!=mkIget(control_mode__CALIBRATION) )
	{
		debug_out(" measurement timeOut due to calibration mode\n");
		return ( 1 ); // calibration
	}
	if ( 0!=mkIget(control_mode__SINGLE_MEASUREMENT))
	{
		debug_out(" measurement timeOut due to single measurement mode\n");
		return ( 1 );
	}
	if ( 0==get_stream_analysis_interval(stream))
	{
		debug_out(" measurement timeOut due to fastest mode\n");
		return ( 1 ); // fastest mode
	}
	if(mkIget(scale_Digital1__DigitalIn1_16bit11))
	{
		debug_out(" measurement Hold DigitalIn1Bit11 an\n");
		return 0;
	}
	if ( 1==get_stream_analysis_interval(stream))
	{
		debug_out(" measurement timeOut due to remote control (todo)\n");
		if(stream == 1)	return mkIget(scale_Digital1__DigitalIn1_16bit9);
		if(stream == 2)	return mkIget(scale_Digital1__DigitalIn1_16bit10);
		if(stream == 3)	return mkIget(scale_Digital1__DigitalIn1_16bit12);
		if(stream == 4)	return mkIget(scale_Digital2__DigitalIn1_16bit4);
		if(stream == 5)	return mkIget(scale_Digital2__DigitalIn1_16bit5);
		if(stream == 6)	return mkIget(scale_Digital2__DigitalIn1_16bit6);
		return ( 0 );
	}
	if ( 0!=get_stream_analysis_interval_timer( stream ) )
	{	// check interval timeout
		int ret=get_stream_analysis_interval_timeOut(stream,1);
		if (ret){/*DEBUG_MSG_PRINT(fprintf(MKT_DEBUG_OUT,"Time Out Stream %d\n",stream);) */debug_out(" measurement timeOut due to timer\n");}
		return ( ret );
	}
	debug_out(" measurement timeOut due to unexpected default behaviour\n");
	return ( 1 );	// default is fastest mode
}

uint32_t NEXT_STREAM_INTERVAL()
{
	/*DEBUG_MSG_PRINT(fprintf(MKT_DEBUG_OUT,"NEXT_STREAM_INTERVAL Stream %d Interval %d sec=%d\n",STREAM,get_stream_analysis_interval(STREAM),
			get_stream_analysis_interval_timer(STREAM) );)*/
	uint32_t ret = 0;
	if (CALIBRATION )        {    mkt_debug("Run Stream %d Calibration\n",STREAM);     return ( 1 ); }
	if (SINGLE_MEASUREMENT ) {    mkt_debug("Run Stream %d SingleMeasurement\n",STREAM);  return ( 1 ); }
	if (time_out(STREAM)  )  {    mkt_debug("Run Stream %d Time Out\n",STREAM);        return ( 1 ); }
	if (MESSBEREICH_UMSCHALTEN) {  mkt_debug("Run Stream %d MESSBEREICH_UMSCHALTEN\n",STREAM);        return ( 1 ); }
	SET_NEXT_SIGNAL_STREAM;
	return ( 0 );
}

uint32_t IS_NEED_AUTOCALIBRATION(int stream)
{
	if( get_stream_autocalibration_timeOut(stream,1))
	{
		SET_AUTOCAL_STREAM(stream);
		return 1;
	}
	return 0;
}

#define NEED_AUTOCALIBRATION ( (!CALIBRATION)&&(!SINGLE_MEASUREMENT)&&(IS_NEED_AUTOCALIBRATION(STREAM)) )

uint32_t IS_NEED_VALIDIERUNG(int stream)
{
	return mkIget(scale_Digital2__DigitalIn1_16bit7) && mkIget(parameter___gas_validation);
}


#define NEED_VALIDIERUNG ( (!CALIBRATION)&&(!SINGLE_MEASUREMENT)&&(IS_NEED_VALIDIERUNG(STREAM)))

#define NEED_CHECK ( (!CALIBRATION)&&(!SINGLE_MEASUREMENT)&&(get_stream_check_timeOut(STREAM,1)))


#define NEED_HS_RINSING ( (!CALIBRATION)&&(!SINGLE_MEASUREMENT)&&(get_HS_rinsing_timeOut(1)))

// PROGRESS options

int
control_REPLICATES(int stream )
{
	switch( stream )
	{
	case 6:  return  mkIget(parameter___number_of_replicates6);
	case 5:  return  mkIget(parameter___number_of_replicates5);
	case 4:  return  mkIget(parameter___number_of_replicates4);
	case 3:  return  mkIget(parameter___number_of_replicates3);
	case 2:  return  mkIget(parameter___number_of_replicates2);
	case 1:  return  mkIget(parameter___number_of_replicates1);
	default: return  mkIget(parameter___number_of_replicates1);
	}
}



int getProcessReplicates()
{

	int replicate_par = 0;
	int ret =0;
	int repCount = mkIget(measurement___replicateCount);
	if(SINGLE_MEASUREMENT){ replicate_par = control_REPLICATES(STREAM); }
	else if(CALIBRATION)  { replicate_par = mkIget(parameter___number_of_replicates_cal); repCount =  mkIget(control_mode__CALIBRATION_SIGNAL);  }
	else  replicate_par = control_REPLICATES(STREAM);
	ret = replicate_par-repCount;
	if(ret<=0) ret = 1;
	return ret;
}
int getMaxReplicates()
{
	int replicate_par =  control_REPLICATES(STREAM);
	if(SINGLE_MEASUREMENT){ replicate_par = mkIget(parameter___number_of_replicates_single); }
	else if(CALIBRATION)  { replicate_par = mkIget(parameter___number_of_replicates_cal);    }
	return replicate_par;
}

void  setProgress(int pr ,int Replicates)
{
	printf("TEST_PROGRESS:setProgress pr = %d Replicates = %d\n",pr,Replicates);
	double percents;
	double Rep    = (double)Replicates;
	double pr_d   = (double)pr;
	double maxRep = (double)getMaxReplicates();
	percents = (100.0/Rep+1)+(((100.0)/maxRep)*(pr_d/100.));
	int val =(int) percents;
	if(val >100) val = 100;
	if(SINGLE_MEASUREMENT) mkIset(control_subscription__singleMeasurementProgress,val);
	else if(CALIBRATION)   mkIset(control_subscription__calibrationProgress,val);
	else                   mkIset(control_subscription__progress,val);
}

#define REPLICATE getProcessReplicates()

#define SET_MEASUREMENT_PROGRESS(pr) do{ printf(" TEST_PROGRESS:control does SET_MEASUREMENT_PROGRESS %d\n",pr);\
									    setProgress(pr,REPLICATE);\
                                        }while(0)

#define END_MEASUREMENT_PROGRESS do{printf(" TEST_PROGRESS:control does END_MEASUREMENT_PROGRESS\n"); \
		mkIset(control_subscription__singleMeasurementProgress,0);                      \
		mkIset(control_subscription__calibrationProgress,0);                            \
        mkIset(control_subscription__progress,0);                                       \
    }while(0)



#define SET_AIR_FLOW_CALIBRATE_PROGRESS(pr) do{ printf(" TEST_PROGRESS:control does SET_AIR_FLOW_CALIBRATE_PROGRESS %d\n",pr); mkIset(control_subscription__calibrateFlowSensor,pr);}while(0)


#define ALL_ST_Y3_NO do{Y3Y1_NO;Y3Y2_NO;Y3Y3_NO;Y3Y4_NO;Y3Y5_NO;Y3Y6_NO;}while(0)
#define ALL_ST_Y5_NO do{Y5Y1_NO;Y5Y2_NO;Y5Y3_NO;Y5Y4_NO;Y5Y5_NO;Y5Y6_NO;}while(0)
#define ALL_ST_GP_STD do{GP12_STD;GP13_STD;GP14_STD;GP15_STD;GP16_STD;GP17_STD;}while(0)


#define ALL_INITIALIZE do{LOOP_BLOCK_WAIT;Y4Y7_NO;GP10_STD; \
		  	ALL_ST_GP_STD; \
			FURNACE_ON;SET_RETRY(0);/*GP2_STD;*/ \
			ALL_ST_Y3_NO;ALL_ST_Y5_NO;Y5Y9_NO; \
		    Y4Y5_NO;Y4Y6_NO;Y4Y7_NO;Y6Y1_NO;   \
			}while(0)


#define NO_BUTTON do{mkIset(control_gui__enable_green_button,0);mkIset(control_gui__enable_red_button,0);}while(0)
#define RED_BUTTON do{mkIset(control_gui__enable_green_button,0);mkIset(control_gui__enable_red_button,1);}while(0)
#define BUTTON_IS_AUTOSTART do{NO_BUTTON;\
	             if(mkIget(parameter___autostart))mkIset(control_gui__enable_red_button,1);\
                 else mkIset(control_gui__enable_green_button,1);}while(0)



#define GREEN_BUTTON do{mkIset(control_gui__enable_green_button,1);mkIset(control_gui__enable_red_button,0);}while(0)

#define EMERGENCY_STD do{/* @TODO if restarts are needed: ST1_INITIALIZE; */LOOP_BLOCK_WAIT;Y4Y7_NO;\
			GP1_STD; ALL_ST_GP_STD;ALL_ST_Y3_NO;ALL_ST_Y5_NO;\
			Y3Y10_NO;Y4Y5_NO;Y4Y6_NO;Y4Y7_NO;Y6Y1_NO;LOOP_BLOCK_WAIT;Y5Y9_NO;Y3Y9_NO;\
			}while(0)

#define ALL_STD do{/* @TODO if restarts are needed: ST1_INITIALIZE; */Y1_STD;LOOP_BLOCK_WAIT;Y4Y7_NO;\
			FURNACE_ON;\
			GP1_STD; ALL_ST_GP_STD;ALL_ST_Y3_NO;ALL_ST_Y5_NO;\
			Y3Y10_NO;Y4Y5_NO;Y4Y6_NO;Y4Y7_NO;Y6Y1_NO;LOOP_BLOCK_WAIT;Y5Y9_NO;Y3Y9_NO;\
			}while(0)

#define ALL_INACTIVE do{Y1_STD;LOOP_BLOCK_WAIT;Y4Y7_NO; \
			FURNACE_OFF; \
			GP1_STD; ALL_ST_GP_STD;ALL_ST_Y3_NO;ALL_ST_Y5_NO; \
			Y4Y5_NO;Y4Y6_NO; \
			}while(0)

#define ENTRY_OFFLINE do{  \
		}while(0)


#define ENTRY_ONLINE do{   \
	}while(0)
#define ENTRY_MEASUREMENT do{    \
	ALL_STD;Y3Y1_NO;Y3Y2_NO;                      \
	mkIset(control_mode__CALIBRATION,0);  \
	SET_AUTOCALIBRATION(0);               \
	SET_COMPARE(0);                       \
	SET_SINGLE_MEASUREMENT(0);SET_VALIDIERUNG(0);     \
	SET_MESSBEREICH_UMSCHALTEN(0);SET_MESSBEREICH_STREAM(0);    \
	SET_RINSING(1);                       \
	SET_WARTUNG(0);                       \
	SET_RETRY(0);SET_CALIBRATION_SIGNAL(1);SET_STREAM(FIRST_STREAM);SET_ACTIVE_STREAM;SET_TIC(0);SET_MULTIPLE_MEASUREMENT(0); \
	if(TIC_ONLY(STREAM))SET_TIC(1); mkIset(control_error__E1850,0);     \
	}while(0)
#define WAITING_FOR_VALUE 				 (2001==mkIget(measurement_subscription__internalStatus))
#define WAITING_FOR_NEXT_MEASUREMENT	 (2009==mkIget(measurement_subscription__internalStatus))
#define WAITING_FOR_NEXT_MEASUREMENT_NEW (NEXT_STREAM_INTERVAL())

#define NEXT_MEASUREMENT do{  \
     if(0==CALIBRATION)mkIset(control_measurement__control,2000);       \
    }while(0)

#define SET_CALIBRATION_AIRFLOW do{if(CALIBRATION){printf(" control does SET_CALIBRATION_AIRFLOW\n");  \
	mkIset(control_parameter__airflow_cal,(60. * (mkIget(formula_smooth__airflow2))));   }              \
	}while(0)


#define ENTRY_JUSTIFICATION do{mkt_trace(" control does ENTRY_JUSTIFICATION\n");mkIset(control_formula__control,2100);}while(0)
#define EXIT_JUSTIFICATION  do{mkt_trace(" control does EXIT_JUSTIFICATION\n");mkIset(control_formula__control,1000);}while(0)

#define ENTRY_WAIT_ANALYSIS do{/*mkt_trace(" control does ENTRY_BEFORE_ANALYSIS\n");*/mkIset(control_formula__control,2101);}while(0)
#define EXIT_WAIT_ANALYSIS  do{/*mkt_trace(" control does EXIT_BEFORE_ANALYSIS\n");*/mkIset(control_formula__control,100);}while(0)

#define DONE_JUSTIFICATION ( (mkIget(formula_sensor1_subscription__internalStatus) == 2110 || (!mkIget(scale_sensor__ndir1IsAttached)) ) \
				 	&&       (mkIget(formula_sensor2_subscription__internalStatus) == 2110 || (!mkIget(scale_sensor__ndir2IsAttached)) ) \
				 	&&       (mkIget(formula_sensor3_subscription__internalStatus) == 2110 || (!mkIget(scale_sensor__tnbIsAttached))   ) \
				 	&&       (mkIget(formula_sensor4_subscription__internalStatus) == 2110 || (!mkIget(scale_sensor__oxygenIsAttached))) )


#define ENTRY_ANALYSING do{/*mkt_trace(" control does ENTRY_ANALYSING\n");*/mkIset(control_formula__control,2160);}while(0)
#define DONE_ANALYSING ( (!mkIget(scale_sensor__ndir1IsAttached) || mkIget(formula_sensor1_mode__integrationStopped)) \
				 	&&   (!mkIget(scale_sensor__ndir2IsAttached) || mkIget(formula_sensor2_mode__integrationStopped)) \
					&&   (!mkIget(scale_sensor__tnbIsAttached  ) || mkIget(formula_sensor3_mode__integrationStopped)) \
					&&   (!mkIget(scale_sensor__oxygenIsAttached)|| mkIget(formula_sensor4_mode__integrationStopped)) )

#define EXIT_ANALYSING do{/*mkt_trace(" control does EXIT_ANALYSING\n");*/mkIset(control_formula__control,100);}while(0)

#ifndef WITHOUT_AUTOSTART	/* for security reasons we disable it in OEE */
#define READ_ALL_PARAMETERS do{printf(" control READ_ALL_PARAMETERS\n");mkIset(control_parameter__control,21);mkIset(control_calpar__control,21);mkIset(control_node__control,21);}while(0)
#define READ_PARAMETERS do{printf(" control READ_PARAMETERS\n");mkIset(control_parameter__control,21);}while(0)
#else
#define READ_ALL_PARAMETERS do{printf(" control READ_ALL_PARAMETERS WITHOUT_AUTOSTART\n");mkIset(parameter___autostart,0);mkIset(control_parameter__control,21);mkIset(control_calpar__control,21);mkIset(control_node__control,21);}while(0)
#endif

#define CHECK_PARAMETERS do{printf(" control CHECK_PARAMETERS\n");mkIset(control_pc__control,21);}while(0)

#define READ_ALL_PARAMETERS_DONE (mkIget(control_debug__read_all_parameters_done))
#define READ_PARAMETERS_DONE     (mkIget(control_debug__read_parameters_done))
#define CHECK_PARAMETERS_DONE    (mkIget(control_debug__check_parameters_done))
#define STARTUP_IS_MEASUREMENT (mkIget(parameter___autostart))
#define WRITE_STARTUP_FLAGS_IF_CHANGED(i) do{if(i!=mkIget(parameter___autostart)){mkIset(control_parameter__autostart,i);if(mkIget(parameter___autostart))mkt_log("S Device is set to Online Mode");else mkt_log("S Device is set to Online Mode");}}while(0)

#define ENTRY_SELFTEST do{printf(" control does ENTRY_SELFTEST\n"); \
	mkIset(control_node__control,25); \
	mkIset(control_scale__control,25); \
	}while(0)
#define EXIT_SELFTEST do{printf(" control EXIT_SELFTEST\n"); \
	mkIset(control_node__control,100); \
	mkIset(control_scale__control,100); \
	}while(0)

#ifndef WITHOUT_CHECK
#define SELFTEST_DONE ( mkIget(control_debug__selftest_done))
#else
#define SELFTEST_DONE (mkIget(parameter___measure_while_airflowerror)||mkIget(control_debug__selftest_done))
#endif


#define WARTUNG 			(mkIget(control_mode__WARTUNG))
#define SET_WARTUNG(v)do{mkIset(control_mode__WARTUNG,(v));printf("TESTC_SET_NEED_AIR %d ",mkIget(control_debug__WARTUNG));}while(0)

#define SHUTDOWN            (mkIget(control_debug__SHUTDOWN))
#define SET_SHUTDOWN(v)do{mkIset(control_debug__SHUTDOWN,(v));}while(0)




#define ENTRY_WARTUNG do{printf(" control does ENTRY_WARTUNG\n"); \
		SET_WARTUNG(1);                      \
		END_MEASUREMENT_PROGRESS;            \
		}while(0)

#ifndef WITHOUT_CHECK

#define EMERGENCY_FURNACE   (mkIget(control_error__E1810))
#define TEMPERATURE_MONITORING_MIN (mkIget(control_error__E1871))
#define TEMPERATURE_MONITORING_MAX (mkIget(control_error__E1872))
#define TEMPERATURE_MONITORING_REPAIR do{printf(" control does nothing ENTRY_OPERATIONAL\n");mkIset(control_error__E1872,0);}while(0)
#define EMERGENCY_HUMIDITY  (mkIget(control_error__E1815))
#define WARMING_UP_DONE     (mkIget(control_debug__warming_up_done) || WARTUNG )

#else /* not tested (!) */

#define EMERGENCY_FURNACE (0==mkIget(parameter___measure_while_airflowerror)&&(mkIget(control_error__E1810) /* furnace */))
#define EMERGENCY_HUMIDITY (0==mkIget(parameter___measure_while_airflowerror)&&(mkIget(control_error__E1815) /* humidity */))
#define WARMING_UP_DONE (mkIget(parameter___measure_while_airflowerror)||mkIget(control_debug__warming_up_done))

#endif

#define ENTRY_OPERATIONAL do{printf(" control does nothing ENTRY_OPERATIONAL\n");}while(0)
#define EXIT_OPERATIONAL do{printf(" control does nothing EXIT_OPERATIONAL\n");}while(0)
#define ALL_BREAK do{printf(" control does ALL_BREAK\n");\
	    mkIset(control_mode__CALIBRATION,0);\
	    mkIset(control_measurement__control,1000);\
	    SET_SINGLE_MEASUREMENT(0);SET_HS_RINSING(0);				  \
	    EXIT_CALIBRATE_AIRFLOW;   \
	    SET_AUTOCALIBRATION(0);   \
		}while(0)
#define ENTRY_STANDBY do{printf(" control does ENTRY_STANDBY\n");\
			EXIT_ANALYSING;EXIT_CALIBRATION; \
		 if(!WARTUNG){ALL_BREAK;ALL_STD;}}while(0)

#define ENTRY_CALIBRATION do{mkt_log("S Calibrating is started");\
	ALL_STD; ALL_BREAK;\
	mkIset(control_calibration__control,3000);   \
	mkIset(control_mode__CALIBRATION,1);         \
	SET_SINGLE_MEASUREMENT(0);SET_HS_RINSING(0); \
	SET_RINSING(1);SET_NEW_SOLUTION(0);SET_VALIDIERUNG(0);          \
	SET_RETRY(0);SET_CALIBRATION_SIGNAL(1);NEXT_SOLUTION(0);SET_STREAM(NEXT_STREAM(0));SET_TIC(0);SET_MULTIPLE_MEASUREMENT(0);\
	mkIset(control_parameter__calibration_started,mktTimeStrClock(NULL));                                                     \
	if(TIC_ONLY(STREAM))SET_TIC(1);       \
	}while(0)

#define EXIT_CALIBRATION do{printf(" control does EXIT_CALIBRATION\n");\
	if(!WARTUNG){ALL_STD;}   \
	mkIset(control_calibration__CalibrationComplited,mkIget(control_calibration__CalibrationComplited)+1); \
	mkIset(control_mode__CALIBRATION,0);}while(0)
//#define CALIBRATION_DONE (100==mkIget(calibration_subscription__internalStatus))

#define IS_SINGLE_STREAM_ERROR ( (1>mkIget(parameter___single_stream))||((mkIget(parameter___single_stream))>mkIget(parameter___streamCount)))




#define IS_NEED_GAS_CLEANUP ( (IS_GAS_CALIBRATION && CALIBRATION && (SOLUTION== 1)&& (CALIBRATION_SIGNAL==1)) || VALIDIERUNG )




#define ENTRY_SINGLE_MEASUREMENT do{mkt_log("S Single Measurement is started"); \
	ALL_STD; SET_RETRY(0);                \
	SET_SINGLE_MEASUREMENT(1);            \
	mkIset(control_mode__CALIBRATION,0);  \
	SET_MULTIPLE_MEASUREMENT(0);          \
	SET_RINSING(1); SET_HS_RINSING(0);SET_VALIDIERUNG(0);    \
	SET_RETRY(0);SET_CALIBRATION_SIGNAL(1);SET_STREAM(SINGLE_STREAM);SET_TIC(0);SET_MULTIPLE_MEASUREMENT(0); \
	SET_ACTIVE_STREAM; \
	}while(0)

#define ENTRY_INJECTION_SYSTEM_FLUSH do{printf(" control does ENTRY_CALIBRATION\n");  \
		ALL_STD; SET_RETRY(0);SET_RETRY_INJECTION_SYSTEM(0);                          \
		}while(0)

#define ENTRY_CALIBRATE_AIRFLOW do{printf(" control does ENTRY_CALIBRATE_AIRFLOW\n"); \
		RED_BUTTON;ALL_STD;                                                           \
		mkIset(control_scale__control,200);}while(0)
#define EXIT_CALIBRATE_AIRFLOW do{printf(" control does EXIT_CALIBRATE_AIRFLOW\n");}while(0)

#define WAIT_RINSING_SAMPLE (mkIget(parameter___rinsing_sample_time)*1000)

/* parameter___airflow_correction: Service adjustment p4:[%] Correction factor for carrier gas stream. */

			/**
 * transmit the slope/intercept values as necessary
 **/

#define SET_CALIBRATION_PARAMETER(st,v)do{               \
		if(mkIget(calpar___Stream##st##Slope_##v##_on))  \
		{                                                \
			mkIset(control_parameter__Stream##st##Slope_##v##_value,		mkIget(calpar___Stream##st##Slope_##v##_value));    \
			mkIset(control_parameter__Stream##st##Intercept_##v##_value,	mkIget(calpar___Stream##st##Intercept_##v##_value));\
		} }while(0)

void entry_activate_calibration (void)
{

	// AirFlow2(Spannung) Honeywell AWM3300V [0..1000]sccm @ [0..60] l/h @ [1..5]V
	//

	// Soll Default: 30 l/h (=0.5 l/min) wird apokryph bei Kalibrierung gesetzt
	mkIset(control_parameter__airflow, mkIget(parameter___airflow_cal));
	if(IS_GAS_CALIBRATION)
		mkIset(control_parameter__is_gas_calibration,1);
	else
		mkIset(control_parameter__is_gas_calibration,0);
	//Set new calibrations parameter Stream1
	SET_CALIBRATION_PARAMETER(1,TOC);SET_CALIBRATION_PARAMETER(1,TC);
	SET_CALIBRATION_PARAMETER(1,TIC);SET_CALIBRATION_PARAMETER(1,TOCb);
	SET_CALIBRATION_PARAMETER(1,TCb);SET_CALIBRATION_PARAMETER(1,TICb);
	SET_CALIBRATION_PARAMETER(1,CODo);SET_CALIBRATION_PARAMETER(1,TNb);

	//Set new calibrations parameter Stream2
	SET_CALIBRATION_PARAMETER(2,TOC);SET_CALIBRATION_PARAMETER(2,TC);
	SET_CALIBRATION_PARAMETER(2,TIC);SET_CALIBRATION_PARAMETER(2,TOCb);
	SET_CALIBRATION_PARAMETER(2,TCb);SET_CALIBRATION_PARAMETER(2,TICb);
	SET_CALIBRATION_PARAMETER(2,CODo);SET_CALIBRATION_PARAMETER(2,TNb);

	//Set new calibrations parameter Stream3
	SET_CALIBRATION_PARAMETER(3,TOC);SET_CALIBRATION_PARAMETER(3,TC);
	SET_CALIBRATION_PARAMETER(3,TIC);SET_CALIBRATION_PARAMETER(3,TOCb);
	SET_CALIBRATION_PARAMETER(3,TCb);SET_CALIBRATION_PARAMETER(3,TICb);
	SET_CALIBRATION_PARAMETER(3,CODo);SET_CALIBRATION_PARAMETER(3,TNb);

	//Set new calibrations parameter Stream4
	SET_CALIBRATION_PARAMETER(4,TOC);SET_CALIBRATION_PARAMETER(4,TC);
	SET_CALIBRATION_PARAMETER(4,TIC);SET_CALIBRATION_PARAMETER(4,TOCb);
	SET_CALIBRATION_PARAMETER(4,TCb);SET_CALIBRATION_PARAMETER(4,TICb);
	SET_CALIBRATION_PARAMETER(4,CODo);SET_CALIBRATION_PARAMETER(4,TNb);

	//Set new calibrations parameter Stream5
	SET_CALIBRATION_PARAMETER(5,TOC);SET_CALIBRATION_PARAMETER(5,TC);
	SET_CALIBRATION_PARAMETER(5,TIC);SET_CALIBRATION_PARAMETER(5,TOCb);
	SET_CALIBRATION_PARAMETER(5,TCb);SET_CALIBRATION_PARAMETER(5,TICb);
	SET_CALIBRATION_PARAMETER(5,CODo);SET_CALIBRATION_PARAMETER(5,TNb);

	//Set new calibrations parameter Stream6
	SET_CALIBRATION_PARAMETER(6,TOC);SET_CALIBRATION_PARAMETER(6,TC);
	SET_CALIBRATION_PARAMETER(6,TIC);SET_CALIBRATION_PARAMETER(6,TOCb);
	SET_CALIBRATION_PARAMETER(6,TCb);SET_CALIBRATION_PARAMETER(6,TICb);
	SET_CALIBRATION_PARAMETER(6,CODo);SET_CALIBRATION_PARAMETER(6,TNb);
}

//* (1.+1./100  * mkIget(parameter___airflow_correction)))
//			  	    		<    (95./100. * mkIget(parameter___airflow)/60.)) );
float32_t __percent_difference(float old,float new)
{
	//mkt_trace("__percent_difference old=%f;new=%f;percent=%f need=%f\n",old,new,((float32_t) abs((int) (100.-(new/old)*100.))),AC_DEVIATION(STREAM));

	return (float32_t) abs((int) (100.-(new/old)*100.));
}
/*if(((1.+1./100 *mkIget(control_parameter__Stream##st##Slope_##v##_value))<\
//					(100-mkIget(parameter___autocalibration_deviation))/100* mkIget(calpar___Stream##st##Slope_##v##_value))||  \
//					((1.+1./100 *mkIget(control_parameter__Stream##st##Slope_##v##_value))< \
					(100+mkIget(parameter___autocalibration_deviation))/100* mkIget(calpar___Stream##st##Slope_##v##_value)))\*/

/*
#define SET_AUTOCALIBRATION_PARAMETER(st,v)do{                                                                    \
		if( (mkIget(calpar___Stream##st##Slope_##v##_on))&&(AC_DEVIATION(STREAM))>0.0 )  \
		{                                                                                                         \
				if(AC_DEVIATION(STREAM) > __percent_difference(mkIget(control_parameter__Stream##st##Slope_##v##_value),mkIget(calpar___Stream##st##Slope_##v##_value)))\
				{		mkIset(control_parameter__Stream##st##Slope_##v##_value,mkIget(calpar___Stream##st##Slope_##v##_value));                         \
				}                                                                                                                                        \
		} }while(0)

*/
void
set_autocalibration_parameter( int stream , const char *par )
{
	mktItem_t *on   = mktSubscriptionLookupInput(subscription0071,"calpar___Stream%dSlope_%s_on",stream,par);
	mktItem_t *test  = mktSubscriptionLookupInput(subscription0071,"parameter___Stream%dSlope_%s_value",stream,par);
	mktItem_t *new  = mktSubscriptionLookupInput(subscription0071,"calpar___Stream%dSlope_%s_value",stream,par);
	mktItem_t *set  = mktSubscriptionLookupOutput(subscription0071,"control_parameter__Stream%dSlope_%s_value",stream,par);
	//mkt_trace("Parameter %s  check in stream %d",par , stream);
	if(on&&new&&test&&set)
	{
		//mkt_trace("Test all items find ...");
		if( on->value.bool32  )
		{

			//mkt_trace("Parameter %s activate in stream %d",par , stream);
			//mkt_trace("New_slope= %f old slope = %f DEVIATION=%f",new->value.float32,test->value.float32,AC_DEVIATION(STREAM));
			if(AC_DEVIATION(stream)>0.0)
			{
				if(AC_DEVIATION(STREAM) > __percent_difference(new->value.float32,test->value.float32))
				{
					//mkt_trace("Activate New_slope= %f",new->value.float32);
					set->value.float32 = new->value.float32;
					mktTransmitItem(set,MKT_ITEM_QUALITY_GOOD,1);
					if(mkIget(control_parameter__airflow)!=mkIget(parameter___airflow_cal))
						mkIset(control_parameter__airflow, mkIget(parameter___airflow_cal));
				}
			}
		}
	}
}

void entry_activate_autocalibration(void )
{
	// AirFlow2(Spannung) Honeywell AWM3300V [0..1000]sccm @ [0..60] l/h @ [1..5]V
	// Soll Default: 30 l/h (=0.5 l/min) wird apokryph bei Kalibrierung gesetzt
	//mkIset(control_parameter__airflow, mkIget(parameter___airflow_cal));
	// we do *not* check change flags here
	//Set new calibrations parameter Stream1
	set_autocalibration_parameter(AUTOCAL_STREAM,"TOC");
	set_autocalibration_parameter(AUTOCAL_STREAM,"TC");
	set_autocalibration_parameter(AUTOCAL_STREAM,"TIC");
	set_autocalibration_parameter(AUTOCAL_STREAM,"TOCb");
	set_autocalibration_parameter(AUTOCAL_STREAM,"TCb");
	set_autocalibration_parameter(AUTOCAL_STREAM,"TICb");
	set_autocalibration_parameter(AUTOCAL_STREAM,"CODo");
	set_autocalibration_parameter(AUTOCAL_STREAM,"TNb");

}
#undef SET_AUTOCALIBRATION_PARAMETER
#undef SET_CALIBRATION_PARAMETER

#define __ACTIVATE_CALIBRATION__(txt) \
	do {\
		printf (" control does %s\n", txt);\
		entry_activate_calibration(); \
		mkIset(control_calpar__control,31); \
		mkIset(control_parameter__calibration_activated,mktTimeStrClock(NULL)); \
		mkt_log("S activate calibration");\
	}\
	while(0)

#define __ACTIVATE_AUTOCALIBRATION__(txt) \
	do {\
		printf (" control does %s\n", txt);\
		entry_activate_autocalibration(); \
		mkIset(control_calpar__control,31); \
		mkIset(control_parameter__calibration_activated,mktTimeStrClock(NULL)); \
		mkt_log("S activate autocalibration");\
	}\
	while(0)

#define ENTRY_ACTIVATE_CALIBRATION      __ACTIVATE_CALIBRATION__ ("ENTRY_ACTIVATE_CALIBRATION")
#define ENTRY_ACTIVATE_AUTOCALIBRATION  __ACTIVATE_AUTOCALIBRATION__ ("ENTRY_ACTIVATE_AUTOCALIBRATION")

#define ENTRY_AUTO_CALIBRATION do{printf(" control does ENTRY_AUTO_CALIBRATION\n"); \
	ALL_STD; \
	mkIset(control_calibration__control,3000); \
	mkIset(control_mode__CALIBRATION,1); \
	SET_SINGLE_MEASUREMENT(0);SET_VALIDIERUNG(0);\
	SET_AUTOCALIBRATION(1);   \
	SET_OLD_STREAM(STREAM);   \
	SET_RETRY(0);SET_CALIBRATION_SIGNAL(1);NEXT_SOLUTION(0);SET_STREAM(AUTOCAL_STREAM);SET_TIC(0);SET_MULTIPLE_MEASUREMENT(0);\
	mkIset(control_parameter__calibration_started,mktTimeStrClock(NULL)); \
	if(TIC_ONLY(STREAM))SET_TIC(1);      \
	}while(0)
#define CALCULATE_AUTO_CALIBRATION_OUTLIERS do{printf(" control does  CALCULATE_AUTO_CALIBRATION_OUTLIERS\n"); \
	mkIset(control_calibration__CalibrationComplited,mkIget(control_calibration__CalibrationComplited)+1); \
	}while(0)
#define EXIT_AUTO_CALIBRATION do{printf(" control does  EXIT_AUTO_CALIBRATION\n"); \
	mkIset(control_mode__CALIBRATION,0); \
    SET_AUTOCALIBRATION(0);              \
    SET_STREAM(STREAM_OLD);              \
	}while(0)
#define AUTO_CALIBRATION_DONE (100==mkIget(calibration_subscription__internalStatus))

#define ENTRY_VALIDIERUNG do{printf(" control does ENTRY_VALIDIERUNG\n"); \
	ALL_STD; \
	SET_SINGLE_MEASUREMENT(0);\
	SET_VALIDIERUNG(1);\
	SET_RETRY(0);SET_STREAM(NEXT_STREAM(0));SET_TIC(0);SET_MULTIPLE_MEASUREMENT(0);\
	if(TIC_ONLY(STREAM))SET_TIC(1);      \
	}while(0)

#define EXIT_VALIDIERUNG do{printf(" control does ENTRY_VALIDIERUNG\n"); \
		ALL_STD; \
		SET_TIC(0);  \
		SET_VALIDIERUNG(0);\
}while(0)

#define ENTRY_CHECK do{printf(" control does ENTRY_CHECK\n"); \
		SET_COMPARE(1);            \
		SET_OLD_STREAM(STREAM);    \
		mkt_log("S check  start"); \
		}while(0)
#define EXIT_CHECK do{printf(" control does  EXIT_CHECK\n"); \
		if(COMPARE){ SET_STREAM(STREAM_OLD);\
		mkt_log("S check  exit");    }      \
        SET_COMPARE(0);                     \
	}while(0)

#define ENTRY_SAVE_CALIBRATIONS_PARAMETER do{printf(" control does ENTRY_SAVE_CALIBRATIONS_PARAMETERdo\n"); \
		mkIset(control_calpar__control,31); \
		}while(0)

#define SAVE_CALIBRATIONS_PARAMETER_DONE (100==mkIget(calpar_subscription__internalStatus))




// state machine handling of output, state, and error
static char external_state_description[1025]="";
static char internal_state_description[1025]="";
static char position_state_description[1025]="";
static char internal_control_state[1025]="";
int getState( void ) {	return( mkIget(control_subscription__internalStatus) ); }
int getError( void ) {	return( mkIget(control_subscription__errorCame) ); }
int cameError(void) {
// @TODO: trial error transmission type3 to fix error gone problem - find cameError goneError
	if ( 1&debugMode  ) printf(" control:error %d came (was %d)\n", getState(), getError());
	mkIset(control_subscription__errorCame,getState()); }
int goneError(void) {
// @TODO: trial error transmission type3 to fix error gone problem - find cameError goneError
	if ( 1&debugMode  ) printf(" control:error %d gone (was %d)\n", getState(), getError());
	mkIset(control_subscription__errorGone,getState()); }

/*
R�tsel: control error type3 : Manchmal scheint ein E.... gone zu fehlen

2011-02-21T10:19:32.336342842 S stateFurnaceOpen_2128
2011-02-21T10:19:42.339234551 S ErrorRetry
2011-02-21T10:19:42.341239275 E E2128 came

2011-02-21T12:15:47.844163112 S stateFurnaceOpen_2128		war ok, aber "E2128 gone" fehlte
2011-02-21T12:28:18.316311240 S stateFurnaceOpen_2128		war ok, aber "E2128 gone" fehlte


2011-02-21T12:15:47.843941575 5 -1 control_subscription__timerError
2011-02-21T12:15:47.843973423 5 2126 control_subscription__errorGone
2011-02-21T12:15:47.843993816 5 2128 control_subscription__internalStatus
...
2011-02-21T12:15:51.480043827 5 -1 control_subscription__timerError
2011-02-21T12:15:51.480082659 5 2130 control_subscription__internalStatus		hier fehlt ein 2128 Gone
...
2011-02-21T12:15:53.484969594 5 -1 control_subscription__timerError
2011-02-21T12:15:53.485002839 5 2130 control_subscription__errorGone
2011-02-21T12:15:53.485023232 5 2132 control_subscription__internalStatus
2011-02-21T12:15:53.485042509 5 2132 control_subscription__status


2011-02-21T10:15:53.052284034 5 -1 control_subscription__timerError
2011-02-21T10:15:53.052321190 5 2128 control_subscription__errorGone			hier war ein 2128 Gone
2011-02-21T10:15:53.052342142 5 2130 control_subscription__internalStatus
*/

#ifdef STATE_MACHINE_TEST
#define stateMachine(st,desc) \
	char *state_machine_description=desc; \
	char *state_description=""; \
	int state_machine( int control, int entry, int exit) { int state; int error; \
	state = getState(); \
	error = getError(); \
	/* TEST: */ printf("step starts with state=%d error=%d control=%d, entry=%d, exit=%d\n", state, error, control, entry, exit); /**/ \
	switch ( state ) { default: stateChange(0);

#define stateMachineEnd ;break; } /* TEST: */ printf("step end with state=%d error=%d control=%d, entry=%d, exit=%d\n", state, error, control, entry, exit); /**/ return( getError() );}
#endif

// int stateChange(int state) allows overwriting of next state.
//  It is called after calling the exit function of the last state
//  and before setting the next state.
//  It is typically used to propagate state number.
int stateChange( int state ) {
//TEST:	if ( 3090==getState())	printf(" control_subscription__internalStatus 3090 stateChange\n"); if ( 3090==state)	printf(" control_subscription__internalStatus stateChange to 2090\n");
//DIRTY HACK:
	if ( 3010==getState())	printf(" %s:%u: control_subscription__internalStatus 3010 tries stateChange to %d\n", __FILE__, __LINE__, state);
	if ( 3010==getState() && 1000 != state)
	{
		printf(" %s:%u: going to 1000 using a dirty hack\n", __FILE__, __LINE__);
		state = 1000;
	}
	//printf("CHANGE_STATE %d from %d Error %d\n" , state,getState() , getError());
	if ( getState() != getError() ) goneError();	// @TODO: trial to fix error gone problem by separating exit and entry
	if ( state == getError() ) goneError();
	// initialize external status and description
	if (mkIisNull(control_subscription__status))
	{
//TEST: printf(" control_subscription__internalStatus status init - status is NULL\n");
		mkIset(control_subscription__status, 0);
		mkIset(control_subscription__statemachineDescription, state_machine_description);
	}
	// set internal state
//TEST:	if ( 2003==getState())	printf(" control_subscription__internalStatus 2003 state was %d\n",mkIget(control_subscription__internalStatus));
//TEST:
if ( state == mkIget(control_subscription__internalStatus)) printf(" control error UNEXPECTED stateChange to same state %d\n",state);
	mkIset(control_subscription__internalStatus,state);
//TEST:	if ( 2003==getState())	printf(" control_subscription__internalStatus 2003 state is now %d\n",mkIget(control_subscription__internalStatus));

	return(state);
}

// void stateEnter(int state) allows actions when entering a new state.
// 	It is called after switching to the next state
//  and before calling the entry function of the next state
//  It is typically used to propagate state number and state_description.
void stateEnter( int state )
{
	if ( ' ' != *state_description)	// external state like "measurement"
	{
		if ( state!=mkIget(control_subscription__status))
		{
			// new external state: set status number and description
			mkIset(control_subscription__status,state);
			strncpy(external_state_description, state_description, 1024);
			mkIset(control_subscription__stateDescription, external_state_description);
			// clear internal description
			mkIset(control_subscription__internalStateDescription, "");
			mkIset(control_subscription__positionStateDescription, "");
		}
	}
	else if (state_description==strstr(state_description, " - "))
	{	// internal state like " - filling sample"

		// set internal description
		strncpy(internal_state_description, state_description, 1024);
		mkIset(control_subscription__internalStateDescription, internal_state_description);
		// clear position description
		mkIset(control_subscription__positionStateDescription, "");
	} else if (state_description==strstr(state_description, " -- state"))
	{	// position state state like " -- stateX_2138", also Y Z V, X0 Y0 Z0 V0
		// also stateY?_needle_.., "stateFo_FurnaceOpen" and ~Fc_Close

		// set position description
		strncpy(position_state_description, state_description+strlen(" -- state"), 1024);
		if (position_state_description[2]='_') position_state_description[2]='\0'; // X0 Y? Fo etc.
		if (position_state_description[1]='_') position_state_description[1]='\0'; // X Y Z etc.
		mkIset(control_subscription__positionStateDescription, position_state_description);
	} else
	{	// unknown format
		strncpy(position_state_description, state_description, 1024);
		mkIset(control_subscription__positionStateDescription, position_state_description);
	}

	char sol[9]="";
	if(CALIBRATION)
	{
		snprintf(sol,9,"sol%d ",SOLUTION);
	}
	// create gui status line information
	snprintf(internal_control_state, 1024
	, " %s state %u %sst%u/%u%s%s: %s%s"
	, state_machine_description
	, mkIget(control_subscription__internalStatus)
	, sol
	, mkIget(control_mode__STREAM)
	, CALIBRATION?mkIget(control_mode__CALIBRATION_SIGNAL):mkIget(measurement___replicateCount)
	, mkIget(control_mode__TIC) ? "tic" : ""
	, *mkIget(control_subscription__positionStateDescription) ? mkIget(control_subscription__positionStateDescription) : ""
	, mkIget(control_subscription__stateDescription)
	, mkIget(control_subscription__internalStateDescription)
	);
	mkIset(control_subscription__internalControlState, internal_control_state);
}

enum
{
	C_WARMING_UP     = 90,
	C_OPERATION      = 100,
	C_OFFLINE        = 1000,
	C_MEASUREMENT    = 2000,
	C_RINSE_SHUTDOWN = 5040,
	C_RINSE_HS       = 5200

};

#include "tmp/control.sm.c"

int subscriptionTriggerCallback(mktSubscription_t *subscription)
{

	// update timer

	if ( mkIpropagate(pc_system__date) )
	{
//TEST: printf(" updating timer due to system date change");

		// retransmitting timers with new time stamp
		// works as if restarting them from the current second

		// timerMs starts another time period from now on
		mktTransmitItem(TIMER_DELAY,MKT_ITEM_QUALITY_GOOD,1);
		mktTransmitItem(TIMER_ERROR,MKT_ITEM_QUALITY_GOOD,1);

		// timerAt finishes waiting action, or acts later on this day
		mktTransmitItem(TIMERAT_AUTO_CALIBRATION,MKT_ITEM_QUALITY_GOOD,1);

		// timerDf acts each given seconds from now on
		mktTransmitItem((MKiP_control_subscription__timerDf_E1835),MKT_ITEM_QUALITY_GOOD,1);
		mktTransmitItem((MKiP_control_subscription__timerDf_E1815),MKT_ITEM_QUALITY_GOOD,1);
		mktTransmitItem((MKiP_control_subscription__timerDfMem),MKT_ITEM_QUALITY_GOOD,1);
		MEASUREMENTS_TIMER_RESTART();
		AUTOCALIBRATION_TIMER_RESTART();
		CHECK_TIMER_RESTART();
		init_HS_rinsing_timer();
		//init_autocalibration_timer();

	}

	// progressbar for gui

	uint32_t progress = 0;
	uint32_t sec = mktNow().tv_sec - mkIgetDate(control_subscription__status).tv_sec;
	uint32_t duration = 9999;
	switch ( mkIget(control_subscription__status) )
	{
		case 2060:  duration = sample_vessel_filling(STREAM);
					progress = 1 + 99. * sec / duration;
					break;

//@TODO		case 2100:  progress = mkIget(formula_subscription__progress);
//@TODO					break;

//@TODO		case 2160:  progress = mkIget(formula_subscription__progress);
//@TODO					break;

		//case 3100: 	progress = mkIget(calibration_subscription__progress);
					//break;

		default: 	progress = 0;
	}
	if ( progress != mkIget(control_subscription__progress))
	{
		mkIset(control_subscription__progress, progress);
	}

	return( mkIisNull(control_monitor__internalStatus)
		 || mktTimeout(TIMER)
		 || mktTimeout(TIMER_DELAY)
		 || mktTimeout(TIMER_ERROR)
		 || mktTimeoutDf((MKiP_control_subscription__timerDf_E1835),1)
		 || mktTimeoutAt(TIMERAT_AUTO_CALIBRATION)
		 || mktTimeoutDf((MKiP_control_subscription__timerDf_E1815),1)
		 || mktTimeoutDf((MKiP_control_subscription__timerDfMem),1)
		 || ALL_STREAM_TIME_OUT()
		 || ALL_AUTOCALIBRATION_TIME_OUT()
		 || ALL_CHECK_TIME_OUT()
		 );
}

#define CONTROL_IS_INJECTION_TC_ACTION       (mkIget(control_subscription__internalStatus)>=2220 && mkIget(control_subscription__internalStatus)<=2238)

#define E1835_LAST_POS  9
static int E1835_array[E1835_LAST_POS+1] = {0};


void
E1835_array_set_value(int error)
{
	int i ;
	for(i=1;i<=E1835_LAST_POS;i++) E1835_array[i-1] = E1835_array[i];
	E1835_array[E1835_LAST_POS] = error;
}

int
E1835_array_is_error()
{
	int i ;
	for(i=0;i<=E1835_LAST_POS;i++)	if(E1835_array[i]==0) return 0;
	return 1;
}

int
control_is_online()
{
	if(2000 <  mkIget(control_subscription__internalStatus) && mkIget(control_subscription__internalStatus)< 3000 )
	{
		if(!SINGLE_MEASUREMENT && ! CALIBRATION)
		{
			return 1;
		}
	}
	return 0;
}


mktItem_t*
get_stream_analysis_interval_item(int stream)
{
	switch(stream)
	{
	case 1: return MKiP_control_debug__stream1TimerDf;
	case 2: return MKiP_control_debug__stream2TimerDf;
	case 3: return MKiP_control_debug__stream3TimerDf;
	case 4: return MKiP_control_debug__stream4TimerDf;
	case 5: return MKiP_control_debug__stream5TimerDf;
	case 6: return MKiP_control_debug__stream6TimerDf;
	default: return MKiP_control_debug__stream1TimerDf;
	}
}


void
check_next_measurement_time ( )
{
	int stream ;
	for (stream=1;stream<=6;stream++)
	{
		char *val=NULL;
		if (!control_is_online())
		{
			val = mkt_strdup(_TR_("TRANSLATION_market_Offline","off line"));
		}
		else if ( 0==get_stream_analysis_interval(stream))
		{
			val = mkt_strdup(_TR_("TRANSLATION_market_FastestMode","fastest mode"));
		}
		else if ( 1==get_stream_analysis_interval(stream))
		{
			val = mkt_strdup(_TR_("TRANSLATION_market_RemoteControl","remote control"));
		}

		else
		{
			mktItem_t *titem = get_stream_analysis_interval_item(stream);
			mktTime_t expire;
			struct tm *pTm;
			pTm = gmtime( &titem->date.tv_sec);
			pTm->tm_sec += titem->value.uint32;
			expire.tv_sec = timegm(pTm);
			expire.tv_nsec = 0;
			val = mkt_strdup(mktTimeStrClockHMDMY(&expire));
			//	mkIset()
		}
		if(val != NULL)
		{
			char *iname = mkt_strdup_printf("control___nextMeasurementStream%d",stream);
			mktItem_t *it = mktSubscriptionLookupOutputItem(subscription0071,iname);free(iname);
			if(it!=NULL && it->type == MKT_ITEM_TYPE_string32)
			{
				it->value.string32 = val;
				mktSetString(it);
				mktTransmitItem(it,MKT_ITEM_QUALITY_GOOD,1);
			}
			free(val);
		}

	}
}


int
subscriptionCallback(mktSubscription_t *subscription)
{
	int error;
	int control = -1;	// -1:no command
	char s[100];
	//TEST:printf(" subscriptionCallback %s\n",subscription->name);
	// get digital out to shadow
    digitalOutShadow = mkIget(control_Digital1__DigitalOut1_16bit)
    				 |(mkIget(control_Digital2__DigitalOut1_16bit)<<16);
#ifdef MKT_NODE_CONTROL_DIGITAL_3
    digital3OutShadow = mkIget(control_Digital3__DigitalOut1_16bit);
#endif
    //timer init
    if (mkIisChanged(parameter___analysis_interval1))init_timer(1);
    if (mkIisChanged(parameter___analysis_interval2))init_timer(2);
    if (mkIisChanged(parameter___analysis_interval3))init_timer(3);
    if (mkIisChanged(parameter___analysis_interval4))init_timer(4);
    if (mkIisChanged(parameter___analysis_interval5))init_timer(5);
    if (mkIisChanged(parameter___analysis_interval6))init_timer(6);
    if (mkIisChanged(parameter___autocalibration_stream1))init_autocalibration_timer(1);
    if (mkIisChanged(parameter___autocalibration_stream2))init_autocalibration_timer(2);
    if (mkIisChanged(parameter___autocalibration_stream3))init_autocalibration_timer(3);
    if (mkIisChanged(parameter___autocalibration_stream4))init_autocalibration_timer(4);
    if (mkIisChanged(parameter___autocalibration_stream5))init_autocalibration_timer(5);
    if (mkIisChanged(parameter___autocalibration_stream6))init_autocalibration_timer(6);
    if (mkIisChanged(parameter___check_stream1))init_check_timer(1);
    if (mkIisChanged(parameter___check_stream2))init_check_timer(2);
    if (mkIisChanged(parameter___check_stream3))init_check_timer(3);
    if (mkIisChanged(parameter___check_stream4))init_check_timer(4);
    if (mkIisChanged(parameter___check_stream5))init_check_timer(5);
    if (mkIisChanged(parameter___check_stream6))init_check_timer(6);
    if (mkIisChanged(parameter___HS_rinsing_interval))init_HS_rinsing_timer();

	// error check
    int isError = 0;

	// check E1810 "furnace emergency off"
	isError = FURNACE_IS_DEAD;
	if (mkIget(control_error__E1810) != isError)
	{
		mkIset(control_error__E1810  ,  isError);
	}

	if(mkIget(parameter___temperatur_monitoring))
	{
		if(mkIisChanged(node_Analog1__TemperatureIn2))
		{
			double temp2 = mkIget(node_Analog1__TemperatureIn2)/10.0;
			if(mkIget(parameter___temperatur_monitoring_max)<temp2)
			{
				if(mkIget(control_error__E1872)!=1)
					mkIset(control_error__E1872 , 1);
			}
		}
	}
	else
	{
		if(mkIget(control_error__E1872)!=0)
			mkIset(control_error__E1872,0);
	}


	// check E1815 "humidity emergency off"
	// error, if Humidity > parameter___threshold_humidity (default 60 %RH)
	// for more than 30 seconds (until clv2011-06-08 no duration was checked)
	//
	if mkIisNull(control_subscription__timerDf_E1815)
	{
		mktStartTimerDf(MKiP_control_subscription__timerDf_E1815);	// init
	}

	isError = (mkIisInitialized(scale_sensor__humidity)
	    && mkIisInitialized(parameter___threshold_humidity)
		&& (mkIget(scale_sensor__humidity)
		    > 1./100. * mkIget(parameter___threshold_humidity)) // default 60.00 %
		  ||mkIget(scale_sensor__humidity) < -1./100.);			// cable broken shows -29%
	if (mkIget(control_debug__E1815) != isError)  mkIset(control_debug__E1815  ,  isError);

	isError = mkIget(control_debug__E1815)
	 && (mktTime2sec(mktNow()) - mktTime2sec(mkIgetDate(control_debug__E1815)))			// .. for some seconds
	    >mkIget(control_subscription__timerDf_E1815);

	if (mkIget(control_error__E1815) != isError)
	{
		mkIset(control_error__E1815  ,  isError);
	}

	// check E1820 "furnace deviation band alarm"
	isError = FURNACE_IS_OUT_OF_RANGE;
	if (mkIget(control_error__E1820) != isError)
	{
		mkIset(control_error__E1820  ,  isError);
	}

	// check E1830 "cooler deviation band alarm"
	isError = COOLER_IS_OUT_OF_RANGE;
	if (mkIget(control_debug__E1830) != isError)  mkIset(control_debug__E1830  ,  isError);

	isError = mkIget(control_debug__E1830)
		 && (mktTime2sec(mktNow()) - mktTime2sec(mkIgetDate(control_debug__E1830)))			// .. for some seconds
		    >mkIget(control_subscription__timerDf_E1830);
	if (mkIget(control_error__E1830) != isError)
	{
		mkIset(control_error__E1830  ,  isError);
	}

	// check E1833 "pressure is high"
	// Pressure �ber 700 mbar (normal ca. 25 mbar) - Ofen verstopft, Ofenfu� k�nnte abplatzen
	isError = mkIget(scale_sensor__pressure) > 0.6;
	if (mkIget(control_error__E1833) != isError)
	{
		mkIset(control_error__E1833  ,  isError);
	}

	// check E1835 "carrier gas flow OUT is low"
	// error, if Airflow Out < parameter___airflow (default 30 l/h) for more than 60 seconds
	// airflow defaults to 30 l/h, sensor shows [0..1000] sccm = [0..1] l/min = [0..60] l/h
	//
	// Soll Default: 30 l/h (=0.5 l/min) wird bei Kalibrierung gesetzt
	// 		clv2011-05-13: take 90 % of calibration value
	if mkIisNull(control_subscription__timerDf_E1835)
	{
		mktStartTimerDf(MKiP_control_subscription__timerDf_E1835);	// init
	}

	if((mktTime2sec(mktNow()) - mktTime2sec(mkIgetDate(control_debug__E1835)))>mkIget(control_subscription__timerDf_E1835) )
	{
		isError = (mkIisInitialized(scale_sensor__airflow2)
				&& mkIisInitialized(parameter___airflow_correction)
				&& ((( mkIget(scale_sensor__airflow2))
						* (1.+1./100  * mkIget(parameter___airflow_correction)))
						<    (95./100. * mkIget(parameter___airflow)/60.)) );	// .. or is low	clv2011-05-13: take 95 % of calibration value

		mkIset(control_debug__E1835  ,  isError);
		E1835_array_set_value(isError);
		isError = E1835_array_is_error();
		if(!mkIget(parameter___measure_while_airflowerror))
		{
			if (mkIget(control_error__E1835) !=isError )
			{
				mkIset(control_error__E1835  , isError);
			}
			if (mkIget(control_error__E1836) !=0 )
			{
				mkIset(control_error__E1836,0);
			}
		}
		else
		{
			if (mkIget(control_error__E1836) !=isError )
			{
				mkIset(control_error__E1836  , isError);
			}
			if(mkIget(control_error__E1835) != 0 )
			{
				mkIset(control_error__E1835  , 0);
			}
		}

	}


	if(control_is_online())
	{
		if( (mkIpropagate(scale_sensor__ndir1IncreasesOne) && mkIget(scale_sensor__ndir1IsAttached))
			||(mkIpropagate(scale_sensor__ndir2IncreasesOne) && mkIget(scale_sensor__ndir2IsAttached))
			||(mkIpropagate(scale_sensor__oxygenIncreasesOne) && mkIget(scale_sensor__oxygenIsAttached))
			||(mkIpropagate(scale_sensor__tnbIncreasesOne) && mkIget(scale_sensor__tnbIsAttached))
			)
		{
			isError = (   mkIget(scale_sensor__ndir1IncreasesOne) && (mkIget(scale_sensor__ndir1IsAttached) )
						||(mkIget(scale_sensor__ndir2IncreasesOne) && mkIget(scale_sensor__ndir2IsAttached))
						||(mkIget(scale_sensor__oxygenIncreasesOne) && mkIget(scale_sensor__oxygenIsAttached))
						||(mkIget(scale_sensor__tnbIncreasesOne) && mkIget(scale_sensor__tnbIsAttached)) );

			if(mkIget(control_error__E1850) ==0 && mkIget(control_error__E1850) !=isError )
			{
				mkIset(control_error__E1850,isError);
			}
		}
	}

	// check E1851 "zero signal NDIR1 sensor out of range"
	if ( mkIget(scale_sensor__ndir1IsAttached)
	 && mkIpropagate(formula_sensor1_last__zero) )
	{
		isError = (0.02 > mkIget(formula_sensor1_last__zero)
		        || 0.2   < mkIget(formula_sensor1_last__zero) );
		// error, if scaled sensor signal is < 0.01 or >0.3 FSR
		if (mkIget(control_error__E1851) != isError) mkIset(control_error__E1851  ,  isError);
	}
	// check E1852 "zero signal NDIR2 sensor out of range"
	if ( mkIget(scale_sensor__ndir2IsAttached)
	 && mkIpropagate(formula_sensor2_last__zero) )
	{
		isError = (0.02 > mkIget(formula_sensor2_last__zero)
		        || 0.2   < mkIget(formula_sensor2_last__zero) );
		// error, if scaled sensor signal is < 0.01 or >0.3 FSR
		if (mkIget(control_error__E1852) != isError) mkIset(control_error__E1852  ,  isError);
	}
	// check E1853 "zero signal TNb sensor out of range"
	if ( mkIget(scale_sensor__tnbIsAttached)
	 && mkIpropagate(formula_sensor3_last__zero) )
	{
		isError = (0.02 > mkIget(formula_sensor3_last__zero)
		        || 0.2  < mkIget(formula_sensor3_last__zero) );
		// error, if scaled sensor signal is < 0.01 or >0.3 FSR
		if (mkIget(control_error__E1853) != isError) mkIset(control_error__E1853  ,  isError);
	}
	// check E1854 "zero signal COD_o sensor out of range"
	if ( mkIget(scale_sensor__oxygenIsAttached)
	 && mkIpropagate(formula_sensor4_last__zero) )
	{
		isError = (0.02 > mkIget(formula_sensor4_last__zero)
		        || 0.2  < mkIget(formula_sensor4_last__zero) );
		// error, if scaled sensor signal is < 0.01 or >0.3 FSR
		if (mkIget(control_error__E1854) != isError) mkIset(control_error__E1854  ,  isError);
	}

//TEST:	printf( " %s:%u: check E1940\n", __FILE__, __LINE__);
	// check E1940 "external option failure"
//	isError = mkIget(scale_Digital1__DigitalIn1_16bit4);
//	if (mkIget(formula_error__E1940) != isError)
//	{
//		mkIset(formula_error__E1940,    isError);
//	}

	// check E1950 "sample missing"
	isError = 0==FLUID1_IS;
	if (mkIget(control_error__E1950) != isError)
	{
		mkIset(control_error__E1950  ,  isError);
	}

	// check E1960 "reagents missing"
	isError = 0==FLUID2_IS;
	if (mkIget(control_error__E1960) != isError)
	{
		mkIset(control_error__E1960  ,  isError);
	}

	// propagate DOOR_OPEN (T�rschalter) for gui
	isError = 0==DOOR_OPEN_IS;
	if (mkIget(control_gui__door_open) != isError)
	{
		mkIset(control_gui__door_open  ,  isError);
	}

	int isDone = 0;

	// save flags for later response to detect raising and falling edges
	isDone = ( (100==mkIget(parameter_subscription__internalStatus)));
	if ( isDone != mkIget(control_debug__read_parameters_done) ) mkIset(control_debug__read_parameters_done,isDone);
	isDone = ( (100==mkIget(pc_subscription__internalStatus)));
	if ( isDone != mkIget(control_debug__check_parameters_done) ) mkIset(control_debug__check_parameters_done,isDone);

	isDone = ( (100==mkIget(parameter_subscription__internalStatus))
			&& (100==mkIget(calpar_subscription__internalStatus))
#ifndef WITHOUT_NODE
			//TODO:note wird bei Selftest gepr�ft && (100==mkIget(node_subscription__internalStatus))
#endif
		);
	if ( isDone != mkIget(control_debug__read_all_parameters_done) ) mkIset(control_debug__read_all_parameters_done,isDone);

	isDone = ( (100==mkIget(parameter_subscription__internalStatus))
#ifndef WITHOUT_NODE
		    && (100==mkIget(node_subscription__internalStatus))
			// @TODO enable after serial sensor fixed: && (100==mkIget(edinburgh_subscription__internalStatus))
			// @TODO enable after serial sensor fixed: && (100==mkIget(aide_subscription__internalStatus))
			// @TODO enable after serial sensor fixed: && (100==mkIget(ziroxZR5_subscription__internalStatus))
#endif
		);
	if ( isDone != mkIget(control_debug__selftest_done) ) mkIset(control_debug__selftest_done,isDone);

	isDone = ( 1
#ifndef WITHOUT_NODE
			&& (0==mkIget(control_error__E1810))	// emergency error
			&& (0==mkIget(control_error__E1820))	// emergency error
#endif
		);
	if ( isDone != mkIget(control_debug__emergency_done) ) mkIset(control_debug__emergency_done,isDone);

	isDone = (1
#ifndef WITHOUT_NODE
			&& (0==mkIget(control_error__E1810))	// furnace emergency
			&& (0==mkIget(control_error__E1815))	// humidity emergency
			&& (0==mkIget(control_error__E1820))	// furnace deviation
			&& (0==mkIget(control_error__E1830))	// cooler deviation
			&& (0==mkIget(control_error__E1872))
			&& (0==mkIget(gui_subscription__ERROR_CRITICAL))
			&& ((0==mkIget(control_error__E1835) 	// carrier gas flow is low
			    || mkIget(parameter___measure_while_airflowerror)))
			// @TODO enable after serial sensor fixed: && (0==mkIget(aide___warmingUp)) 		// aide sensor needs a warming up phase
			// @TODO enable after serial sensor fixed: && (90!=mkIget(aide_subscription__internalStatus))
#endif
		);
	if ( isDone != mkIget(control_debug__warming_up_done) ) mkIset(control_debug__warming_up_done,isDone);


	// state handling
	if ( mkIpropagate(controlControl_subscription__control) )
	{
		control = mkIget(controlControl_subscription__control);
	}

	debugMode = mkIget(controlControl_subscription__debugMode);
// TEST: debugMode = 1;
	if ( mkIpropagate(controlControl_subscription__debugMode) )
	{
		mkIset( control_parameter__debugMode, debugMode );
		mkIset( control_node__debugMode, debugMode );
		mkIset( control_calibration__debugMode, debugMode );
		mkIset( control_formula__debugMode, debugMode );
	}

	// init timer for memory selftest
	if ( mkIisNull(control_subscription__timerDfMem) )
	{
		mktStartTimerDf(MKiP_control_subscription__timerDfMem);
		mktTransmitItem(MKiP_control_subscription__timerDfMem,MKT_ITEM_QUALITY_GOOD,1);
	}
	if(mkIget(control_subscription__errorCame)!=0)	mkIset(control_subscription__errorCame,0);
	// the state machine does the real work
	if(1&debugMode) printf("state_machine_step control=%d\n", control);
	error=state_machine_step( control );

	// state logging
	if ( mkIpropagate(control_subscription__internalStatus) )
	{
		check_next_measurement_time ( );
	}

	// @TODO: recalculate (redundant) parameter items on changes - even for startup values
	//mkIset(control_parameter__sensorChannel, "TOC TC TIC COD_ib TOCb TCb TICb COD_ib COD_o TNb");
	//mkIset(control_parameter__sensorChannel_cal,"TOC TC TIC TOCb TCb TICb CODo TNb");
	//mkIset(control_parameter___streamCount,2);		// @TODO: license check
	// stays constant: item(__COUNTER__,control,parameter,,solutionCount_cal,,uint32,1,"",1,10,"","Count of different solutions, that can be used for calibration"),
	// stays constant: item(__COUNTER__,control,parameter,,signalCount_cal,,uint32,1,"",1,5,"","Count of measurements replicates averaged for one calibration value - use 1 to disable"),
	// @TODO: control & startup check for aide, edinburgh, ziroxZR5
	//item(__COUNTER__,control,aide,,control,,uint32,0,"---",0,UINT32_MAX,"//possibleValues:0 - stop, 25 - selftest, 100 - operational (internal)//","Controls the component."),

	// note: analog output is done by the scale subscription


	// propagate digital output
//TODO:gui_controlDigital1 ersetzt mit gui_scaleDigital1 (sascha )

#ifdef MKT_NODE_CONTROL_DIGITAL_1
	if ( mkIpropagate( gui_scaleDigital1__DigitalOut1_16bit0) ) digitalOut(0, mkIget( gui_scaleDigital1__DigitalOut1_16bit0) );
	if ( mkIpropagate( gui_scaleDigital1__DigitalOut1_16bit1) ) digitalOut(1, mkIget( gui_scaleDigital1__DigitalOut1_16bit1) );
	if ( mkIpropagate( gui_scaleDigital1__DigitalOut1_16bit2) ) digitalOut(2, mkIget( gui_scaleDigital1__DigitalOut1_16bit2) );
	if ( mkIpropagate( gui_scaleDigital1__DigitalOut1_16bit3) ) digitalOut(3, mkIget( gui_scaleDigital1__DigitalOut1_16bit3) );
	if ( mkIpropagate( gui_scaleDigital1__DigitalOut1_16bit4) ) digitalOut(4, mkIget( gui_scaleDigital1__DigitalOut1_16bit4) );
	if ( mkIpropagate( gui_scaleDigital1__DigitalOut1_16bit5) ) digitalOut(5, mkIget( gui_scaleDigital1__DigitalOut1_16bit5) );
	if ( mkIpropagate( gui_scaleDigital1__DigitalOut1_16bit6) ) digitalOut(6, mkIget( gui_scaleDigital1__DigitalOut1_16bit6) );
	if ( mkIpropagate( gui_scaleDigital1__DigitalOut1_16bit7) ) digitalOut(7, mkIget( gui_scaleDigital1__DigitalOut1_16bit7) );

	if ( mkIpropagate( boolexpr___relais1 ) ) digitalOut(8, mkIget( boolexpr___relais1 ) );
	if ( mkIpropagate( boolexpr___relais2 ) ) digitalOut(9, mkIget( boolexpr___relais2 ) );
	if ( mkIpropagate( boolexpr___relais3 ) ) digitalOut(10, mkIget( boolexpr___relais3 ) );
	if ( mkIpropagate( boolexpr___relais4 ) ) digitalOut(11, mkIget( boolexpr___relais4 ) );


	if ( mkIpropagate( gui_scaleDigital1__DigitalOut1_16bit8) )  digitalOut(8, mkIget( gui_scaleDigital1__DigitalOut1_16bit8) );
	if ( mkIpropagate( gui_scaleDigital1__DigitalOut1_16bit9) )  digitalOut(9, mkIget( gui_scaleDigital1__DigitalOut1_16bit9) );
	if ( mkIpropagate( gui_scaleDigital1__DigitalOut1_16bit10) ) digitalOut(10, mkIget( gui_scaleDigital1__DigitalOut1_16bit10) );
	if ( mkIpropagate( gui_scaleDigital1__DigitalOut1_16bit11) ) digitalOut(11, mkIget( gui_scaleDigital1__DigitalOut1_16bit11) );
	if ( mkIpropagate( gui_scaleDigital1__DigitalOut1_16bit12) ) digitalOut(12, mkIget( gui_scaleDigital1__DigitalOut1_16bit12) );
	if ( mkIpropagate( gui_scaleDigital1__DigitalOut1_16bit13) ) digitalOut(13, mkIget( gui_scaleDigital1__DigitalOut1_16bit13) );
	if ( mkIpropagate( gui_scaleDigital1__DigitalOut1_16bit14) ) digitalOut(14, mkIget( gui_scaleDigital1__DigitalOut1_16bit14) );
	if ( mkIpropagate( gui_scaleDigital1__DigitalOut1_16bit15) ) digitalOut(15, mkIget( gui_scaleDigital1__DigitalOut1_16bit15) );

	if (mkIpropagate(gui_nodeDigital1__DigitalOut1_16bit))
	{
		digitalOutShadow = (0xFFFF0000 & digitalOutShadow) | mkIget(gui_nodeDigital1__DigitalOut1_16bit);
	}
	if ( (0x0000FFFF & digitalOutShadow) != mkIget(control_Digital1__DigitalOut1_16bit) )
	{
		mkIset(control_Digital1__DigitalOut1_16bit, 0x0000FFFF & digitalOutShadow);
	}

#endif
	//TODO: gui_scaleDigital2
#ifdef MKT_NODE_CONTROL_DIGITAL_2

    if ( mkIpropagate( gui_scaleDigital2__DigitalOut1_16bit0) ) digitalOut(16, mkIget( gui_scaleDigital2__DigitalOut1_16bit0) );
	if ( mkIpropagate( gui_scaleDigital2__DigitalOut1_16bit1) ) digitalOut(17, mkIget( gui_scaleDigital2__DigitalOut1_16bit1) );
	if ( mkIpropagate( gui_scaleDigital2__DigitalOut1_16bit2) ) digitalOut(18, mkIget( gui_scaleDigital2__DigitalOut1_16bit2) );
	if ( mkIpropagate( gui_scaleDigital2__DigitalOut1_16bit3) ) digitalOut(19, mkIget( gui_scaleDigital2__DigitalOut1_16bit3) );
	if ( mkIpropagate( gui_scaleDigital2__DigitalOut1_16bit4) ) digitalOut(20, mkIget( gui_scaleDigital2__DigitalOut1_16bit4) );
	if ( mkIpropagate( gui_scaleDigital2__DigitalOut1_16bit5) ) digitalOut(21, mkIget( gui_scaleDigital2__DigitalOut1_16bit5) );
	if ( mkIpropagate( gui_scaleDigital2__DigitalOut1_16bit6) ) digitalOut(22, mkIget( gui_scaleDigital2__DigitalOut1_16bit6) );
	if ( mkIpropagate( gui_scaleDigital2__DigitalOut1_16bit7) ) digitalOut(23, mkIget( gui_scaleDigital2__DigitalOut1_16bit7) );

	if ( mkIpropagate( gui_scaleDigital2__DigitalOut1_16bit8) ) digitalOut(24, mkIget( gui_scaleDigital2__DigitalOut1_16bit8) );
	if ( mkIpropagate( gui_scaleDigital2__DigitalOut1_16bit9) ) digitalOut(25, mkIget( gui_scaleDigital2__DigitalOut1_16bit9) );
	if ( mkIpropagate( gui_scaleDigital2__DigitalOut1_16bit10) ) digitalOut(26, mkIget( gui_scaleDigital2__DigitalOut1_16bit10) );
	if ( mkIpropagate( gui_scaleDigital2__DigitalOut1_16bit11) ) digitalOut(27, mkIget( gui_scaleDigital2__DigitalOut1_16bit11) );

	if ( mkIpropagate( boolexpr___relais5 ) ) digitalOut(28, mkIget( boolexpr___relais5 ) );
	if ( mkIpropagate( boolexpr___relais6 ) ) digitalOut(29, mkIget( boolexpr___relais6 ) );
	if ( mkIpropagate( boolexpr___relais7 ) ) digitalOut(30, mkIget( boolexpr___relais7 ) );
	if ( mkIpropagate( boolexpr___relais8 ) ) digitalOut(31, mkIget( boolexpr___relais8 ) );


	if ( mkIpropagate( gui_scaleDigital2__DigitalOut1_16bit12) ) digitalOut(28, mkIget( gui_scaleDigital2__DigitalOut1_16bit12) );
	if ( mkIpropagate( gui_scaleDigital2__DigitalOut1_16bit13) ) digitalOut(29, mkIget( gui_scaleDigital2__DigitalOut1_16bit13) );
	if ( mkIpropagate( gui_scaleDigital2__DigitalOut1_16bit14) ) digitalOut(30, mkIget( gui_scaleDigital2__DigitalOut1_16bit14) );
	if ( mkIpropagate( gui_scaleDigital2__DigitalOut1_16bit15) ) digitalOut(31, mkIget( gui_scaleDigital2__DigitalOut1_16bit15) );


	if (mkIpropagate(gui_nodeDigital2__DigitalOut1_16bit))
		{
			digitalOutShadow = (0x0000FFFF & digitalOutShadow) | (mkIget(gui_nodeDigital2__DigitalOut1_16bit)<<16);
		}
	if ( (digitalOutShadow>>16) != mkIget(control_Digital2__DigitalOut1_16bit))
		{
		    mkIset(control_Digital2__DigitalOut1_16bit, (digitalOutShadow>>16));
		}
#endif

#ifdef MKT_NODE_CONTROL_DIGITAL_3

	   if (  mkIpropagate( gui_scaleDigital3__DigitalOut1_16bit0) ) digital3Out(0, mkIget( gui_scaleDigital3__DigitalOut1_16bit0) );
		if ( mkIpropagate( gui_scaleDigital3__DigitalOut1_16bit1) ) digital3Out(1, mkIget( gui_scaleDigital3__DigitalOut1_16bit1) );
		if ( mkIpropagate( gui_scaleDigital3__DigitalOut1_16bit2) ) digital3Out(2, mkIget( gui_scaleDigital3__DigitalOut1_16bit2) );
		if ( mkIpropagate( gui_scaleDigital3__DigitalOut1_16bit3) ) digital3Out(3, mkIget( gui_scaleDigital3__DigitalOut1_16bit3) );
		if ( mkIpropagate( gui_scaleDigital3__DigitalOut1_16bit4) ) digital3Out(4, mkIget( gui_scaleDigital3__DigitalOut1_16bit4) );
		if ( mkIpropagate( gui_scaleDigital3__DigitalOut1_16bit5) ) digital3Out(5, mkIget( gui_scaleDigital3__DigitalOut1_16bit5) );
		if ( mkIpropagate( gui_scaleDigital3__DigitalOut1_16bit6) ) digital3Out(6, mkIget( gui_scaleDigital3__DigitalOut1_16bit6) );
		if ( mkIpropagate( gui_scaleDigital3__DigitalOut1_16bit7) ) digital3Out(7, mkIget( gui_scaleDigital3__DigitalOut1_16bit7) );

		if ( mkIpropagate( gui_scaleDigital3__DigitalOut1_16bit8) ) digital3Out(8, mkIget( gui_scaleDigital3__DigitalOut1_16bit8) );
		if ( mkIpropagate( gui_scaleDigital3__DigitalOut1_16bit9) ) digital3Out(9, mkIget( gui_scaleDigital3__DigitalOut1_16bit9) );
		if ( mkIpropagate( gui_scaleDigital3__DigitalOut1_16bit10) ) digital3Out(10, mkIget( gui_scaleDigital3__DigitalOut1_16bit10) );
		if ( mkIpropagate( gui_scaleDigital3__DigitalOut1_16bit11) ) digital3Out(11, mkIget( gui_scaleDigital3__DigitalOut1_16bit11) );

		if ( mkIpropagate( gui_scaleDigital3__DigitalOut1_16bit12) ) digital3Out(12, mkIget( gui_scaleDigital3__DigitalOut1_16bit12) );
		if ( mkIpropagate( gui_scaleDigital3__DigitalOut1_16bit13) ) digital3Out(13, mkIget( gui_scaleDigital3__DigitalOut1_16bit13) );
		if ( mkIpropagate( gui_scaleDigital3__DigitalOut1_16bit14) ) digital3Out(14, mkIget( gui_scaleDigital3__DigitalOut1_16bit14) );
		if ( mkIpropagate( gui_scaleDigital3__DigitalOut1_16bit15) ) digital3Out(15, mkIget( gui_scaleDigital3__DigitalOut1_16bit15) );


		if (mkIpropagate(gui_nodeDigital3__DigitalOut1_16bit))
		{
			digital3OutShadow = (0xFFFF0000 & digital3OutShadow) | mkIget(gui_nodeDigital3__DigitalOut1_16bit);
		}
		if ( (0x0000FFFF & digital3OutShadow) != mkIget(control_Digital3__DigitalOut1_16bit) )
		{
			mkIset(control_Digital3__DigitalOut1_16bit, 0x0000FFFF & digital3OutShadow);
		}

#endif
	//TODO: gui_controlDigital1  - gui_nodeDigital1 (sascha)


//TODO : gui_control durch gui_node

	#ifdef MKT_NODE_CONTROL_MOTOR_1
	if (mkIpropagate(gui_nodeMotor1__StirrerMode))		mkIset(control_Motor1__StirrerMode, mkIget(gui_nodeMotor1__StirrerMode));
	if (mkIpropagate(gui_nodeMotor1__CommandStatus)) 	mkIset(control_Motor1__CommandStatus, mkIget(gui_nodeMotor1__CommandStatus));
	if (mkIpropagate(gui_nodeMotor1__EndschalterInvertiren)) mkIset(control_Motor1__EndschalterInvertiren, mkIget(gui_nodeMotor1__EndschalterInvertiren));
	if (mkIpropagate(gui_nodeMotor1__CommandGoToPos)) 	mkIset(control_Motor1__CommandGoToPos, mkIget(gui_nodeMotor1__CommandGoToPos));
	if (mkIpropagate(gui_nodeMotor1__MaxPosition)) 		mkIset(control_Motor1__MaxPosition, mkIget(gui_nodeMotor1__MaxPosition));
	if (mkIpropagate(gui_nodeMotor1__Stepperparameter)) {if(!IS_DOPPELMOTOR_VERSION_3(1)) mkIset(control_Motor1__Stepperparameter, mkIget(gui_nodeMotor1__Stepperparameter)); else mkIset(control_Motor1__StepperparameterByte, mkIget(gui_nodeMotor1__Stepperparameter));}
	if (mkIpropagate(gui_nodeMotor1__StirrerOn)) 		mkIset(control_Motor1__StirrerOn, mkIget(gui_nodeMotor1__StirrerOn));
	if (mkIpropagate(gui_nodeMotor1__StirrerDelay)) 		mkIset(control_Motor1__StirrerDelay, mkIget(gui_nodeMotor1__StirrerDelay));
	if (mkIpropagate(gui_nodeMotor1__StirrerCurrent)) 	mkIset(control_Motor1__StirrerCurrent, mkIget(gui_nodeMotor1__StirrerCurrent));
	if (mkIpropagate(gui_nodeMotor1__Pump1)) 			mkIset(control_Motor1__Pump1, mkIget(gui_nodeMotor1__Pump1));
	if (mkIpropagate(gui_nodeMotor1__Pump1Mode)) 		mkIset(control_Motor1__Pump1Mode, mkIget(gui_nodeMotor1__Pump1Mode));
	if (mkIpropagate(gui_nodeMotor1__Pump1Left)) 		mkIset(control_Motor1__Pump1Left, mkIget(gui_nodeMotor1__Pump1Left));
	if (mkIpropagate(gui_nodeMotor1__Pump1Speed)) 		mkIset(control_Motor1__Pump1Speed, mkIget(gui_nodeMotor1__Pump1Speed));
	if (mkIpropagate(gui_nodeMotor1__Pump1IntervalPulse)) mkIset(control_Motor1__Pump1IntervalPulse, mkIget(gui_nodeMotor1__Pump1IntervalPulse));
	if (mkIpropagate(gui_nodeMotor1__Pump1IntervalTime))	mkIset(control_Motor1__Pump1IntervalTime, mkIget(gui_nodeMotor1__Pump1IntervalTime));
	if (mkIpropagate(gui_nodeMotor1__ConstantKp)) 		mkIset(control_Motor1__ConstantKp, mkIget(gui_nodeMotor1__ConstantKp));
	if (mkIpropagate(gui_nodeMotor1__ConstantKd)) 		mkIset(control_Motor1__ConstantKd, mkIget(gui_nodeMotor1__ConstantKd));
	if (mkIpropagate(gui_nodeMotor1__ConstantKi)) 		mkIset(control_Motor1__ConstantKi, mkIget(gui_nodeMotor1__ConstantKi));
#endif

#ifdef MKT_NODE_CONTROL_MOTOR_2
	// propagate gui control to motor2
	if (mkIpropagate(gui_nodeMotor2__StirrerMode))		mkIset(control_Motor2__StirrerMode, mkIget(gui_nodeMotor2__StirrerMode));
	if (mkIpropagate(gui_nodeMotor2__CommandStatus)) 	mkIset(control_Motor2__CommandStatus, mkIget(gui_nodeMotor2__CommandStatus));
	if (mkIpropagate(gui_nodeMotor2__EndschalterInvertiren)) mkIset(control_Motor2__EndschalterInvertiren, mkIget(gui_nodeMotor2__EndschalterInvertiren));
	if (mkIpropagate(gui_nodeMotor2__CommandGoToPos)) 	mkIset(control_Motor2__CommandGoToPos, mkIget(gui_nodeMotor2__CommandGoToPos));
	if (mkIpropagate(gui_nodeMotor2__MaxPosition)) 		mkIset(control_Motor2__MaxPosition, mkIget(gui_nodeMotor2__MaxPosition));
	if (mkIpropagate(gui_nodeMotor2__Stepperparameter)) {if(!IS_DOPPELMOTOR_VERSION_3(2)) mkIset(control_Motor2__Stepperparameter, mkIget(gui_nodeMotor2__Stepperparameter)); else mkIset(control_Motor2__StepperparameterByte, mkIget(gui_nodeMotor2__Stepperparameter));}
	if (mkIpropagate(gui_nodeMotor2__StirrerOn)) 		mkIset(control_Motor2__StirrerOn, mkIget(gui_nodeMotor2__StirrerOn));
	if (mkIpropagate(gui_nodeMotor2__StirrerDelay)) 	mkIset(control_Motor2__StirrerDelay, mkIget(gui_nodeMotor2__StirrerDelay));
	if (mkIpropagate(gui_nodeMotor2__StirrerCurrent)) 	mkIset(control_Motor2__StirrerCurrent, mkIget(gui_nodeMotor2__StirrerCurrent));
	if (mkIpropagate(gui_nodeMotor2__Pump1)) 			mkIset(control_Motor2__Pump1, mkIget(gui_nodeMotor2__Pump1));
	if (mkIpropagate(gui_nodeMotor2__Pump1Mode)) 		mkIset(control_Motor2__Pump1Mode, mkIget(gui_nodeMotor2__Pump1Mode));
	if (mkIpropagate(gui_nodeMotor2__Pump1Left)) 		mkIset(control_Motor2__Pump1Left, mkIget(gui_nodeMotor2__Pump1Left));
	if (mkIpropagate(gui_nodeMotor2__Pump1Speed)) 		mkIset(control_Motor2__Pump1Speed, mkIget(gui_nodeMotor2__Pump1Speed));
	if (mkIpropagate(gui_nodeMotor2__Pump1IntervalPulse)) mkIset(control_Motor2__Pump1IntervalPulse, mkIget(gui_nodeMotor2__Pump1IntervalPulse));
	if (mkIpropagate(gui_nodeMotor2__Pump1IntervalTime))mkIset(control_Motor2__Pump1IntervalTime, mkIget(gui_nodeMotor2__Pump1IntervalTime));
	if (mkIpropagate(gui_nodeMotor2__ConstantKp)) 		mkIset(control_Motor2__ConstantKp, mkIget(gui_nodeMotor2__ConstantKp));
	if (mkIpropagate(gui_nodeMotor2__ConstantKd)) 		mkIset(control_Motor2__ConstantKd, mkIget(gui_nodeMotor2__ConstantKd));
	if (mkIpropagate(gui_nodeMotor2__ConstantKi)) 		mkIset(control_Motor2__ConstantKi, mkIget(gui_nodeMotor2__ConstantKi));
#endif

#ifdef MKT_NODE_CONTROL_MOTOR_3
	// propagate gui control to motor3
	if (mkIpropagate(gui_nodeMotor3__StirrerMode))		mkIset(control_Motor3__StirrerMode, mkIget(gui_nodeMotor3__StirrerMode));
	if (mkIpropagate(gui_nodeMotor3__CommandStatus)) 	mkIset(control_Motor3__CommandStatus, mkIget(gui_nodeMotor3__CommandStatus));
	if (mkIpropagate(gui_nodeMotor3__EndschalterInvertiren)) mkIset(control_Motor3__EndschalterInvertiren, mkIget(gui_nodeMotor3__EndschalterInvertiren));
	if (mkIpropagate(gui_nodeMotor3__CommandGoToPos)) 	mkIset(control_Motor3__CommandGoToPos, mkIget(gui_nodeMotor3__CommandGoToPos));
	if (mkIpropagate(gui_nodeMotor3__MaxPosition)) 		mkIset(control_Motor3__MaxPosition, mkIget(gui_nodeMotor3__MaxPosition));
	if (mkIpropagate(gui_nodeMotor3__Stepperparameter)) {if(!IS_DOPPELMOTOR_VERSION_3(2)) mkIset(control_Motor3__Stepperparameter, mkIget(gui_nodeMotor3__Stepperparameter)); else mkIset(control_Motor3__StepperparameterByte, mkIget(gui_nodeMotor3__Stepperparameter));}
	if (mkIpropagate(gui_nodeMotor3__StirrerOn)) 		mkIset(control_Motor3__StirrerOn, mkIget(gui_nodeMotor3__StirrerOn));
	if (mkIpropagate(gui_nodeMotor3__StirrerDelay)) 	mkIset(control_Motor3__StirrerDelay, mkIget(gui_nodeMotor3__StirrerDelay));
	if (mkIpropagate(gui_nodeMotor3__StirrerCurrent)) 	mkIset(control_Motor3__StirrerCurrent, mkIget(gui_nodeMotor3__StirrerCurrent));
	if (mkIpropagate(gui_nodeMotor3__Pump1)) 			mkIset(control_Motor3__Pump1, mkIget(gui_nodeMotor3__Pump1));
	if (mkIpropagate(gui_nodeMotor3__Pump1Mode)) 		mkIset(control_Motor3__Pump1Mode, mkIget(gui_nodeMotor3__Pump1Mode));
	if (mkIpropagate(gui_nodeMotor3__Pump1Left)) 		mkIset(control_Motor3__Pump1Left, mkIget(gui_nodeMotor3__Pump1Left));
	if (mkIpropagate(gui_nodeMotor3__Pump1Speed)) 		mkIset(control_Motor3__Pump1Speed, mkIget(gui_nodeMotor3__Pump1Speed));
	if (mkIpropagate(gui_nodeMotor3__Pump1IntervalPulse)) mkIset(control_Motor3__Pump1IntervalPulse, mkIget(gui_nodeMotor3__Pump1IntervalPulse));
	if (mkIpropagate(gui_nodeMotor3__Pump1IntervalTime))mkIset(control_Motor3__Pump1IntervalTime, mkIget(gui_nodeMotor3__Pump1IntervalTime));
	if (mkIpropagate(gui_nodeMotor3__ConstantKp)) 		mkIset(control_Motor3__ConstantKp, mkIget(gui_nodeMotor3__ConstantKp));
	if (mkIpropagate(gui_nodeMotor3__ConstantKd)) 		mkIset(control_Motor3__ConstantKd, mkIget(gui_nodeMotor3__ConstantKd));
	if (mkIpropagate(gui_nodeMotor3__ConstantKi)) 		mkIset(control_Motor3__ConstantKi, mkIget(gui_nodeMotor3__ConstantKi));
#endif

#ifdef MKT_NODE_CONTROL_MOTOR_4
	// propagate gui control to motor4
	if (mkIpropagate(gui_nodeMotor4__StirrerMode))		mkIset(control_Motor4__StirrerMode, mkIget(gui_nodeMotor4__StirrerMode));
	if (mkIpropagate(gui_nodeMotor4__CommandStatus)) 	mkIset(control_Motor4__CommandStatus, mkIget(gui_nodeMotor4__CommandStatus));
	if (mkIpropagate(gui_nodeMotor4__EndschalterInvertiren)) mkIset(control_Motor4__EndschalterInvertiren, mkIget(gui_nodeMotor4__EndschalterInvertiren));
	if (mkIpropagate(gui_nodeMotor4__CommandGoToPos)) 	mkIset(control_Motor4__CommandGoToPos, mkIget(gui_nodeMotor4__CommandGoToPos));
	if (mkIpropagate(gui_nodeMotor4__MaxPosition)) 		mkIset(control_Motor4__MaxPosition, mkIget(gui_nodeMotor4__MaxPosition));
	if (mkIpropagate(gui_nodeMotor4__Stepperparameter)) {if(!IS_DOPPELMOTOR_VERSION_3(1)) mkIset(control_Motor4__Stepperparameter, mkIget(gui_nodeMotor4__Stepperparameter)); else mkIset(control_Motor4__StepperparameterByte, mkIget(gui_nodeMotor4__Stepperparameter));}
	if (mkIpropagate(gui_nodeMotor4__StirrerOn)) 		mkIset(control_Motor4__StirrerOn, mkIget(gui_nodeMotor4__StirrerOn));
	if (mkIpropagate(gui_nodeMotor4__StirrerDelay)) 	mkIset(control_Motor4__StirrerDelay, mkIget(gui_nodeMotor4__StirrerDelay));
	if (mkIpropagate(gui_nodeMotor4__StirrerCurrent)) 	mkIset(control_Motor4__StirrerCurrent, mkIget(gui_nodeMotor4__StirrerCurrent));
	if (mkIpropagate(gui_nodeMotor4__Pump1)) 			mkIset(control_Motor4__Pump1, mkIget(gui_nodeMotor4__Pump1));
	if (mkIpropagate(gui_nodeMotor4__Pump1Mode)) 		mkIset(control_Motor4__Pump1Mode, mkIget(gui_nodeMotor4__Pump1Mode));
	if (mkIpropagate(gui_nodeMotor4__Pump1Left)) 		mkIset(control_Motor4__Pump1Left, mkIget(gui_nodeMotor4__Pump1Left));
	if (mkIpropagate(gui_nodeMotor4__Pump1Speed)) 		mkIset(control_Motor4__Pump1Speed, mkIget(gui_nodeMotor4__Pump1Speed));
	if (mkIpropagate(gui_nodeMotor4__Pump1IntervalPulse)) mkIset(control_Motor4__Pump1IntervalPulse, mkIget(gui_nodeMotor4__Pump1IntervalPulse));
	if (mkIpropagate(gui_nodeMotor4__Pump1IntervalTime))mkIset(control_Motor4__Pump1IntervalTime, mkIget(gui_nodeMotor4__Pump1IntervalTime));
	if (mkIpropagate(gui_nodeMotor4__ConstantKp)) 		mkIset(control_Motor4__ConstantKp, mkIget(gui_nodeMotor4__ConstantKp));
	if (mkIpropagate(gui_nodeMotor4__ConstantKd)) 		mkIset(control_Motor4__ConstantKd, mkIget(gui_nodeMotor4__ConstantKd));
	if (mkIpropagate(gui_nodeMotor4__ConstantKi)) 		mkIset(control_Motor4__ConstantKi, mkIget(gui_nodeMotor4__ConstantKi));
#endif


	return( 0 );
#undef mkIpropagate

//@TODO test: nodeRetry();

}

static char* whatident="@(#)control" QUOTEME(VER_MAJOR) "." QUOTEME(VER_MINOR) " build " QUOTEME(VER_BUILDNUM) " device control for the LAR QuickTOCxy - $Id: $ $URL: $";

void printCompleteVersionInformation(void)
{
	printf ("\nPrinting complete version information for\n");

	printf ("control "
		QUOTEME(VER_MAJOR) "." QUOTEME(VER_MINOR) " build " QUOTEME(VER_BUILDNUM)
		" device control for the LAR QuickTOCxy\n");

	printf ("   whatident: %s\n", whatident);
	printf ("   Subversion Id=$Id$\n");
	printf ("   Subversion URL=$URL$\n");

	printf ("   VER_COMPILE_DATE=%s\n",VER_COMPILE_DATE);
	printf ("   VER_COMPILE_TIME=%s\n",VER_COMPILE_TIME);
	printf ("   VER_COMPILE_BY=%s\n",VER_COMPILE_BY);
	printf ("   VER_COMPILE_HOST=%s\n",VER_COMPILE_HOST);
	printf ("   VER_COMPILER=%s\n",VER_COMPILER);
	printf ("   VER_BUILD_TYPE=%s\n",VER_BUILD_TYPE);
	printf ("   VER_CFLAGS=%s\n",VER_CFLAGS);
	printf ("   VER_MAJOR=%u\n",VER_MAJOR);
	printf ("   VER_MINOR=%u\n",VER_MINOR);
	//printf ("   VER_BUILDNUM=%u\n",VER_BUILDNUM);
	printf ("   VER_FULLSTR=%s\n",VER_FULLSTR);

	printf ("\nEnd of version inforamtion\n");
}

int main()
{
	// The integrated market component needs no main() fundction at all.
	// So you can use the main() function for your own test purposes.

	printf ("control" QUOTEME(VER_MAJOR) "." QUOTEME(VER_MINOR) " build " QUOTEME(VER_BUILDNUM) " device control for the LAR QuickTOCxy\n");
	printf("A LAR market subscription written by C.Vogt" __DATE__ " " __TIME__);
	//subscription0071 = mktSmallMarket("/lar/ultimate/loop/sub/larcontrol.so");
	//item0071 = subscription0071->item;

	return 1;
//  printCompleteVersionInformation();
/*
	mkIset(node_Motor3__PositionOld,0x10000-3);	printf("Position %u ok: %u\n", mkIget(node_Motor3__PositionOld), 3>abs((short int)mkIget(node_Motor3__PositionOld)));
	mkIset(node_Motor3__PositionOld,0x10000-2);	printf("Position %u ok: %u\n", mkIget(node_Motor3__PositionOld), 3>abs((short int)mkIget(node_Motor3__PositionOld)));
	mkIset(node_Motor3__PositionOld,0x10000-1);	printf("Position %u ok: %u\n", mkIget(node_Motor3__PositionOld), 3>abs((short int)mkIget(node_Motor3__PositionOld)));
	mkIset(node_Motor3__PositionOld,0);	printf("Position %u ok: %u\n", mkIget(node_Motor3__PositionOld), 3>abs((short int)mkIget(node_Motor3__PositionOld)));
	mkIset(node_Motor3__PositionOld,1);	printf("Position %u ok: %u\n", mkIget(node_Motor3__PositionOld), 3>abs((short int)mkIget(node_Motor3__PositionOld)));
	mkIset(node_Motor3__PositionOld,2);	printf("Position %u ok: %u\n", mkIget(node_Motor3__PositionOld), 3>abs((short int)mkIget(node_Motor3__PositionOld)));
	mkIset(node_Motor3__PositionOld,3);	printf("Position %u ok: %u\n", mkIget(node_Motor3__PositionOld), 3>abs((short int)mkIget(node_Motor3__PositionOld)));
*/
	printf("\n\nThis is a test example for a LAR market component\n");
	printf("The main() function is just used for test purposes. It shows, how to\n");
	printf("1 - print items\n");
	printf("2 - call the subscriptionCallback() function to change some items\n");
	printf("3 - print items again\n");

	printf("\n ********* Now executing: 1 - print items\n");
	mktItemPrintAll(item0071);
	printf("\n ********* Now executing: 2 - call the subscriptionCallback() function to change some items\n");
	subscriptionCallback(subscription0071);
	printf("\n ********* Now executing: 3 - print items again\n");
	mktItemPrintAll(item0071);


//#define cpp_test(no) QUOTEME(stateX_ ## no)
//	printf("\ncpp test printing the old 'stateX_1': %s\n",cpp_test(1));
//#define cpp_test(no) QUOTEME($ ## stateX_ ## no)
//	printf("\ncpp test printing '$stateX_2': %s\n",cpp_test(2));
//#define cpp_test(no) QUOTEME(" -- " ## stateX_ ## no)
//	printf("cpp test FAILED\n: %s",cpp_test(3));
//#define cpp_test(no) (" -- " QUOTEME(stateX_ ## no))
//	printf("cpp test printing what we want '-- stateX_4': %s\n ",cpp_test(4));
//#define cpp_test(no) (" -- stateX_" QUOTEME(no))
//	printf("cpp test printing what we want having nicer code '-- stateX_5': %s\n ",cpp_test(5));

	return( 0 ); 	// no error
}
