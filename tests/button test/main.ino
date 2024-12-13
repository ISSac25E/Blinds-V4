void setup()
{
  pinMode(0, INPUT_PULLUP);
  pinMode(BUILTIN_LED, OUTPUT);
}

void loop()
{
  digitalWrite(BUILTIN_LED, digitalRead(0));
  delay(10);
}