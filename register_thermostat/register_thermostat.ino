// include the SevenSegmentTM1637 library
#include "SevenSegmentTM1637.h"
#include <Servo.h>
#include <EEPROM.h>
#include <ezButton.h>

const byte PIN_CLK = 4;   // define CLK pin (any digital pin)
const byte PIN_DIO = 5;   // define DIO pin (any digital pin)
SevenSegmentTM1637    display(PIN_CLK, PIN_DIO);
//The seven segment display is set to timeout after a set number of seconds.  
//This reduces the power consumption (on USB power) from .02-.05A to 0A
//(or low enough that my usb power meter can't read it)
//Haven't tested power draw on Vin

//buttons between ground and defined pin (pulled low)
const byte setButton = 6;
const byte minusButton = 7;
const byte plusButton = 8;
const int LONG_PRESS_TIME = 3000;
const int SHORT_PRESS_TIME = 1000;
int setState = 0, minusState = 0, plusState = 0;
int lastSet = 0, lastMinus = 0, lastPlus = 0;
long pressTime = 0;
long releaseTime = 0;
bool pressed = false;
bool longPressed = false;
ezButton buttonS(setButton);
ezButton buttonM(minusButton);
ezButton buttonP(plusButton);

//Max servo position ~160 for SM-S2309S that came with my starter kit
const byte servoPin = 9;
Servo servo;
byte openPos = 0; //placeholder
byte closedPos = 160; //placeholder

const byte airThermPin = 0;
const byte roomThermPin = 1;
int VoA, VoR; //A: air from vent, R: ambient air
const float R1 = 100000;
float logR2a, R2a, Ta, TFa, logR2r, R2r, Tr, TFr;
const float c1 = 3.493649684e-3, c2 = -1.221890210e-4, c3 = 8.357479746e-7;

struct settings {
  int max;
  int min;
  byte hyst;
  bool useF;
  bool open;
  bool onAuto;
  bool raising;
  float SCREEN_TIMEOUT;
  byte openPos;
  byte closedPos;
  bool firstRun;
};

typedef struct settings Settings;
Settings tStatSet;

String menuItem[] = {
  "MAX",
  "MIN",
  "HYST",
  "F-C",
  "OPOS",
  "CPOS",
  "TOUT",
  "BRIT",
  "EXIT"
};
byte menuItems;
float lastDisp = 0;
float lastChange = 0;
float lastPress = 0;
bool screenOff = false;

void setup() {
  Serial.begin(9600);         // initializes the Serial connection @ 9600 baud

  display.begin();            // initializes the display
  display.setBacklight(100);  // set the brightness to 100 %
  display.print("INIT");      // display INIT on the display

  buttonS.setDebounceTime(50);
  buttonM.setDebounceTime(50);
  buttonP.setDebounceTime(50);

  tStatSet = EEPROM.get(0, tStatSet);
  Serial.print("MAX: ");
  Serial.println(tStatSet.max);
  Serial.print("MIN: ");
  Serial.println(tStatSet.min);
  Serial.print("HYST: ");
  Serial.println(tStatSet.hyst);
  Serial.print("F-C: ");
  Serial.println(tStatSet.useF);
  Serial.print("ONAUTO: ");
  Serial.println((tStatSet.onAuto) ? "TRUE" : "FALSE");
  Serial.print("OPEN: ");
  Serial.println((tStatSet.open) ? "TRUE" : "FALSE");
  Serial.print("SCREEN TIMEOUT: ");
  Serial.println(tStatSet.SCREEN_TIMEOUT);
  Serial.print("FIRST RUN: ");
  Serial.println((tStatSet.firstRun) ? "TRUE" : "FALSE");
  Serial.println((unsigned long)&menuItem[6]);
  Serial.println((unsigned long)&menuItem + sizeof(menuItem));
  bool foundSize = false;
  for (int i; !foundSize; i++) { //dynamically find size of array with addresses
    if ((unsigned long)(&menuItem[i]) >= (unsigned long)(&(menuItem)) + sizeof(menuItem)) {
      menuItems = i;
      foundSize = true;
    }
  }
  Serial.println(menuItems);

  if (tStatSet.firstRun) {
    tStatSet.max = 22;
    tStatSet.min = 16;
    tStatSet.hyst = 0;
    tStatSet.useF = false;
    tStatSet.open = true;
    tStatSet.onAuto = true;
    tStatSet.SCREEN_TIMEOUT = 10000;
    tStatSet.firstRun = false;
    EEPROM.put(0, tStatSet);
  }

  delay(1000);                // wait 1000 ms
}

