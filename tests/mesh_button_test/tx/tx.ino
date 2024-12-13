#include "Arduino.h"
// #include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Serge\Blinds V4\tests\mesh_button_test\tx\include.h"
// #include "C:/Users/AVG/Documents/Electrical Main (Amir)/Arduino/Projects/Serge/Blinds V4/tests/esp_tuya_test/include.h"  // << this doesnt work for esp :(
#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Serge\Blinds V4\tests\mesh_button_test\tx\include.h"


// // standard tuya lib used for atmel:
// #include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Serge\Blinds V4\Blinds_V4_Core\Tuya WiFi MCU Serial SDK\Tuya_WiFi_MCU_SDK_ESP8266_2.1.0\Tuya_WiFi_MCU_SDK\src\TuyaWifi.h"

// // led macro and sequences:
// #include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Serge\Blinds V4\Blinds_V4_Core\SequenceBuild\SequenceBuild_1.0.4.h"
// #include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Serge\Blinds V4\Blinds_V4_Core\LedMacro\LedMacro_1.1.0b.h"

// // Button Interface:
// #include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Serge\Blinds V4\Blinds_V4_Core\ESP_BUTTON_INTERFACE\EspButtonInterface_v1.1.0.h"
// #include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Serge\Blinds V4\Blinds_V4_Core\InputMacro\InputMacro_1.0.1.h"

// for mesh network:
#include <ESP8266WiFi.h>
#include <espnow.h>

const uint8_t slaveList[][6] = {{0x42, 0x4C, 0x4E, 0x44, 0xAB, 0x01},
                                {0x42, 0x4C, 0x4E, 0x44, 0xAB, 0x02},
                                {0x42, 0x4C, 0x4E, 0x44, 0xAB, 0x03}};

#define TUYA_PID "zquaihibynlw2hxq" // << This can be found in your TUYA account
#define TUYA_MCU_Version "1.0.0"    // << This version does not really matter. You can put anything you like. It can be a useful tool to keep track of revisions

#define TUYA_WiFi_ModeSetPin 3 // << configure pin number for the button you'll use to Pair your TUYA Device

#define TUYA_ModuleResetPin 5 // << configure pin number for ESP8266 to reset the TUYA module in the case of mis-communication or module error. GPIO5 = D1

#define StatusLedPin LED_BUILTIN // << pin Number of status LED. Make sure pin is a 'PWM' and NOT FROM TIMER 2 (5, 6, 9, or 10. NOT 3 or 11)

#define DPID_UpdateInterval 1000 // << set update DPID's interval in milliseconds

#define DPID_Button 101

/////////////////////////////////
////////// LED Setup: //////////
/////////////////////////////////
// setup sequence builder for LED:
SequenceBuild ledBuild;

// setup macro for led (total 1):
LedMacro _macro[1];
LedMacroManager macro(_macro, 1);

// PWM led value:
uint8_t ledVal = 0;

EspPinDriver pinInput(0); // << esp built in flash-button
InputMacro pinMacro(true);

bool dpidButtonValue = false;

//////////////////////////////////
////////// TUYA Setup: //////////
//////////////////////////////////
// config Tuya Module communication with Hardware Serial:
TuyaWifi tuya_module;

// setup DPID array to pass onto 'TuyaWifi':
unsigned char dpid_array[3][2] = {{DPID_Button, DP_TYPE_BOOL}};

uint32_t updateAll_Timer = 0; // timer to handle periodic updating of DPID's

void setup()
{
  // initialize hardware Serial to 9600 for communication with TUYA Module:
  Serial.begin(9600);

  // setup pin module reset pin and output led:
  {
    pinMode(TUYA_ModuleResetPin, INPUT);
    digitalWrite(TUYA_ModuleResetPin, HIGH);

    pinMode(StatusLedPin, OUTPUT);
    analogWrite(StatusLedPin, ledVal);
  }

  // set up TUYA Module:
  {
    unsigned char pid[] = {TUYA_PID};
    unsigned char mcu_ver[] = {TUYA_MCU_Version};

    // init TUYA PID and MCU Version:
    tuya_module.init(pid, mcu_ver);

    // Set all TUYA DPID's:
    tuya_module.set_dp_cmd_total(dpid_array, 3);

    // set callbacks:
    tuya_module.dp_process_func_register(dp_process);       // function to proccess incomming commands
    tuya_module.dp_update_all_func_register(dp_update_all); // function to update all DPID states at once
  }

  // setup esp now. if there is an error, not much we can do:
  {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    esp_now_init();
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
  }
}

