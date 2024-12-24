#include <ESP8266WiFi.h>

/* 
  In this test, I attempted to crash the esp8266 in some way through misaligned data, I failed
  the esp8266 seems to have robust ram alignment recovery. while there most certainly is a performance hit, it wont crash
*/

void setup()
{
  Serial.begin(115200);

  // Allocate a misaligned buffer in IRAM
  char *misalignedIRAM = (char *)malloc(16);
  if (!misalignedIRAM)
  {
    Serial.println("Memory allocation failed.");
    return;
  }

  // Ensure the buffer is misaligned for double
  char *misalignedPtr = misalignedIRAM + 1;

  // Attempt misaligned access
  double *doublePtr = reinterpret_cast<double *>(misalignedPtr);
  *doublePtr = 3.14159;       // Writing misaligned data
  Serial.println(*doublePtr); // Reading misaligned data

  Serial.println("This line should not be reached if there's a crash.");
  free(misalignedIRAM); // Free memory to avoid a leak
}

void loop()
{
  // Empty loop

  Serial.println("input >>");
  while (!Serial.available())
    ;

  delay(20);

  char c[1024];
  int index = 0;
  while (Serial.available())
    c[index++] = Serial.read();

  Serial.println("parsing as floats:");

  Serial.println(*((float*)(c)));
  Serial.println(*((float*)(c+1)));
  Serial.println(*((float*)(c+2)));
  Serial.println(*((float*)(c+3)));
  Serial.println(*((float*)(c+4)));
  Serial.println(*((float*)(c+5)));
  Serial.println(*((float*)(c+6)));
  Serial.println(*((float*)(c+7)));
  Serial.println(*((float*)(c+8)));

  Serial.println();
}
