// EspButtonInterface REV 1.0.0
//.h
#ifndef ESP_BUTTON_INTERFACE_h
#define ESP_BUTTON_INTERFACE_h

#include "Arduino.h"

class EspPinDriver
{
public:
  EspPinDriver(uint8_t Pin, bool PullMode); // Mode = High, PullUp Enabled Exp. GPIO 16 PullDown. default pullup = true

  // Returns Pin State and runs Button:
  bool run();
  // Returns button State, does not run Button:
  bool buttonState()
  {
    return _buttonState;
  }
  void buttonDebounce(uint16_t Debounce_us)
  {
    _ButtonDeBounceDelay = Debounce_us;
  }

private:
  uint8_t _Pin;

  bool _buttonState = false;
  bool _ButtonTest = false;
  uint32_t _ButtonTestTimer = 0;
  uint16_t _ButtonDeBounceDelay = 5000; // Default Value 5ms, 5000us
  // Function for Pin reading
  bool _pinRead()
  {
    return digitalRead(_Pin);
  }
};

// deprecated:
// class PinMacro
// {
// public:
//   // Returns true if StateChange
//   bool run(bool PinState);
//   bool state()
//   {
//     return _MacroState;
//   }
//   uint32_t prevInterval()
//   {
//     return _MacroPrevInterval;
//   }
//   uint32_t interval()
//   {
//     return millis() - _MacroIntervalTimer;
//   }
//   void timerReset()
//   {
//     _MacroIntervalTimer = millis();
//   }
//   void timerSet(uint32_t TimerSet)
//   {
//     _MacroIntervalTimer = (millis() + TimerSet);
//   }

// private:
//   // Variables for running Macro:
//   bool _init = false;
//   bool _MacroState = false;
//   uint32_t _MacroIntervalTimer = 0;
//   uint32_t _MacroPrevInterval = 0;
// };

//.cpp
// #include "ESP_BUTTON_INTERFACE.h"
// #include "Arduino.h"
EspPinDriver::EspPinDriver(uint8_t Pin, bool PullMode = true)
{
  // Mode High: PullUp, if GPIO 16 PullDown
  _Pin = Pin;
  if (PullMode)
  {
    if (_Pin == 16)
    {
      pinMode(_Pin, INPUT_PULLDOWN_16);
    }
    else
    {
      pinMode(_Pin, INPUT_PULLUP);
    }
  }
  else
  {
    pinMode(_Pin, INPUT);
  }
}

bool EspPinDriver::run()
{
  if (_ButtonTest)
  {
    if (micros() - _ButtonTestTimer >= _ButtonDeBounceDelay)
    {
      if (_pinRead() != _buttonState)
      {
        _buttonState = !_buttonState;
        _ButtonTest = false;
      }
      else
      {
        _ButtonTest = false;
      }
    }
  }
  else
  {
    if (_pinRead() != _buttonState)
    {
      _ButtonTest = true;
      _ButtonTestTimer = micros();
    }
  }
  return _buttonState;
}

// bool PinMacro::run(bool PinState)
// {
//   if (!_init)
//   {
//     _MacroState = PinState;
//     _MacroPrevInterval = millis() - _MacroIntervalTimer;
//     _MacroIntervalTimer = millis();
//     return false;
//   }
//   else if (PinState != _MacroState)
//   {
//     // State Change:
//     _MacroState = PinState;
//     _MacroPrevInterval = millis() - _MacroIntervalTimer;
//     _MacroIntervalTimer = millis();
//     return true;
//   }
//   else
//     return false;
// }

#endif
