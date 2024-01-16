#include <Arduino.h>
#include "MotorController_DRV8833.h"

// // put function declarations here:
// int myFunction(int, int);

// void setup() {
//   // put your setup code here, to run once:
//   int result = myFunction(2, 3);
// }

// void loop() {
//   // put your main code here, to run repeatedly:
// }

// // put function definitions here:
// int myFunction(int x, int y) {
//   return x + y;
// }





// Define the control inputs
#define MOT_A1_PIN 16   // Motor A, Input 1   (IN4)
#define MOT_A2_PIN 17   // Motor A, Input 2   (IN3)
#define MOT_B1_PIN 18   // Motor B, Input 1   (IN2)
#define MOT_B2_PIN 19   // Motor B, Input 2   (IN1)

#define SLEEP_PIN  23   // DRV8833 sleep pin   (EEP)



// void setMotorSpeeds(int pinA, int pinB, bool clockwise=true, int pmw=0, int duration=50) {
//   // Ensure PWM values are within valid range (0-255)
//   pmw = constrain(pmw, 0, 255);

//   // Set the PWM values for the specified pins
//   if (clockwise) {
//     analogWrite(pinA, pmw);
//     analogWrite(pinB, 0);
//   } else {
//     analogWrite(pinA, 0);
//     analogWrite(pinB, pmw);
//   }
//   delay(duration);
// }



MotorController_DRV8833 motorController;


void setup(void)
{
  motorController.makeMotorA(16, 17);   // Connect motor A to pins 8 and 9
  motorController.makeMotorB(18, 19);   // Connect motor B to pins 10 and 11
  motorController.setSleepPin(23);      // Set the sleep pin to 12



  // Initialize the serial UART at 115200 baud
  Serial.begin(115200);
}

void loop(void)
{

for (int counter = 0; counter < 3; counter++) {
  Serial.println("Forward ");
  motorController.setSpeedA(true, 255);  // Run motor A forward at full speed
  motorController.setSpeedB(true, 128); // Run motor B backward at half speed
  delay(1000); // Wait for 1 second
  
  Serial.println("Backward ");
  motorController.setSpeedA(false, 255);  // Run motor A forward at full speed
  motorController.setSpeedB(false, 128); // Run motor B backward at half speed
  delay(1000); // Wait for 1 second

}

// put the unit to sleep
Serial.println("Sleep ");
motorController.setSleep(true);

Serial.println("try to move");
motorController.setSpeedA(false, 100);  // Run motor A forward at full speed

delay(3000);

//wake up the unit
Serial.println("Wake up ");
motorController.setSleep(false);


}