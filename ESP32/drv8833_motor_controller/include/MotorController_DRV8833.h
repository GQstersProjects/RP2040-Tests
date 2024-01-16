#include <Arduino.h>

class MotorController_DRV8833 {
public:

  void makeMotorA(int pinA1, int pinA2) {
    this->pinA1 = pinA1;
    this->pinA2 = pinA2;
    // pinMode(pinA1, OUTPUT);
    // pinMode(pinA2, OUTPUT);
  }
  void makeMotorB(int pinB1, int pinB2) {
    this->pinB1 = pinB1;
    this->pinB2 = pinB2;
    pinMode(pinB1, OUTPUT);
    pinMode(pinB2, OUTPUT);
  }

  void setSleepPin(int sleepPin) {
    this->sleepPin = sleepPin;
    pinMode(sleepPin, OUTPUT);
  }

  void setSleep(bool sleep=true) {
    if (sleep) {
      digitalWrite(sleepPin, LOW);
    } else {
      digitalWrite(sleepPin, HIGH);
    }
  }

  void setSpeedA(bool clockwise=true, int pwm=0, int duration=50) {
    pwm = constrain(pwm, 0, 255);     // Ensure PWM values are within valid range (0-255)
    this->speedA = pwm;               // save the speed for later

    if (clockwise) {
      analogWrite(pinA1, pwm);
      analogWrite(pinA2, 0);
    } else {
      analogWrite(pinA1, 0);
      analogWrite(pinA2, pwm);
    }

    delay(duration);
  }

  void setSpeedB(bool clockwise=true, int pwm=0, int duration=50) {
    pwm = constrain(pwm, 0, 255);     // Ensure PWM values are within valid range (0-255)
    this->speedB = pwm;               // save the speed for later

    if (clockwise) {
      analogWrite(pinB1, pwm);
      analogWrite(pinB2, 0);
    } else {
      analogWrite(pinB1, 0);
      analogWrite(pinB2, pwm);
    }

    delay(duration);
  }


private:

  int pinA1;
  int pinA2;
  int speedA;
  int pinB1;
  int pinB2;
  int speedB;

  int sleepPin;
};
