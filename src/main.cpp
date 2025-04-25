#include <Arduino.h>
#include <Wire.h>

#define VCNL4200_ADDR 0x51
#define RED_LED D2
#define YELLOW_LED D3
#define BLUE_LED D4


#define THRESH_BLUE 10
#define THRESH_YELLOW 35
#define THRESH_RED 100


enum stages {
  BLUE = 0,
  YELLOW = 1,
  RED = 2,
  CRITICAL = 3,
  CLEAR = 4
};

void setup() {
  Serial.begin(115200);
  Wire.begin();  // custom I2C pins

  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  // config: write PS_CONF1/2 register (0x03)
  Wire.beginTransmission(VCNL4200_ADDR);
  Wire.write(0x03); // PS_CONF1 integration time bit 0-3
  Wire.write(0x00);  // example config <-- change this to 
  Wire.write(0x00);
  Wire.endTransmission();

  delay(100);
}

uint16_t readProximity() {
  Wire.beginTransmission(VCNL4200_ADDR);
  Wire.write(0x08);  // proximity data register
  Wire.endTransmission(false);  // repeated start

  Wire.requestFrom(VCNL4200_ADDR, 2);
  uint8_t low = Wire.read();
  uint8_t high = Wire.read();

  return (high << 8) | low;
}

void SET_STAGE(uint8_t stage) {
  digitalWrite(BLUE_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(RED_LED, LOW);

  switch (stage) {
    case CLEAR:
      digitalWrite(BLUE_LED, LOW);
      digitalWrite(YELLOW_LED, LOW);
      digitalWrite(RED_LED, LOW);
      break;
    case BLUE:
      digitalWrite(BLUE_LED, HIGH);
      break;
    case YELLOW:
      digitalWrite(YELLOW_LED, HIGH);
      break;
    case RED:
      digitalWrite(RED_LED, HIGH);
      break;
    case CRITICAL:
      digitalWrite(RED_LED, HIGH);
      delay(100);
      digitalWrite(RED_LED, LOW);
      delay(10);
      break;
  }
}

void loop() {
  uint16_t prox = readProximity();
  if (prox > 0xFFFF) {
    Serial.println("Error reading proximity sensor");
    return;
  }

  if (prox == 0) {
    Serial.println("Proximity sensor not detected or no object in range");
    return;
  }
  if (prox > 5) {
  Serial.print("Proximity: ");
  delay(5);
  Serial.println(prox);
  delay(5);
}

  uint STAGE = CLEAR;

  if (prox < 5)
    STAGE = CLEAR;
  else if (prox < THRESH_BLUE)
    STAGE = BLUE;
  else if (prox < THRESH_YELLOW)
    STAGE = YELLOW;
  else if (prox < THRESH_RED)
    STAGE = RED;
  else if (prox >= THRESH_RED)
    STAGE = CRITICAL;

    SET_STAGE(STAGE);

  delay(200);
  }