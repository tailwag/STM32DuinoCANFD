/*  -------------------------------------------------  *
 *  -- This sketch takes in values over canfd and  --  *
 *  -- and used them to set a pwm output. Then we  --  *
 *  -- measure the pwm output freq and dc with the --  *
 *  -- pwm input, then send them back over canfd.  --  *
 *  -------------------------------------------------  */ 

#include <Arduino.h>
#include "STM32DuinoPWM.hpp"
#include "STM32DuinoCANFD.hpp"

FDCanChannel can0(CH1, b500000, b2000000);

CanFrame SendFrame;
CanFrame RecvFrame;

OutputPWM pwmOut(A0);
InputPWM   pwmIn(A5, LOWFREQ);

uint32_t loopTime = 0;

float outputFrequency = 100;
float outputDutyCycle = 50;

float inputFrequency;
float inputDutyCycle;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting up...");

  Serial.print("System Clock Frequency    : ");
  Serial.print(HAL_RCC_GetSysClockFreq() / 1000000);
  Serial.println("MHz");

  Serial.print("Starting CAN-FD Interface : ");
  can0.begin();
  Serial.println("Done!");

  Serial.print("Starting PWM Output       : ");
  pwmOut.begin();
  Serial.println("Done!");

  Serial.print("Starting PWM Input        : ");
  pwmIn.begin();
  Serial.println("Done!");

  loopTime = millis();
}

void loop() {
  if (millis() - loopTime >= 1000) {
    // check the inbox for the frame
    if (!can0.inbox.empty()) {
      // put the latest frame into RecvFrame
      can0.inbox.pop(RecvFrame);

      // make sure we're getting the right frame
      if (RecvFrame.canId == 0x100) {
        // get float signals out of frame
        outputFrequency = RecvFrame.GetFloat(0, 32); 
        outputDutyCycle = RecvFrame.GetFloat(32, 32); 

        // set the output pwm to the calues we received over can
        pwmOut.setFrequency(outputFrequency); 
        pwmOut.setDutyCycle(outputDutyCycle); 
      }
    }

    // measure frequency and duty cycle from input pwm 
    inputFrequency = pwmIn.getFrequency();
    inputDutyCycle = pwmIn.getDutyCycle();

    // put our measured values into a new canframe
    SendFrame.clear();
    SendFrame.canId  = 0x200; 
    SendFrame.canDlc = 8;
    SendFrame.SetFloat(inputFrequency, 0, 32);
    SendFrame.SetFloat(inputDutyCycle, 32, 32);

    // send the frame
    can0.sendFrame(&SendFrame);

    loopTime = millis();
  }
}