void loop()
{
  // run tuya uart service:
  tuya_module.uart_service();

  // run all handles:
  outputHandle();
  buttonHandle();
  moduleErrorHandle();
  espnow_handle();

  // check update all timer:
  if (millis() - updateAll_Timer >= DPID_UpdateInterval)
    dp_update_all();
}

// Callback when data is received
void OnDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len)
{
  dpidButtonValue = incomingData[0];
  espnow_update_all();
}

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus)
{
}

void espnow_handle()
{
  static uint32_t tim = millis();

  if (millis() - tim >= 3000)
  {
    tim = millis();
    // update all:
    espnow_update_all();
  }
}

void espnow_update_all()
{
  for (uint8_t x = 0; x < sizeof(slaveList) / sizeof(slaveList[0]); x++)
    esp_now_send((uint8_t *)slaveList[x], (uint8_t *)&dpidButtonValue, 1);
}

/*
  outputHandle():
    handles LED sequenses and servo sequences
*/
inline void outputHandle()
{
  // run all necessary handles first:
  ledBuild.run();
  macro.run();

  // write to output pins:
  analogWrite(StatusLedPin, map(ledVal, 0, 255, 255, 0));

  // determine led sequence:
  if (tuya_module.mcu_get_wifi_work_state() == AP_STATE)
    ledBuild.setSequence(ap_mode_led, 0, true);
  else if (tuya_module.mcu_get_wifi_work_state() == SMART_CONFIG_STATE)
    ledBuild.setSequence(smart_mode_led, 0, true);
  else if (tuya_module.mcu_get_wifi_work_state() == MODULE_UART_ERROR || tuya_module.mcu_get_wifi_work_state() == WIFI_STATE_UNKNOWN)
    ledBuild.setSequence(error_led, 0, true);
  else
    ledBuild.setSequence(idle_led, 0, true);
}

/*
  buttonHandle():
    handle WiFi Mode button, sets TUYA Module to Pairing mode
*/
inline void buttonHandle()
{
  /*
    WiFi_Config:
      0 = don't set WiFi
      1 = Smart_Mode
      2 = AP_Mode
  */
  static uint8_t WiFi_Config = 0;
  static uint32_t WiFi_setTimer = 0;
  const uint16_t WiFi_setInterval = 20000;

  // check for pin state change:
  if (pinMacro(pinInput.run()))
  {
    if (pinMacro) // button released
    {
      WiFi_Config = 0; // stop setting up WiFi
    }
    else // button pressed
    {
      WiFi_setTimer = (millis() - WiFi_setInterval); // reset wifi send timer so it sends immediately next timer
    }
  }

  // check if button has been pushed down for more than 1000ms
  if (!pinMacro && !pinMacro.triggered() && pinMacro.interval() > 1000)
  {
    pinMacro.trigger();

    // check if the module is already in Smart Config State:
    if (tuya_module.mcu_get_wifi_work_state() == SMART_CONFIG_STATE)
      WiFi_Config = 2; // set to AP Mode
    else
      WiFi_Config = 1; // set to smart config
  }

  // set up wifi according to 'WiFi_Config'
  if (WiFi_Config)
  {
    // check if WiFi already done with setup:
    if (WiFi_Config == 1 && tuya_module.mcu_get_wifi_work_state() == SMART_CONFIG_STATE)
      WiFi_Config = 0; // no need to set up WiFi pair anymore
    else if (WiFi_Config == 2 && tuya_module.mcu_get_wifi_work_state() == AP_STATE)
      WiFi_Config = 0; // no need to set up WiFi pair anymore,
    else
    {
      if (millis() - WiFi_setTimer >= WiFi_setInterval)
      {
        WiFi_setTimer = millis(); // reset timer

        if (WiFi_Config == 1) // set to smart config mode
          tuya_module.mcu_set_wifi_mode(SMART_CONFIG);
        else if (WiFi_Config == 2) // set to smart config mode
          tuya_module.mcu_set_wifi_mode(AP_CONFIG);
      }
    }
  }
}

