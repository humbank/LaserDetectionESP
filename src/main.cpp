#include <Arduino.h>

#define LASER_PIN 26
#define SENSOR_PIN 34
#define LED_BUILTIN 2

int ambient = 0;
int laserHit = 0;

int readSensor() {
  return analogRead(SENSOR_PIN);
}

int averageRead(int samples) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += readSensor();
    delay(5);
  }
  return sum / samples;
}

void calibrate() {
  //measure ambient
  digitalWrite(LASER_PIN, LOW);
  delay(1000);

  Serial.println("Calibrating...");
  ambient = averageRead(100);

  //measure laser

  digitalWrite(LASER_PIN, HIGH);
  delay(1500);

  laserHit = averageRead(100);

  Serial.print("Ambient: ");
  Serial.println(ambient);

  Serial.print("Laser hit: ");
  Serial.println(laserHit);

  delay(1000);
}

boolean beam(){
  int value = averageRead(5);

  
  int deviation = abs(value - laserHit);

  
  int tolerance = abs(laserHit - ambient) * 0.4;

  if (deviation >= tolerance) {
    digitalWrite(LED_BUILTIN, LOW);
    return false;
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
    return true;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LASER_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  calibrate();
}



void loop() {
  Serial.print("Beam established: ");
  Serial.println(beam());
  delay(20);
}