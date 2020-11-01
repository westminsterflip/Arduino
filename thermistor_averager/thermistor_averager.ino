#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

int tpin0 = 0;
int tpin1 = 1;
int tpin2 = 2;
int tpin3 = 3;
int v0;
int v1;
int v2;
int v3;
float R1 = 10000;
float logr20, r20, t0 = -999;
float logr21, r21, t1 = -999;
float logr22, r22, t2 = -999;
float logr23, r23, t3 = -999;
float c1 = 1.123278976e-03, c2 = 2.356740178e-04, c3 = 0.769208529e-07;
float maxtemp;
int cs = 4;
int ud = 5;
int inc = 6;
float a50k = 0.9657917765e-03, b50k = 2.106711297e-04, c50k = 0.8590386534e-07;
float curR = 0;
float atMax = -273.15;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

void setup() {
  pinMode(cs, OUTPUT);
  pinMode(ud, OUTPUT);
  pinMode(inc, OUTPUT);
  digitalWrite(inc, HIGH);
  digitalWrite(cs, HIGH);
  digitalWrite(ud, HIGH);
  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  digitalWrite(inc, HIGH);
  digitalWrite(cs, LOW);
  digitalWrite(ud, LOW);
  Serial.println("starting zero");
  float count = 100000;
  for (; count != 0; count -= 1000) {
    digitalWrite(inc, HIGH);
    delayMicroseconds(50);
    digitalWrite(inc, LOW);
    delayMicroseconds(50);
  }
  Serial.println("zeroed");
  delay(1000);
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
  v2 = analogRead(tpin2);
  if (v2 != 0) {
    r22 = R1 * (1023.0 / (float)v2 - 1.0);
    logr22 = log(r22);
    t2 = (1.0 / (c1 + c2 * logr22 + c3 * pow(logr22, 3)));
    t2 -= 273.15;
    if (t2 < 0)
      t2 = -273.15;
  } else {
    t2 = -273.15;
  }
  v3 = analogRead(tpin3);
  if (v3 != 0) {
    r23 = R1 * (1023.0 / (float)v3 - 1.0);
    logr23 = log(r23);
    t3 = (1.0 / (c1 + c2 * logr23 + c3 * pow(logr23, 3)));
    t3 -= 273.15;
    if (t3 < 0)
      t3 = -273.15;
  } else {
    t3 = -273.15;
  }

  maxtemp = max(t0, max(t1, max(t2, t3)));
  if(maxtemp > atMax)
    atMax = maxtemp;
  float sent = setTo(maxtemp);
  float logs = log(sent);
  float sentT = (1.0 / (a50k + b50k * logs + c50k * pow(logs, 3)));
  sentT -= 273.15;

  Serial.print(t0);
  Serial.print(" ");
  Serial.print(t1);
  Serial.print(" ");
  Serial.print(t2);
  Serial.print(" ");
  Serial.print(t3);
  Serial.print(" atMax: ");
  Serial.println(atMax);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.print("ssr:   ");
  display.println(t0);
  display.print("ZL:    ");
  display.println(t1);
  display.print("psu1:  ");
  display.println(t2);
  display.print("psu2:  ");
  display.println(t3);
  display.print("max:   ");
  display.println(maxtemp);
  display.print("sent:  ");
  display.println(sentT);
  display.print("atMax: ");
  display.println(atMax);
  display.display();

  delay(500);
}

void store() {
  digitalWrite(inc, HIGH);
  delayMicroseconds(1);
  digitalWrite(cs, HIGH);
  delay(100);
}

float setTo(float setTemp) {
  setTemp += 273.15;
  float x = (a50k - 1 / setTemp) / (2 * c50k);
  float y = sqrt(pow(b50k / (3 * c50k), 3) + pow(x, 2));
  float R = exp(cbrt(y - x) - cbrt(y + x));
  R = round(R / 1000) * 1000;
  if (curR > R) {
    Serial.print("down: ");
    Serial.print(curR);
    Serial.print(" -> ");
    Serial.println(R);
    digitalWrite(inc, HIGH);
    digitalWrite(cs, LOW);
    digitalWrite(ud, LOW);
    for (; curR > R; curR -= 1000) {
      digitalWrite(inc, HIGH);
      delay(10);
      digitalWrite(inc, LOW);
      delay(10);
    }
  } else if (curR < R) {
    Serial.print("up: ");
    Serial.print(curR);
    Serial.print(" -> ");
    Serial.println(R);
    digitalWrite(inc, HIGH);
    digitalWrite(cs, LOW);
    digitalWrite(ud, HIGH);
    for (; curR < R; curR += 1000) {
      digitalWrite(inc, HIGH);
      delay(10);
      digitalWrite(inc, LOW);
      delay(10);
    }
  }
  delay(1);
  store();
  return R;
}
