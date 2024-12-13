// TimerInterval
//.h
/*
  Timer interrupt library for esp8266 using ESP8266TimerInterrupt library
*/
#ifndef TimerInterval_h
#define TimerInterval_h

#if !defined(ESP8266)
#error This code is designed to run on ESP8266 and ESP8266-based boards! Please check your Tools->Board setting.
#endif

// These define's must be placed at the beginning before #include "ESP8266TimerInterrupt.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define TIMER_INTERRUPT_DEBUG 0
#define _TIMERINTERRUPT_LOGLEVEL_ 0

// using div16, seems to be enough for our purposes.
#define USING_TIM_DIV1 false   // for shortest and most accurate timer
#define USING_TIM_DIV16 true   // for medium time and medium accurate timer
#define USING_TIM_DIV256 false // for longest timer but least accurate. Default

#include "Arduino.h"
#include "../../ESP8266TimerInterrupt/ESP8266TimerInterrupt/src/ESP8266TimerInterrupt.h"

void IRAM_ATTR __esp8266_timer_handler__();
static inline void __place_holder_funct__(void);

class TimerIntervalClass
{
private:
  ESP8266Timer _ESP_ITimer;

public:
  /*
    TimerIntervalClass():
      set timer 2 up
      only one instance can be used
  */
  TimerIntervalClass();

  /*
    disableTimer():
      disables timer completely, may cause unpredictable results,
      use as short as possible, re-enable timer using '_enableTimer()' or 'setTimer()'
  */
  inline void disableTimer();
  /*
    enableTimer():
      re-enables interrupt timer
  */
  inline void enableTimer();
  /*
    setTimer():
      set timer interrupt interval in microseconds(us)
      this will reenable timer if it was disabled

      Valid Interval Range: 1-1048576us

      This function is optimized for time.

      inputs: ( (uint32_t)timer interrupt interval in microseconds(us) (1-1048576) (range retained for consistency) )
  */
  inline void setTimer(uint32_t);

  /*
    setCallback():
      set the function/method to be called on each interrupt
      callback can be changed dynamically whenever needed
  */
  inline void setCallback(void (*)(void));

private:
  /*
    since using someone else's lib, we will manually make sure the timer doesn't call the function:
  */
  volatile bool _timerEnable = false;
};
TimerIntervalClass TimerInterval;

//.cpp

/*
    callback pointer:
*/
static void (*__callBack__)(void) = __place_holder_funct__;

TimerIntervalClass::TimerIntervalClass()
{
  // presets:
  disableTimer();
  __callBack__ = __place_holder_funct__;
}

inline void TimerIntervalClass::disableTimer()
{
  // disable all interrupts in Timer 2
  _ESP_ITimer.disableTimer();
}
inline void TimerIntervalClass::enableTimer()
{
  // enable compare match A interrupt:
  _ESP_ITimer.enableTimer();
}
inline void TimerIntervalClass::setTimer(uint32_t interval_us)
{
  if (!interval_us)
    interval_us++;

  if (interval_us > 1048576)
    interval_us = 1048576;
  
  _ESP_ITimer.setInterval(interval_us, __callBack__);
  enableTimer();
}

inline void TimerIntervalClass::setCallback(void (*callBack)(void))
{
  __callBack__ = callBack;
}

// placeholder function to prevent null point error in the event a callback is not set
static inline void __place_holder_funct__(void) {}

void IRAM_ATTR __esp8266_timer_handler__()
{
  __callBack__();
}

#endif
