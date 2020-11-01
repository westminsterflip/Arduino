#include "SevenSegmentTM1637.h"
int tpin0 = 0;
int tpin1 = 1;
int v0;
int v1;
float R1 = 10000;
float logr20, r20, t0 = -999, lpwm0 = 0;
float logr21, r21, t1 = -999, lpwm1 = 0;
float c1 = 1.123278976e-03, c2 = 2.356740178e-04, c3 = 0.769208529e-07;

const byte PIN_CLK = 4;   // define CLK pin (any digital pin)
const byte PIN_DIO = 5;   // define DIO pin (any digital pin)
SevenSegmentTM1637    display(PIN_CLK, PIN_DIO);

void setup() {
  Serial.begin(9600);
  display.begin();
  display.setBacklight(10);
  display.print("HI");
  TCCR2B = TCCR2B & B11111000 | B00000001;    // set timer 1 divisor to     1 for PWM frequency of 31372.55 Hz
}

void loop() {
  v0 = analogRead(tpin0);
  if (v0 != 0) {
    r20 = R1 * (1023.0 / (float)v0 - 1.0);
    logr20 = log(r20);
    t0 = (1.0 / (c1 + c2 * logr20 + c3 * pow(logr20, 3)));
    t0 -= 273.15;
    if (t0 < 0)
      t0 = -273.15;
  } else {
    t0 = -273.15;
  }
  v1 = analogRead(tpin1);
  if (v1 != 0) {
    r21 = R1 * (1023.0 / (float)v1 - 1.0);
    logr21 = log(r21);
    t1 = (1.0 / (c1 + c2 * logr21 + c3 * pow(logr21, 3)));
    t1 -= 273.15;
    if (t1 < 0)
      t1 = -273.15;
  } else {
    t1 = -273.15;
  }
  float pwm0 = 29 * t0 / 7 - 100 / 7;
  float pwm1 = 29 * t1 / 7 - 100 / 7;
  int pwmr0 = (t0 < 30) ? 0 : pwm0;
  int pwmr1 = (t1 < 30) ? 0 : pwm1;
  pwmr0 = (t0 <= 0) ? 255 : pwmr0;
  pwmr1 = (t1 <= 0) ? 255 : pwmr1;
  if (lpwm0 != pwmr0 && lpwm1 != pwmr1) {
    if (lpwm0 == 0)
      analogWrite(3, 255);
    if (lpwm1 == 0)
      analogWrite(11, 255);
    if (lpwm1 == 0 || lpwm0 == 0)
      delay(100);
    analogWrite(3, pwmr0);
    analogWrite(11, pwmr1);
    lpwm0 = pwmr0;
    lpwm1 = pwmr1;
  } else if (lpwm0 != pwmr0) {
    if (lpwm0 == 0) {
      analogWrite(3, 255);
      delay(100);
    }
    analogWrite(3, pwmr0);
    lpwm0 = pwmr0;
  } else if (lpwm1 != pwmr1) {
    if (lpwm1 == 0) {
      analogWrite(11, 255);
      delay(100);
    }
    analogWrite(11, pwmr1);
    lpwm1 = pwmr1;
  }
  String tdisp = String((round(t0) >= 100) ? 99 : round(t0)) + String((round(t1) >= 100) ? 99 : round(t1));
  display.clear();
  display.print(tdisp);
  Serial.print("t0:");
  Serial.print(t0);
  Serial.print(" t1:");
  Serial.print(t1);
  Serial.print(" pwm0:");
  Serial.print(pwmr0);
  Serial.print(" pwm1:");
  Serial.println(pwmr1);
  delay(1000);
}
