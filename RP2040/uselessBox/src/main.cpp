#include <Arduino.h>
#include <Servo.h>

// Define servo pin (adjust as needed)
const int servoPin = 14;

// Create servo object
Servo myservo;

void setup() {
  // Start serial communication for prints
  Serial.begin(115200);

  // Attach servo to pin
  myservo.attach(servoPin);
}

void loop() {
  // Move servo to 0 degrees
  myservo.write(45);
  Serial.println("Servo moved to 45 degrees");
  delay(1000);

  // Move servo to 90 degrees
  myservo.write(90);
  Serial.println("Servo moved to 90 degrees");
  delay(1000);

  // Move servo to 180 degrees
  myservo.write(135);
  Serial.println("Servo moved to 135 degrees");
  delay(1000);
}



// const int LED_PIN = 14;

// // the setup routine runs once when you press reset:
// void setup() {
//   // initialize the digital pin as an output.
//   pinMode(LED_PIN, OUTPUT);
// }


// // the loop routine runs over and over again forever:
// void loop() {
//   digitalWrite(LED_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)
//   delay(1000);               // wait for a second
//   digitalWrite(LED_PIN, LOW);    // turn the LED off by making the voltage LOW
//   delay(1000);               // wait for a second
// }



