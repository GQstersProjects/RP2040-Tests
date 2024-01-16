#include <Arduino.h>
#include "MotorController_DRV8833.h"


// Define the control inputs
#define MOT_A1_PIN 1   // Motor A, Input 1   (IN4)
#define MOT_A2_PIN 3   // Motor A, Input 2   (IN3)
#define MOT_B1_PIN 12   // Motor B, Input 1   (IN2)
#define MOT_B2_PIN 15   // Motor B, Input 2   (IN1)

// #define SLEEP_PIN  23   // DRV8833 sleep pin   (EEP)


MotorController_DRV8833 motorController;


void setup(void)
{
  motorController.makeMotorA(MOT_A1_PIN, MOT_A2_PIN);   // Connect motor A pin
  motorController.makeMotorB(MOT_B1_PIN, MOT_B2_PIN);   // Connect motor B pin
  // motorController.setSleepPin(SLEEP_PIN);               // Set the sleep pin

  // Wake up the DRV8833
  // motorController.setSleep(false);
}

void loop(void)
{

for (int counter = 0; counter < 3; counter++) {
  motorController.setSpeedA(true, 255);  // Run motor A forward at full speed
  motorController.setSpeedB(true, 255); // Run motor B backward at half speed
  delay(1000); // Wait for 1 second
  
  motorController.setSpeedA(false, 255);  // Run motor A forward at full speed
  motorController.setSpeedB(false, 255); // Run motor B backward at half speed
  delay(1000); // Wait for 1 second

}

}