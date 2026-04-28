#include <Arduino.h>
#include <ArduinoJson.h>

#define LASER_PIN 26
#define SENSOR_PIN 34
#define LED_BUILTIN 2

#define RXD2 16
#define TXD2 17

#define DEVICE_ID 1

HardwareSerial RS485(2);

int ambient = 0;
int laserHit = 0;
bool running = false;

//sensor
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

//json send helpelrs
void sendEvent(const char* event, int value = 0) {
  StaticJsonDocument<128> doc;

  doc["id"] = DEVICE_ID;
  doc["event"] = event;
  doc["value"] = value;

  serializeJson(doc, RS485);
  RS485.print("\n");
}

//calibratee
void calibrate() {

  sendEvent("calibrating", 1);

  digitalWrite(LASER_PIN, LOW);
  delay(1000);
  ambient = averageRead(50);

  digitalWrite(LASER_PIN, HIGH);
  delay(1500);
  laserHit = averageRead(50);

  digitalWrite(LASER_PIN, LOW);

  StaticJsonDocument<128> doc;
  doc["id"] = DEVICE_ID;
  doc["event"] = "calibrate_done";
  doc["ambient"] = ambient;
  doc["laser"] = laserHit;

  serializeJson(doc, RS485);
  RS485.print("\n");
}

bool beam() {
  int value = averageRead(5);

  int deviation = abs(value - laserHit);
  int tolerance = abs(laserHit - ambient) * 0.4;

  return !(deviation >= tolerance);
}


void setup() {
  Serial.begin(115200);
  RS485.begin(115200, SERIAL_8N1, RXD2, TXD2);

  pinMode(LASER_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LASER_PIN, LOW);
}


void loop() {

  //input parsing
  if (RS485.available()) {

    String line = RS485.readStringUntil('\n');
    line.trim();

    StaticJsonDocument<128> doc;
    DeserializationError err = deserializeJson(doc, line);

    if (!err) {

      int id = doc["id"] | 0;
      const char* cmd = doc["cmd"];

      if (id == DEVICE_ID && cmd != nullptr) {

        if (strcmp(cmd, "calibrate_laser") == 0) {
          calibrate();
          running = false;
        }

        if (strcmp(cmd, "start_laser") == 0) {
          running = true;
          sendEvent("running", 1);
        }

        if (strcmp(cmd, "stop_laser") == 0) {
          running = false;
          digitalWrite(LASER_PIN, LOW);
          sendEvent("running", 0);
        }
      }
    }
  }

  //loop
  if (!running) {
    digitalWrite(LASER_PIN, LOW);
    delay(50);
    return;
  }

  bool ok = beam();

  digitalWrite(LASER_PIN, HIGH);
  digitalWrite(LED_BUILTIN, ok ? HIGH : LOW);

  sendEvent("beam", ok ? 1 : 0);

  delay(100);
}