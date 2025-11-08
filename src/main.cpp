#include "main.hpp"
#include "stm32g4xx_hal_fdcan.h"
#include <cstdint>

uint32_t loopTime;
CanFrame SendFrame;
CanFrame SendFrameBig;

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

    // intel format
    SendFrame.canId  = 0x0F0;
    SendFrame.canDlc = 8;
    
    SendFrame.SetSigned(20, 14, 4);
    SendFrame.SetUnsigned(16, 22, 4);
    SendFrame.SetFloat(-2, 32, 32);

    if (SendFrame.data[0] == 255)
      SendFrame.data[0] = 0;
    else 
      ++SendFrame.data[0];

    //Serial.println(loopTime);

    can0.sendFrame(&SendFrame);
    // end intel format sendFrame


    // motorola format
    SendFrameBig.canId  = 0x7EF;
    SendFrameBig.canDlc = 8;
    SendFrameBig.SetSigned(-20, 22, 4, Motorola);
    SendFrameBig.SetUnsigned(16, 30, 4, Motorola);
    SendFrameBig.SetFloat(-2.32123, 56, 32, Motorola);

    if (SendFrameBig.data[0] == 255)
      SendFrameBig.data[0] = 0;
    else 
      ++SendFrameBig.data[0];

    can0.sendFrame(&SendFrameBig);

    CanFrame RecvFrame; 
    if (!can0.inbox.empty()) {
      RecvFrame.clear();
      can0.inbox.pop(RecvFrame);

      if (RecvFrame.canId == 0x101) {
        // Little Endian
        Serial.print("-------- 0x"); 
        Serial.print(RecvFrame.canId, HEX);
        Serial.println(" --------");

        Serial.print("Auto Inc : ");
        // little endian frame
        Serial.println(RecvFrame.GetUnsigned(0, 8, Intel));
        Serial.print("Signed   : ");
        Serial.println(RecvFrame.GetSigned(14, 4, Intel));
        Serial.print("Unsigned : ");
        Serial.println(RecvFrame.GetUnsigned(22, 4, Intel));
        Serial.print("Float    : ");
        Serial.println(RecvFrame.GetFloat(32, 32, Intel));
        Serial.println();
      }
      else if (RecvFrame.canId == 0x7EE) {
        // Big endian
        Serial.print("-------- 0x"); 
        Serial.print(RecvFrame.canId, HEX);
        Serial.println(" --------");

        Serial.print("Auto Inc : ");
        // big endian frame
        Serial.println(RecvFrame.GetUnsigned(0, 8, Motorola));
        Serial.print("Signed   : ");
        Serial.println(RecvFrame.GetSigned(22, 4, Motorola));
        Serial.print("Unsigned : ");
        Serial.println(RecvFrame.GetUnsigned(30, 4, Motorola));
        Serial.print("Float    : ");
        Serial.println(RecvFrame.GetFloat(56, 32, Motorola));
        Serial.println();

      }
    } 


    loopTime = millis();
  }
}
