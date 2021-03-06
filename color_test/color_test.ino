//based on tcs34725.ino

#include <Wire.h>
#include "Adafruit_TCS34725.h"

#include "FastLED.h"

/* Example code for the Adafruit TCS34725 breakout library */

/* Connect SCL    to analog 5
   Connect SDA    to analog 4
   Connect VDD    to 3.3V DC
   Connect GROUND to common ground */
   
//same initialization as in standalone.ino
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_4X);

void setup(void) {
  Serial.begin(9600);
  
  if (tcs.begin()) {
    Serial.println("Found sensor");
  } else {
    Serial.println("No TCS34725 found ... check your connections");
    while (1);
  }
  
  // Now we're ready to get readings!
}

void loop(void) {
  //continously read color sensor every second
  delay(1000);
  uint8_t hue = readHue();
  //future work: determine if red or yellow based on known hues
  //hue2color();
}

//from standalone.ino
//Calculate the hue (color) detected
//Hypothesis is that this is more immune to changes in lighting than e.g. simply using red/green values.
//As written, there is room for optimization and improved accuracy--test first.
uint8_t readHue() {
  uint16_t tcs_r, tcs_g, tcs_b, tcs_c;  //red, green, blue, clear
  tcs.getRawData(&tcs_r,&tcs_g,&tcs_b,&tcs_c);
  CRGB tcs_rgb; //object from FastLED
  /*
  tcs_rgb.red = highByte(tcs_r);
  tcs_rgb.green = highByte(tcs_g);
  tcs_rgb.blue = highByte(tcs_b);*/
  //scale to 8-bit (only need relative precision for hue;
  // ignore saturation and value)
  Serial.println("Color sensor readings:");
  Serial.print("R:\t");
  Serial.println(tcs_r);
  Serial.print("G:\t");
  Serial.println(tcs_g);
  Serial.print("B:\t");
  Serial.println(tcs_b);
  while(max(max(tcs_r,tcs_g),tcs_b) > 255) {
    tcs_r >>= 1;
    tcs_g >>= 1;
    tcs_b >>= 1;
  }
  Serial.println("Color sensor (scaled to 8 bit):");
  Serial.print("R:\t");
  Serial.println(tcs_rgb.r = tcs_r);
  Serial.print("G:\t");
  Serial.println(tcs_rgb.g = tcs_g);
  Serial.print("B:\t");
  Serial.println(tcs_rgb.b = tcs_b);
  CHSV tcs_hsv = rgb2hsv_approximate(tcs_rgb); //convert CRGB object to CHSV
  Serial.print("Hue (8-bit):\t");
  return tcs_hsv.hue;
}

