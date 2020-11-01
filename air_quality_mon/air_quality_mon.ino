#include "SevenSegmentTM1637.h"
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"
#include "SevenSegmentExtended.h"
#include "SevenSegmentFun.h"

//On the nano if the normal Adafruit SSD1306 library is used there are no errors thrown
//from the IDE, but there isn't enough memory on the board so variables start to get
//erased and there are errors and lost data
//With a nano the SH1106 would be rather difficult to use in it's place

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3C

SSD1306AsciiAvrI2c display;

class AirQuality
{
  public:
    int i ;
    long vol_standard;
    int init_voltage;
    int first_vol;
    int last_vol;
    long temp;
    int counter;
    boolean timer_index;
    boolean error;
    void init(int pin);
    int slope(void);
  private:
    int _pin;
    void avgVoltage(void);
};

//Adafruit_SSD1306 display(128, 64, &Wire, -1);
const byte PIN_CLK = 2;   // define CLK pin (any digital pin)
const byte PIN_DIO = 3;   // define DIO pin (any digital pin)
SevenSegmentFun    display2(PIN_CLK, PIN_DIO);

String l1 = "", l2 = "", l3 = "", l4 = "", l5 = "", l6 = "", l7 = "", l8 = "";

AirQuality airqualitysensor;
int current_quality = -1;
void setup()
{
  Serial.begin(9600);
  delay(1000);
  display.begin(&Adafruit128x64, I2C_ADDRESS);
  display.setFont(Adafruit5x7);
  display.clear();
  display2.begin();
  display2.setBacklight(10);
  display2.print("HI");
  airqualitysensor.init(A0);
}

void loop()
{
  airqualitysensor.slope();
}

//from seeed's library
ISR(TIMER2_OVF_vect)
{
  if (airqualitysensor.counter == 122) //set 2 seconds as a detected duty
  {
    airqualitysensor.last_vol = airqualitysensor.first_vol;
    airqualitysensor.first_vol = analogRead(A0);
    airqualitysensor.counter = 0;
    airqualitysensor.timer_index = 1;
    PORTB = PORTB ^ 0x20;
  }
  else
  {
    airqualitysensor.counter++;
  }
}

//from seeed's library with added output
void AirQuality::avgVoltage()
{
  if (i == 150) //sum 5 minutes
  {
    vol_standard = temp / 150;
    temp = 0;
    Serial.print(F("Vol_standard in 5 minutes:"));
    Serial.println(vol_standard);
    String temp = "vol_standard " + String(vol_standard, DEC);
    oprint(temp, "");
    i = 0;
  }
  else
  {
    temp += first_vol;
    i++;
  }
}

//from seeed's library with added output
void AirQuality::init(int pin)
{
  _pin = pin;
  pinMode(_pin, INPUT);
  unsigned char i = 0;
  Serial.println(F("sys_starting..."));
  oprint(F("sys_starting..."), F("sys_starting..."));
  delay(20000);//200000
  init_voltage = analogRead(_pin);
  Serial.println(F("The init voltage is ..."));
  Serial.println(init_voltage);
  String temp = F("init_voltage: ");
  temp += init_voltage;
  oprint(temp, temp);
  while (init_voltage)
  {
    if (init_voltage < 798 && init_voltage > 10) // the init voltage is ok
    {
      first_vol = analogRead(_pin); //initialize first value
      last_vol = first_vol;
      vol_standard = last_vol;
      Serial.println(F("Sensor ready."));
      oprint(F("Sensor ready."), F("Sensor ready"));
      error = false;;
      break;
    }
    else if (init_voltage > 798 || init_voltage <= 10)
    {
      i++;
      delay(60000);//60000
      Serial.println(F("waiting sensor init.."));
      oprint(F("waiting sensor init..."), F("waiting sensor init..."));
      init_voltage = analogRead(_pin);
      if (i == 5)
      {
        i = 0;
        error = true;
        Serial.println(F("Sensor Error!"));
        oprint(F("Sensor Error!"), F("Sensor Error!"));
      }
    }
    else
      break;
  }
  //init the timer
  TCCR2A = 0; //normal model
  TCCR2B = 0x07; //set clock as 1024*(1/16M)
  TIMSK2 = 0x01; //enable overflow interrupt
  Serial.println(F("Test begin..."));
  oprint(F("Test begin..."), F("Test begin..."));
  sei();
}

