#include <Arduino.h>

const int buttonPin = 0; // GPIO pin for the button
const int ledPin = 2;    // GPIO pin for the LED

void setup() {
  pinMode(buttonPin, INPUT);  // Set button pin as input
  pinMode(ledPin, OUTPUT);    // Set LED pin as output
  Serial.begin(115200);       // Initialize serial communication at 115200 baud
}

void loop() {
  int buttonState = digitalRead(buttonPin); // Read the state of the button
  digitalWrite(ledPin, buttonState);        // Set the LED to the button state

  Serial.println(buttonState);              // Print the LED state to the serial monitor
  delay(10);                               // Add a small delay to avoid spamming too fast
}