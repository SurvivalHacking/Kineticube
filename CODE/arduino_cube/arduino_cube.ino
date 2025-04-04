// Modified 20/01/2025 by Davide Gatti (www.survivalhacking.it) 
// Added manual calibration via pushbutton
// Added 4 LED for show calibration status

// Press  button to start calibration, press again to store calibration point (repeat the fo three edges and one angle
// If press button during power-up, forget current calibration


#include "arduino_cube.h"
#include <Wire.h>
#include <EEPROM.h>

void setup() {
  Serial.begin(115200);

  TCCR1A = 0b00000001;
  TCCR1B = 0b00001010;
  TCCR2B = 0b00000010;
  TCCR2A = 0b00000011;

  pinMode(DIR_1, OUTPUT);
  pinMode(DIR_2, OUTPUT);
  pinMode(DIR_3, OUTPUT);
  pinMode(BRAKE, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(CALIBRATE, INPUT_PULLUP);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(LED_4, OUTPUT);

  Motor1_control(0);
  Motor2_control(0);
  Motor3_control(0);
  if  (!digitalRead(CALIBRATE)) { // if press button during power on reset calibration
    save();
    beep();
    beep();
    beep();
  }
  
  EEPROM.get(0, offsets);
  if (offsets.ID1 == 99 && offsets.ID2 == 99 && offsets.ID3 == 99 && offsets.ID4 == 99) {
    calibrated = true;
  }else{
    calibrated = false;
  }
  
  Update_calibrated_LED();

  delay(3000);
  beep();
  angle_setup();
}

void loop() {

  currentT = millis();

  if (currentT - previousT_1 >= loop_time) {
    Tuning();  // derinimui
    angle_calc();
    
    if (balancing_point == 1) {
      angleX -= offsets.X1;
      angleY -= offsets.Y1;
      if (abs(angleX) > 8 || abs(angleY) > 8) vertical = false;
    } else if (balancing_point == 2) {
      angleX -= offsets.X2;
      angleY -= offsets.Y2;
      if (abs(angleY) > 6) vertical = false;
    } else if (balancing_point == 3) {
      angleX -= offsets.X3;
      angleY -= offsets.Y3;
      if (abs(angleY) > 6) vertical = false;
    } else if (balancing_point == 4) {
      angleX -= offsets.X4;
      angleY -= offsets.Y4;
      if (abs(angleX) > 6) vertical = false;
    }
 
    if (vertical && calibrated && !calibrating) {    
      digitalWrite(BRAKE, HIGH);
      gyroZ = GyZ / 131.0; // Convert to deg/s
      gyroY = GyY / 131.0; // Convert to deg/s

      gyroYfilt = alpha * gyroY + (1 - alpha) * gyroYfilt;
      gyroZfilt = alpha * gyroZ + (1 - alpha) * gyroZfilt;
      
      int pwm_X = constrain(pGain * angleX + iGain * gyroZfilt + sGain * motor_speed_pwmX, -255, 255); // LQR
      int pwm_Y = constrain(pGain * angleY + iGain * gyroYfilt + sGain * motor_speed_pwmY, -255, 255); // LQR
      motor_speed_pwmX += pwm_X; 
      motor_speed_pwmY += pwm_Y;
      
      if (balancing_point == 1) {
        XY_to_threeWay(-pwm_X, -pwm_Y);
      } else if (balancing_point == 2) {
        Motor1_control(-pwm_Y);
      } else if (balancing_point == 3) {
        Motor2_control(pwm_Y);
      } else if (balancing_point == 4) {
        Motor3_control(-pwm_X);
      }
    } else {
      balancing_point = 0;
      XY_to_threeWay(0, 0);
      digitalWrite(BRAKE, LOW);
      motor_speed_pwmX = 0;
      motor_speed_pwmY = 0;
    }
    previousT_1 = currentT;
  }
  
  if (currentT - previousT_2 >= 2000) {    
    battVoltage((double)analogRead(VBAT) / bat_divider); 
    if (!calibrated && !calibrating) {
      Serial.println("first you need to calibrate the balancing points...");
    }
    previousT_2 = currentT;
  }
}

void Update_calibrated_LED() {
  EEPROM.get(0, offsets);
  digitalWrite(LED_1, LOW);
  digitalWrite(LED_2, LOW);
  digitalWrite(LED_3, LOW);
  digitalWrite(LED_4, LOW);
  if (offsets.ID1 == 99) {
    digitalWrite(LED_1, HIGH);
  }
  if (offsets.ID2 == 99) {
    digitalWrite(LED_2, HIGH);
  }
  if (offsets.ID3 == 99) {
    digitalWrite(LED_3, HIGH);
  }
  if (offsets.ID4 == 99) {
    digitalWrite(LED_4, HIGH);
  }
}

