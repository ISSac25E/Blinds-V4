
// These define's must be placed at the beginning before #include "ESP8266TimerInterrupt.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define TIMER_INTERRUPT_DEBUG 1
#define _TIMERINTERRUPT_LOGLEVEL_ 1

// Select a Timer Clock
#define USING_TIM_DIV1 false  // for shortest and most accurate timer
#define USING_TIM_DIV16 true // for medium time and medium accurate timer
#define USING_TIM_DIV256 false // for longest timer but least accurate. Default
#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Serge\Blinds V4\Blinds_V4_Core\ESP8266TimerInterrupt\tests\timer_limit_test\include.h"

ESP8266Timer ITimer;

void IRAM_ATTR TimerHandler()
{
  static bool started = false;

  if (!started)
  {
    started = true;
    pinMode(LED_BUILTIN, OUTPUT);
  }

  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // Toggle LED Pin
}

void setup()
{
  Serial.begin(115200);
  while(!Serial);
  delay(500);
  Serial.println("\ninit");

  Serial.print(F("\nStarting TimerInterruptTest on "));
  Serial.println(ARDUINO_BOARD);
  Serial.println(ESP8266_TIMER_INTERRUPT_VERSION);
  Serial.print(F("CPU Frequency = "));
  Serial.print(F_CPU / 1000000);
  Serial.println(F(" MHz"));
  // delay(10);
  //   if (ITimer.attachInterruptInterval(5, TimerHandler))
  //   {
  //     Serial.println(F("Starting ITimer OK, micros = 1"));
  //   }
  //   else
  //   {
  //     Serial.println(F("Can't set ITimer correctly, micros = 1"));
  //   }

  for (uint16_t x = 1; x < 1000; x++)
  {
    delay(10);
    if (ITimer.attachInterruptInterval((unsigned long)(x * 1000UL), TimerHandler))
    {
      Serial.print(F("Starting ITimer OK, micros = "));
      Serial.println((unsigned long)x * 1000UL);
    }
    else
    {
      Serial.print(F("Can't set ITimer correctly, micros = "));
      Serial.println((unsigned long)x * 1000UL);
    }
  }
}

void loop()
{
}