void loop() {
  //T calculation
  VoA = analogRead(airThermPin);
  VoR = analogRead(roomThermPin);
  R2a = R1 * (1023.0 / (float)VoA - 1.0);
  R2r = R1 * (1023.0 / (float)VoR - 1.0);
  logR2a = log(R2a);
  logR2r = log(R2r);
  Ta = (1.0 / (c1 + c2 * logR2a + c3 * pow(logR2a, 3)));
  Tr = (1.0 / (c1 + c2 * logR2r + c3 * pow(logR2r, 3)));
  Ta -= 273.15;
  Tr -= 273.15;
  TFa = (Ta * 9.0) / 5.0 + 32;
  TFr = (Tr * 9.0) / 5.0 + 32;
  int TdispA = (tStatSet.useF) ? TFa : Ta;
  int TdispR = (tStatSet.useF) ? TFr : Tr;

  if (millis() - lastDisp >= 500) {
    //if (!screenOff) {
    if (pressed) {
      display.clear();
      display.print(TdispA);
    } else {
      display.clear();
      display.print(TdispR);
    }
    lastDisp = millis();
    //}
  }

  if (tStatSet.onAuto && millis() - lastChange >= 1000) {
    if (Ta < Tr - 3 && TdispR > tStatSet.max) { //if vent air cooler than room and room warmer than max temp open
      servo.write(openPos);
      tStatSet.open = true;
      tStatSet.raising = false;
    } else if (Ta > Tr + 3 && TdispR < tStatSet.min) { //if vent air warmer than room and room colder than min open
      servo.write(openPos);
      tStatSet.open = true;
      tStatSet.raising = true;
    } else if ((tStatSet.raising || Ta > Tr + 3) && TdispR >= (tStatSet.max + tStatSet.hyst)) { //if tStatSet.raising temp and at or higher than max temp + hysteresis
      servo.write(closedPos);
      tStatSet.open = false;
    } else if ((!tStatSet.raising || Ta < Tr - 3) && TdispR <= (tStatSet.min - tStatSet.hyst)) { //if lowering temp and at or lower than min temp - hysteresis
      servo.write(openPos);
      tStatSet.open = false;
    }
    EEPROM.put(0, tStatSet);
    lastChange = millis();
  } else {
    if (tStatSet.open) {
      servo.write(openPos);
    } else {
      servo.write(closedPos);
    }
  }

  if(!servo.attached()){
    servo.attach(servoPin);
    Serial.println("SERVO ATTACHED");
  }

  //button logic
  buttonS.loop();
  buttonP.loop();
  buttonM.loop();

  if (buttonM.isPressed() || buttonP.isPressed()) {
    lastPress = millis();
  }
  if (!screenOff) {
    if ((tStatSet.open || tStatSet.onAuto) && buttonM.isPressed() && buttonP.getState()) {
      tStatSet.open = false;
      tStatSet.onAuto = false;
      servo.write(closedPos);
      EEPROM.put(0, tStatSet);
    } else if ((!tStatSet.open || tStatSet.onAuto) && buttonP.isPressed() && buttonM.getState()) {
      tStatSet.open = true;
      tStatSet.onAuto = false;
      servo.write(openPos);
      EEPROM.put(0, tStatSet);
    }
  }

  if (buttonS.isPressed()) {
    lastPress = pressTime = millis();
    pressed = true;
    longPressed = false;
    display.clear();
    display.print(TdispA);
  }

  if (buttonS.isReleased()) {
    pressed = false;

    long pressDur = millis() - pressTime;

    if (pressDur < LONG_PRESS_TIME) { //not a long press
      display.clear();
      display.print(TdispR);
      tStatSet.onAuto = true;
      if (pressDur < SHORT_PRESS_TIME) {
        tStatSet.onAuto = true;
        EEPROM.put(0, tStatSet);
      }
      lastPress = millis(); //screen timeout starts after release
    }
  }

  if (pressed == true && longPressed == false) {
    long pressDur = millis() - pressTime;

    if (pressDur >= LONG_PRESS_TIME) {
      longPressed = true;
      lastPress = millis();
      //open settings
      display.clear();
      bool exitPressed = false;
      display.print(menuItem[0]);
      int curMenuItem = 0;
      buttonS.loop();
      bool setUnpressed = buttonS.getState(); //button pulled low so getState=1 when not pressed
      if (!setUnpressed) {
        while (!buttonS.getState()) {
          buttonS.loop();
        }
        setUnpressed = true;
      }
      Serial.println("set unpressed");
      while (!exitPressed) {
        buttonS.loop();
        buttonM.loop();
        buttonP.loop();
        if (buttonS.isPressed() || buttonM.isPressed() || buttonP.isPressed()) {
          lastPress = millis();
        }
        if (buttonM.isPressed() && buttonP.getState()) {
          curMenuItem = (curMenuItem + menuItems - 1) % menuItems;
          display.clear();
          display.print(menuItem[curMenuItem]);
        } else if (buttonP.isPressed() && buttonM.getState()) {
          curMenuItem = (curMenuItem + 1) % menuItems;
          display.clear();
          display.print(menuItem[curMenuItem]);
        } else if (buttonS.isPressed()) {
          if (curMenuItem == menuItems - 1) {
            Serial.println("exiting settings");
            exitPressed = true;
            EEPROM.put(0, tStatSet);
          } else {
            display.clear();
            Serial.print("selected menu item: ");
            Serial.println(menuItem[curMenuItem]);
            buttonS.loop();
            buttonM.loop();
            buttonP.loop();
            switch (curMenuItem) { //repeated code but each requires different checks
              case 0:
                {
                  display.print(tStatSet.max);
                  while (!buttonS.isPressed()) {
                    buttonS.loop();
                    buttonM.loop();
                    buttonP.loop();
                    if (buttonM.isPressed() && buttonP.getState() && tStatSet.max - 1 >= tStatSet.min) {
                      tStatSet.max--;
                    } else if (buttonP.isPressed() && buttonM.getState() && (!tStatSet.useF && tStatSet.max + 1 <= 300 || tStatSet.useF && tStatSet.max + 1 <= 572)) { //thermistor max 300C
                      tStatSet.max++;
                    }
                    if (buttonM.isPressed() || buttonP.isPressed()) {
                      display.clear();
                      display.print(tStatSet.max);
                    }
                    //delay(100);
                  }
                  if (tStatSet.min > tStatSet.max)
                    tStatSet.min = tStatSet.max;
                }
                break;
              case 1:
                {
                  display.print(tStatSet.min);
                  while (!buttonS.isPressed()) {
                    buttonS.loop();
                    buttonM.loop();
                    buttonP.loop();
                    if (buttonM.isPressed() && buttonP.getState() && (!tStatSet.useF && tStatSet.min - 1 >= -273 || tStatSet.useF && tStatSet.min - 1 >= -459)) { //don't need sub absolute 0
                      tStatSet.min--;
                    } else if (buttonP.isPressed() && buttonM.getState() && tStatSet.min + 1 <= tStatSet.max) {
                      tStatSet.min++;
                    }
                    if (buttonM.isPressed() || buttonP.isPressed()) {
                      display.clear();
                      display.print(tStatSet.min);
                    }
                    //delay(100);
                  }
                  if (tStatSet.max < tStatSet.min)
                    tStatSet.max = tStatSet.min;
                }
                break;
              case 2:
                {
                  display.print(tStatSet.hyst);
                  while (!buttonS.isPressed()) {
                    buttonS.loop();
                    buttonM.loop();
                    buttonP.loop();
                    if (buttonM.isPressed() && buttonP.getState() && tStatSet.hyst - 1 >= 0) {
                      tStatSet.hyst--;
                    } else if (buttonP.isPressed() && buttonM.getState() && tStatSet.hyst + 1 <= 255) {
                      tStatSet.hyst++;
                    }
                    if (buttonM.isPressed() || buttonP.isPressed()) {
                      display.clear();
                      display.print(tStatSet.hyst);
                    }
                    ////delay(100);
                  }
                }
                break;
              case 3:
                {
                  bool oldUseF = tStatSet.useF;
                  display.print((tStatSet.useF) ? "F" : "C");
                  while (!buttonS.isPressed()) {
                    buttonS.loop();
                    buttonM.loop();
                    buttonP.loop();
                    //Serial.println("state: ");
                    if ((buttonM.isPressed() && buttonP.getState()) || (buttonP.isPressed() && buttonM.getState())) {
                      if (tStatSet.useF) {
                        tStatSet.useF = false;
                        Serial.println("use c");
                      } else {
                        tStatSet.useF = true;
                        Serial.println("use f");
                      }
                      display.clear();
                      display.print((tStatSet.useF) ? "F" : "C");
                    }
                    //Serial.println(tStatSet.useF);
                    ////delay(100);
                  }
                  if (oldUseF != tStatSet.useF) {
                    if (tStatSet.useF) {
                      tStatSet.max = (float)tStatSet.max * 9.0 / 5.0 + 32;
                      tStatSet.min = (float)tStatSet.min * 9.0 / 5.0 + 32;
                      tStatSet.hyst = (float)tStatSet.hyst * 9.0 / 5.0;
                    } else {
                      tStatSet.max = (float)(tStatSet.max - 32) * 5.0 / 9.0;
                      tStatSet.min = (float)(tStatSet.min - 32) * 5.0 / 9.0;
                      tStatSet.hyst = (float)tStatSet.hyst / 5.0 * 9.0;
                    }
                  }
                }
                break;
              case 4:
                {
                  display.print((int)tStatSet.SCREEN_TIMEOUT / 1000);
                  Serial.println((int)tStatSet.SCREEN_TIMEOUT / 1000);
                  while (!buttonS.isPressed()) {
                    buttonS.loop();
                    buttonM.loop();
                    buttonP.loop();
                    if (buttonM.isPressed() && buttonP.getState() && tStatSet.SCREEN_TIMEOUT - 1000 >= 0) {
                      tStatSet.SCREEN_TIMEOUT -= 1000;
                    } else if (buttonP.isPressed() && buttonM.getState() && tStatSet.SCREEN_TIMEOUT + 1000 <= 9999000) { //4 digit display, can't be longer than 9999 sec
                      tStatSet.SCREEN_TIMEOUT += 1000;
                    }
                    if (buttonM.isPressed() || buttonP.isPressed()) {
                      display.clear();
                      display.print((int)tStatSet.SCREEN_TIMEOUT / 1000);
                    }
                  }
                  break;
                }
            }
            display.clear();
            display.print(menuItem[curMenuItem]);
          }
        }
        //maybe shouldn't time out in menu?
        //slats don't operate in menu
        if (!screenOff && millis() >= (lastPress + tStatSet.SCREEN_TIMEOUT)) {
          //display.setBacklight(0);
          Serial.println("screen off");
          display.off();
          screenOff = true;
        } else if (screenOff && millis() < (lastPress + tStatSet.SCREEN_TIMEOUT)) {
          Serial.println("screen on");
          display.setBacklight(100);
          screenOff = false;
        }
      }
    }
  }

  if (!screenOff && millis() >= (lastPress + tStatSet.SCREEN_TIMEOUT)) {
    //display.setBacklight(0);
    Serial.println("screen off");
    display.off();
    screenOff = true;
  } else if (screenOff && millis() < (lastPress + tStatSet.SCREEN_TIMEOUT)) {
    Serial.println("screen on");
    display.setBacklight(100);
    screenOff = false;
  }
}
