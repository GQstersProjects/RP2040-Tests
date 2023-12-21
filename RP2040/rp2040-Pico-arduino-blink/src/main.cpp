#include <Arduino.h>

// Initialize serial communication in setup()
void setup() {
  Serial.begin(115200); // Start serial communication at 115200 baud rate
  pinMode(LED_BUILTIN, OUTPUT); // Initialize LED pin as output
  Serial.println("Setup complete"); // Print a message to indicate setup is done
}

void loop() {
  Serial.println("Turning LED on"); // Print a message before turning LED on
  digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED on
  delay(1000);                     // Wait for a second

  Serial.println("Turning LED off"); // Print a message before turning LED off
  digitalWrite(LED_BUILTIN, LOW);   // Turn the LED off
  delay(1000);                     // Wait for a second
}


