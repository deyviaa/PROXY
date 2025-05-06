#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <u8g2lib.h>
#include <ESP32Servo.h>


// VCNL4200 proximity sensor address
#define VCNL4200_ADDR 0x51

// LED pins
#define RED_LED D2
#define YELLOW_LED D3
#define BLUE_LED D4

// LED prox thresholds
#define THRESH_BLUE 10
#define THRESH_YELLOW 35
#define THRESH_RED 100

// OLED pins
#define OLED_CS D10
#define OLED_DC D9
#define OLED_RST D8

// OLED icon draw
void drawHazardIcon(U8G2 &u8g2) {
  // triangle vertices
  uint8_t x0 = 64, y0 = 20;  // top
  uint8_t x1 = 44, y1 = 50;  // bottom left
  uint8_t x2 = 84, y2 = 50;  // bottom right

  // draw hollow triangle
  u8g2.drawLine(x0, y0, x1, y1);
  u8g2.drawLine(x1, y1, x2, y2);
  u8g2.drawLine(x2, y2, x0, y0);

  // draw exclamation mark inside
  u8g2.setFont(u8g2_font_helvB14_tr);
  u8g2.drawStr(62, 45, "!");
  
}


// OLED U8g2 instance
U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, OLED_CS, OLED_DC, OLED_RST);

// LED stages
enum stages {
  BLUE = 0,
  YELLOW = 1,
  RED = 2,
  CRITICAL = 3,
  CLEAR = 4
};

// setup function --- initalizing components
void setup() {
  Serial.begin(115200);
  Wire.begin();  // custom I2C pins

  // start SPI OLED
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0,10,"OLED Ready");
  u8g2.sendBuffer();

  // setup LEDs
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

// Function to read proximity data from the VCNL4200 sensor
// Returns the proximity value as a 16-bit unsigned integer
uint16_t readProximity() {
  Wire.beginTransmission(VCNL4200_ADDR);
  Wire.write(0x08);  // proximity data register
  Wire.endTransmission(false);  // repeated start

  Wire.requestFrom(VCNL4200_ADDR, 2);
  uint8_t low = Wire.read();
  uint8_t high = Wire.read();

  return (high << 8) | low;
}

void SET_OLED_DISPLAY(uint8_t stage, uint16_t prx) {
  
  switch (stage) {
    case CLEAR:
    u8g2.clearBuffer();
    u8g2.drawFrame(0, 0, 128, 64);
    u8g2.setFont(u8g2_font_helvB14_tr);
    u8g2.setCursor(20,30);
    u8g2.print("GOOD TO");
    u8g2.setCursor(38,50);
    u8g2.print("GO :D");
    u8g2.sendBuffer();
      break;
    case BLUE:
    u8g2.clearBuffer();
    u8g2.drawFrame(0, 0, 128, 64);
    u8g2.setFont(u8g2_font_freedoomr10_tu);
    u8g2.setCursor(32,30);
    u8g2.print("OBSTACLE");
    u8g2.setCursor(32,50);
    u8g2.print("DETECTED");
    u8g2.sendBuffer();
      break;
    case YELLOW:
    u8g2.clearBuffer();
    u8g2.drawFrame(0, 0, 128, 64);
    u8g2.setFont(u8g2_font_helvB14_tr);
    u8g2.setCursor(5,38);
    u8g2.print("!! CAUTION !!");
    u8g2.sendBuffer();
      break;
    case RED:
    u8g2.clearBuffer();
    u8g2.drawFrame(0, 0, 128, 64);
    u8g2.setFont(u8g2_font_helvB14_tr);
    u8g2.setCursor(13,30);
    u8g2.print("COLLISION");
    u8g2.setCursor(18,50);
    u8g2.print("IMMINENT");
    u8g2.sendBuffer();
   
      break;
    case CRITICAL:
    u8g2.clearBuffer();
    u8g2.drawFrame(0, 0, 128, 64);
    u8g2.setFont(u8g2_font_helvB14_tr);
    u8g2.setCursor(3,20);
    u8g2.print("WATCH OUT!");
    u8g2.setCursor(20,40);
    drawHazardIcon(u8g2);
    u8g2.sendBuffer();
    delay(100);
    u8g2.clearBuffer();
    u8g2.drawFrame(0, 0, 128, 64);
    u8g2.setFont(u8g2_font_helvB14_tr);
    u8g2.setCursor(3,20);
    u8g2.print("");
    u8g2.setCursor(20,40);
    drawHazardIcon(u8g2);
    u8g2.sendBuffer();
      break;
  }


}
// Function to set the LED stage
// 0 = clear, 1 = blue, 2 = yellow, 3 = red, 4 = critical
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
  
  uint8_t STAGE = CLEAR;
  
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
  
  Serial.print("Proximity: ");
  delay(5);
  Serial.println(prox);
  delay(5);

  SET_STAGE(STAGE);
  SET_OLED_DISPLAY(STAGE, prox);

  delay(100);
  }