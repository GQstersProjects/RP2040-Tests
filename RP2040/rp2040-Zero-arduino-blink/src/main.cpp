/*
* An example scetch to show how to loop through the colors of the RP2040Zero onboard WS2812 LED 
*/

#include <Adafruit_NeoPixel.h>

// Define pin for LED and number of LEDs (1 for onboard)
#define LED_PIN       16        // WS2812 LED strand is connected to GPIO16 on the RP2040 Zero
#define NUM_LEDS      1

Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);  // Initialize serial communication
  pixels.begin();       // Initialize NeoPixel library
}

void loop() {
  // Loop through colors with serial print output
  for (int color = 0; color < 5; color++) {
    switch (color) {
      case 0:
        pixels.setPixelColor(0, pixels.Color(255, 0, 0));  // Red
        Serial.println("Red");
        break;
      case 1:
        pixels.setPixelColor(0, pixels.Color(0, 255, 0));  // Green
        Serial.println("Green");
        break;
      case 2:
        pixels.setPixelColor(0, pixels.Color(0, 0, 255));  // Blue
        Serial.println("Blue");
        break;
      case 3:
        pixels.setPixelColor(0, pixels.Color(255, 255, 0));  // Yellow
        Serial.println("Yellow");
        break;
      case 4:
        pixels.setPixelColor(0, pixels.Color(0, 255, 255));  // Cyan
        Serial.println("Cyan");
        break;
    }
    pixels.show();     // Update LED strip
    delay(1000);       // Wait for 1 second
  }
}
