#include "main.hpp"
#include "stm32g4xx_hal_fdcan.h"
#include <cstdint>

uint32_t loopTime;

FDCanChannel can0(HwCanChannel::CH1, Bitrate::b500000, Bitrate::b2000000); 

uint8_t sendData[64];

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

  for (uint8_t i = 0; i < 64; i++) {
    sendData[i] = 0;
  }

  loopTime = millis();
  can0.start();
}

void loop() {
  if (millis() - loopTime >= 500) {

    if (sendData[0] == 255)
      sendData[0] = 0;
    else 
      ++sendData[0];

    Serial.println(loopTime);
    can0.sendFrame(0x701, 8, sendData);

    loopTime = millis();
  }
}
