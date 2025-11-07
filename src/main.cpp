#include "main.hpp"
#include "stm32g4xx_hal_fdcan.h"
#include <cstdint>

uint32_t loopTime;
CanFrame SendFrame;

FDCanChannel can0(HwCanChannel::CH1, Bitrate::b500000, Bitrate::b2000000); 

void setup() {
  HAL_Init();
  
  Serial.begin(115200);
  delay(200);
  Serial.println("Starting up...");

  SystemClock_Config();
  // confirm clock settings from myhal.h 
  Serial.print("SysClockFreq : ");
  Serial.println(HAL_RCC_GetSysClockFreq());
  Serial.print("HCLKFreq     : ");
  Serial.println(HAL_RCC_GetHCLKFreq());
  Serial.print("PCLK1Freq    : ");
  Serial.println(HAL_RCC_GetPCLK1Freq());
  Serial.print("PCLK2Freq    : ");
  Serial.println(HAL_RCC_GetPCLK2Freq());

  loopTime = millis();

  can0.start();

}

void loop() {
  if (millis() - loopTime >= 500) {

    SendFrame.canId  = 0x0F0;
    SendFrame.canDlc = 8;
    
    SendFrame.SetSigned(20, 1, 6, 4);
    SendFrame.SetUnsigned(16, 2, 6, 4);
    SendFrame.SetFloat(-2, 4, 0, 32);

    if (SendFrame.data[0] == 255)
      SendFrame.data[0] = 0;
    else 
      ++SendFrame.data[0];

    //Serial.println(loopTime);

    can0.sendFrame(&SendFrame);

    CanFrame RecvFrame; 
    if (!can0.inbox.empty()) {
      can0.inbox.pop(RecvFrame);

      Serial.print("Last Received : ");
      Serial.println(can0.lastRecv());
      Serial.print("ID : 0x");
      Serial.println(RecvFrame.canId, HEX);
      Serial.print("DLC : ");
      Serial.println(RecvFrame.canDlc);
      Serial.print("Value Signal : ");
      Serial.println(RecvFrame.GetUnsigned(1, 0, 8));
    } 


    loopTime = millis();
  }
}