int AirQuality::slope(void)
{
  while (timer_index)
  {
    if (first_vol - last_vol > 400 || first_vol > 700)
    {
      Serial.print(F("v:"));
      Serial.print(first_vol);
      Serial.println(F("High pollution! Force signal active."));
      String temp = String(first_vol, DEC) + F(" High pollution! FORCE");
      String shortTemp = String(first_vol, DEC);
      if (shortTemp.length() == 2)
        shortTemp += " ";
      shortTemp += F(" High pollution! Force signal active!!!");
      oprint(temp, shortTemp);
      timer_index = 0;
      avgVoltage();
      return 0;
    }
    else if ((first_vol - last_vol > 400 && first_vol < 700) || first_vol - vol_standard > 150)
    {
      Serial.print(F("v:"));
      Serial.print(first_vol);
      Serial.println(F("\t High pollution!"));
      String temp = String(first_vol, DEC) + F(" High pollution!");
      String shortTemp = String(first_vol, DEC);
      if (shortTemp.length() == 2)
        shortTemp += " ";
      shortTemp += F("H");
      oprint(temp, shortTemp);
      timer_index = 0;
      avgVoltage();
      return 1;

    }
    else if ((first_vol - last_vol > 200 && first_vol < 700) || first_vol - vol_standard > 50)
    {
      //Serial.println(first_vol-last_vol);
      Serial.print(F("v:"));
      Serial.print(first_vol);
      Serial.println(F("\t Low pollution!"));
      String temp = String(first_vol, DEC) + F(" Low pollution.");
      String shortTemp = String(first_vol, DEC);
      if (shortTemp.length() == 2)
        shortTemp += F(" ");
      shortTemp += F("L");
      oprint(temp, shortTemp);
      timer_index = 0;
      avgVoltage();
      return 2;
    }
    else
    {
      avgVoltage();
      Serial.print(F("v:"));
      Serial.print(first_vol);
      Serial.println(F("\t Air fresh"));
      String temp = String(first_vol, DEC) + F(" Air fresh");
      String shortTemp = String(first_vol, DEC);
      if (shortTemp.length() == 1)
        shortTemp += F("  ");
      else if (shortTemp.length() == 2)
        shortTemp += F(" ");
      shortTemp += F("F");
      oprint(temp, shortTemp);
      timer_index = 0;
      return 3;
    }
  }
  return -1;
}

//emulate a scrolling display, each new line appears at the bottom
void oprint(String string, String shortString) {
  if (string == F(""))
    return;
  int cur = 8;
  display.setFont(Adafruit5x7);
  display.clear();
  if (l1 != "" && l1 != "NULL") {
    display.println(l1);
  }
  if (l2 != "") {
    display.println(l2);
  }
  if (l3 != "") {
    display.println(l3);
  }
  if (l4 != "") {
    display.println(l4);
  }
  if (l5 != "") {
    display.println(l5);
  }
  if (l6 != "") {
    display.println(l6);
  }
  if (l7 != "") {
    display.println(l7);
  }
  if (l8 != "") {
    display.println(l8);
  }
  display.println(string);
  if (l1 == "") {
    l1 = string;
  } else if (l2 == "") {
    l2 = string;
  } else if (l3 == "") {
    l3 = string;
  } else if (l4 == "") {
    l4 = string;
  } else if (l5 == "") {
    l5 = string;
  } else if (l6 == "") {
    l6 = string;
  } else if (l7 == "") {
    l7 = string;
  } else if (l8 == "") {
    l1 = "NULL";
    l8 = string;
  } else {
    l1 = "NULL";
    l2 = l3;
    l3 = l4;
    l4 = l5;
    l5 = l6;
    l6 = l7;
    l7 = l8;
    l8 = string;
  }
  display2.clear();
  display2.print(shortString);
  if (shortString.equals(F("Sensor Error!")) || shortString.indexOf(F("Force")) != -1)
    display2.blink();
}
