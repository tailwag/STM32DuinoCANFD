#include "main.h"
#include "stm32g4xx_hal_fdcan.h"

uint32_t loopTime;

FDCAN_TxHeaderTypeDef TxHeader;
uint8_t TxData[8] = {1, 2, 3, 4, 5, 6, 7};

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

  __HAL_RCC_FDCAN_CLK_ENABLE();

  MX_FDCAN1_Init();
  MX_FDCAN1_Filter_Init();
  HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);

  TxHeader.Identifier = 0x123; 
  TxHeader.IdType = FDCAN_STANDARD_ID;
  TxHeader.TxFrameType = FDCAN_DATA_FRAME;
  TxHeader.DataLength = FDCAN_DLC_BYTES_8;
  TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  TxHeader.BitRateSwitch = FDCAN_BRS_ON;
  TxHeader.FDFormat = FDCAN_FD_CAN;
  TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
  TxHeader.MessageMarker = 0;

  HAL_FDCAN_Start(&hfdcan1);

  loopTime = millis();
}

void loop() {
  if (millis() - loopTime >= 1000) {
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, TxData);
    
    if (TxData[7] == 0xFF)
      TxData[7] = 0x00;
    else
     TxData[7]++;


    FDCAN_RxHeaderTypeDef rxHeader;
    uint8_t rxData[64];
    
    if (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) > 0) {
        if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK) {
            // Process the received frame
            Serial.println();
            Serial.print("Received ID: 0x");
            Serial.println(rxHeader.Identifier, HEX);
            Serial.print("DLC        : ");
            Serial.println(rxHeader.DataLength);  // DLC to byte count
            Serial.print("Data       : ");
            for (int i = 0; i < dlcToLen(rxHeader.DataLength); i++) {
                if (!(i % 8) && i != 0) {         // if i is divisible by 8 and not 0
                  Serial.println();
                  Serial.print("             ");
                }
              
                Serial.printf("%02X ", rxData[i]);
            }
            Serial.println();
        }
    }


    loopTime = millis();
  }
}
