#include <Servo.h>
Servo servo;
void setup() {
  Serial.begin(9600);
  //Servo on pin 9, change number to change pin
  servo.attach(9);
}

void loop() {
  //if no input Serial will return 0, which isn't what we want
  while(Serial.available()==0)
    delay(200);
  int angle = Serial.parseInt();
  Serial.parseInt();
  Serial.print("got: ");
  Serial.println(angle);
  servo.write(angle);
}
