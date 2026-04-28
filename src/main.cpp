#include <Arduino.h>

#define LASER_PIN 26
#define SENSOR_PIN 34
#define LED_BUILTIN 2

#define RXD2 16
#define TXD2 17

HardwareSerial RS485(2);

int ambient = 0;
int laserHit = 0;
bool running = false;

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

// ---------------- CALIBRATION ----------------
void calibrate() {
  RS485.println("FROM:1:CALIBRATING");

  // laser OFF during ambient measurement
  digitalWrite(LASER_PIN, LOW);
  delay(1000);
  ambient = averageRead(50);

  // laser ON for hit measurement
  digitalWrite(LASER_PIN, HIGH);
  delay(1500);
  laserHit = averageRead(50);

  RS485.print("FROM:1:CAL_DONE:");
  RS485.print(ambient);
  RS485.print(":");
  RS485.println(laserHit);

  // IMPORTANT: always turn laser OFF at end
  digitalWrite(LASER_PIN, LOW);
}

// ---------------- BEAM CHECK ----------------
bool beam() {
  int value = averageRead(5);

  int deviation = abs(value - laserHit);
  int tolerance = abs(laserHit - ambient) * 0.4;

  return !(deviation >= tolerance);
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  RS485.begin(115200, SERIAL_8N1, RXD2, TXD2);

  pinMode(LASER_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LASER_PIN, LOW); // SAFE DEFAULT
}

// ---------------- LOOP ----------------
void loop() {

  // ----------- COMMAND HANDLING -----------
  if (RS485.available()) {
    String cmd = RS485.readStringUntil('\n');
    cmd.trim();

    if (cmd == "1:CALIBRATE_LASER") {
      calibrate();
      running = false;
    }

    if (cmd == "1:START_LASER") {
      running = true;
      RS485.println("FROM:1:RUNNING");
    }

    if (cmd == "1:STOP_LASER") {
      running = false;
      RS485.println("FROM:1:STOPPED");

      // IMPORTANT: ensure laser OFF when stopped
      digitalWrite(LASER_PIN, LOW);
    }
  }

  
  if (!running) {
    digitalWrite(LASER_PIN, LOW);
    delay(50);
  }
  else{
    bool ok = beam();
    digitalWrite(LASER_PIN, HIGH);
    digitalWrite(LED_BUILTIN, ok ? HIGH : LOW);

    RS485.print("FROM:1:BEAM:");
    RS485.println(ok);

    delay(100);
  }
}