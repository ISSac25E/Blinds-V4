/*
  bucket structs to publish and subscribe to
*/
struct device_info
{
  bool init = false;
  char c[10];
};

struct simple_LED_4
{
  bool state = false;
};

struct simple_Button_3
{
  bool state = false;
};