/*
  moduleErrorHandle():
    handle resetting module when error occurs
*/
inline void moduleErrorHandle()
{
  // keep track of the state for the reset pin. false/LOW = reset, true/HIGH = idle
  static bool resetPinState = true;
  const uint16_t moduleResetInterval = 10000;

  if (tuya_module.mcu_get_wifi_work_state() == MODULE_UART_ERROR)
  {
    static uint32_t moduleResetTimer = 0;
    if (resetPinState)
    {
      if (millis() - moduleResetTimer >= moduleResetInterval)
      {
        moduleResetTimer = millis(); // reset timer

        // set pin to LOW and OUTPUT:
        digitalWrite(TUYA_ModuleResetPin, LOW);
        pinMode(TUYA_ModuleResetPin, OUTPUT);

        // set pin state:
        resetPinState = false;
      }
    }
    else
    {
      if (millis() - moduleResetTimer >= 300)
      {
        moduleResetTimer = millis(); // reset timer

        // set pin to LOW and INPUT:
        pinMode(TUYA_ModuleResetPin, INPUT);
        digitalWrite(TUYA_ModuleResetPin, LOW);

        // set pin state:
        resetPinState = true;
      }
    }
  }
  else
  {
    // no module error, make sure pin is high:
    if (!resetPinState)
    {
      // set reset pin to input:
      pinMode(TUYA_ModuleResetPin, INPUT);
      digitalWrite(TUYA_ModuleResetPin, LOW);

      resetPinState = true; // reset pin state
    }
  }
}

/*
  dp_proccess():
    all DPID commands pass through this function
*/
unsigned char dp_process(unsigned char dpid, const unsigned char value[], unsigned short length)
{
  if (dpid == DPID_Button)
  {
    dpidButtonValue = tuya_module.mcu_get_dp_download_data(dpid, value, length);
    tuya_module.mcu_dp_update(DPID_Button, dpidButtonValue, 1);
  }
  // return:
  return TY_SUCCESS;
}

/*
  dp_update_all():
    updates all DPID values and resets update timer
*/
void dp_update_all()
{
  // reset interval timer:
  updateAll_Timer = millis();

  tuya_module.mcu_dp_update(DPID_Button, dpidButtonValue, 1);
}

// smart mode led sequence, fast pulse:
SB_FUNCT(smart_mode_led, macro.ready(ledVal))
SB_STEP(macro.quadEase(ledVal, 0, 170);)
SB_STEP(macro.quadEase(ledVal, 130, 170);)
SB_STEP(macro.delay(ledVal, 100);)
SB_STEP(macro.quadEase(ledVal, 0, 170);)
SB_STEP(macro.delay(ledVal, 100);)
SB_STEP(ledBuild.loop(1);)
SB_END

// ap mode led sequence, slow pulse:
SB_FUNCT(ap_mode_led, macro.ready(ledVal))
SB_STEP(macro.quadEase(ledVal, 0, 170);)
SB_STEP(macro.quadEase(ledVal, 130, 170);)
SB_STEP(macro.delay(ledVal, 1400);)
SB_STEP(macro.quadEase(ledVal, 0, 170);)
SB_STEP(macro.delay(ledVal, 1400);)
SB_STEP(ledBuild.loop(1);)
SB_END

// idle led, steady with very soft glow:
SB_FUNCT(idle_led, macro.ready(ledVal))
SB_STEP(macro.quadEase(ledVal, 130, 3000);)
SB_STEP(macro.delay(ledVal, 400);)
SB_STEP(macro.quadEase(ledVal, 30, 3000);)
SB_STEP(macro.delay(ledVal, 400);)
SB_STEP(ledBuild.loop(0);)
SB_END

// error led, rapid flashing:
SB_FUNCT(error_led, macro.ready(ledVal))
SB_STEP(macro.set(ledVal, 130, 50);)
SB_STEP(macro.set(ledVal, 0, 50);)
SB_STEP(macro.set(ledVal, 130, 50);)
SB_STEP(macro.set(ledVal, 0, 50);)
SB_STEP(macro.set(ledVal, 130, 50);)
SB_STEP(macro.set(ledVal, 0, 1000);)
SB_STEP(ledBuild.loop(0);)
SB_END

// init led, slow fade in and small delay to allow full initialization of Arduino:
SB_FUNCT(init_led, macro.ready(ledVal))
SB_STEP(macro.quadEase(ledVal, 130, 2000);)
SB_STEP(macro.delay(ledVal, 1000);)
SB_END