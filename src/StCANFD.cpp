#include "StCANFD.hpp"
#include "stm32_def.h"
#include "stm32g4xx_hal_fdcan.h"
#include "stm32g4xx_hal_rcc.h"

// look up table for canfd dlc
uint8_t DlcToLen(uint8_t dlcIn) {
  // sanitize input
  if (dlcIn < 0 || dlcIn > 15)
    return 0;

  uint8_t d[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8,
                    12, 16, 20, 24, 32, 48, 64};

  return d[dlcIn];
}

// enum Bitrate in StCANFD.hpp contains possible bitrates 
// this is used to index this array to get the timing values 
// FDCANScalers[Bitrate::b500000] returns a struct with 
// Prescaler=20, SyncJump=1, Segment1=13, Segment2=2 
// these values are used to initialize the HAL CAN hardware 
// and hit the desired bitrate. The values below were calculated
// assuming your clock is running at 160MHz. the g474re's default
// clock is 170MHz, which doesn't divide neatly into most CANFD 
// bitrates, so setting it to 160MHz is important.
FDCAN_ScalerStruct FDCANScalers[24] = {
  // prescaler, sync, seg1, seg2 
  {320, 1, 13, 2}, //  32,250 bps
  {300, 1, 13, 2}, //  33,333 bps
  {250, 1, 13, 2}, //  40,000 bps
  {200, 1, 13, 2}, //  50,000 bps
  {160, 1, 13, 2}, //  62,500 bps
  {125, 1, 13, 2}, //  80,000 bps
  {120, 1, 13, 2}, //  83,333 bps
  {100, 1, 13, 2}, // 100,000 bps
  {80,  1, 13, 2}, // 125,000 bps
  {50,  1, 17, 2}, // 160,000 bps
  {50,  1, 13, 2}, // 200,000 bps
  {40,  1, 13, 2}, // 250,000 bps
  {20,  1, 17, 2}, // 400,000 bps  - works
  {20,  1, 13, 2}, // 500,000 bps  - works
  {20,  1,  7, 2}, // 800,000 bps  - works
  {16,  1,  7, 2}, //1,000,000 bps - works
  {16,  1,  5, 2}, //1,250,000 bps - works
  {10,  1,  7, 2}, //1,600,000 bps - works
  { 8,  1,  7, 2}, //2,000,000 bps - works
  { 8,  1,  5, 2}, //2,500,000 bps - vector: no, peak: yes
  { 4,  1,  7, 2}, //4,000,000 bps - vector: no, peak: yes
  { 4,  1,  5, 2}, //5,000,000 bps - vector: no, peak: no
  { 3,  1,  6, 2}, //6,000,000 bps - use with caution, actually 5,925,926
  { 2,  1,  7, 2}, //8,000,000 bps - vector: no, peak: no
};

// constructor for FDCanChannel class
FDCanChannel::FDCanChannel(HwCanChannel chan, Bitrate baseRate, Bitrate dataRate) {
  // determine hardware channel
  if (chan == HwCanChannel::CH1) {
    Interface.Instance = FDCAN1;
  }
  else if (chan == HwCanChannel::CH2) {
    Interface.Instance = FDCAN2;
  }
  else {
    Error_Handler();
  }

  // setup static values
  Interface.Init.ClockDivider       = FDCAN_CLOCK_DIV1;   // CPU_Clock / Divider 
  Interface.Init.FrameFormat        = FDCAN_FRAME_FD_BRS; 
  Interface.Init.Mode               = FDCAN_MODE_NORMAL;
  Interface.Init.AutoRetransmission = ENABLE;
  Interface.Init.TransmitPause      = ENABLE;
  Interface.Init.ProtocolException  = DISABLE;


  // calculate clock sources from desired bitrates
  // retrieve scaler values using the index of the desired bitrate
  // bitrate = FDCAN_CLOCK / (Prescaler * (SyncSeg + TimeSeg1 + TimeSeg2))
  FDCAN_ScalerStruct BaseScalers = FDCANScalers[baseRate];
  FDCAN_ScalerStruct DataScalers = FDCANScalers[dataRate];

  // configuration of arbitration phase
  Interface.Init.NominalPrescaler     = BaseScalers.Prescaler;
  Interface.Init.NominalSyncJumpWidth = BaseScalers.SyncJump;
  Interface.Init.NominalTimeSeg1      = BaseScalers.Segment1;
  Interface.Init.NominalTimeSeg2      = BaseScalers.Segment2;

  // configuration of data phase
  Interface.Init.DataPrescaler        = DataScalers.Prescaler;
  Interface.Init.DataSyncJumpWidth    = DataScalers.SyncJump;
  Interface.Init.DataTimeSeg1         = DataScalers.Segment1;
  Interface.Init.DataTimeSeg2         = DataScalers.Segment2;

  // filter setup
  Interface.Init.StdFiltersNbr = 0;
  Interface.Init.ExtFiltersNbr = 0;
  Interface.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;

  // initialize interface
  if (HAL_FDCAN_Init(&Interface) != HAL_OK) {
    Error_Handler();
  }
}

void FDCanChannel::start(void) {
  __HAL_RCC_FDCAN_CLK_ENABLE();
  HAL_FDCAN_Start(&Interface);
}

void FDCanChannel::sendFrame(uint16_t canId, uint8_t canDlc, uint8_t * canData, bool BRS) {
  FDCAN_TxHeaderTypeDef TxHeader;
  uint8_t sanitizedDlc;

  if (canDlc < 0 || canDlc > 15) 
    sanitizedDlc = 0;
  else 
    sanitizedDlc = canDlc;

  // input sanitization hadled by DlcToLen
  uint8_t messageBytes = DlcToLen(sanitizedDlc);

  // pass only the bytes we need
  uint8_t trimmedDataArray[messageBytes];
  for (uint8_t i = 0; i < messageBytes; i++) {
    trimmedDataArray[i] = canData[i];
  }

  if (BRS)
    TxHeader.BitRateSwitch = FDCAN_BRS_ON;
  else
    TxHeader.BitRateSwitch = FDCAN_BRS_OFF;

  TxHeader.Identifier          = canId;
  TxHeader.IdType              = FDCAN_STANDARD_ID;
  TxHeader.TxFrameType         = FDCAN_DATA_FRAME;
  TxHeader.DataLength          = sanitizedDlc;
  TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE; 
  TxHeader.FDFormat            = FDCAN_FD_CAN;
  TxHeader.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
  TxHeader.MessageMarker       = 0;

  // diagnostic info for windows/linux compile difference issues
  #ifdef _DEBUG
  Serial.print("sizeof(FDCAN_TxHeaderTypeDef) = ");
  Serial.println(sizeof(FDCAN_TxHeaderTypeDef));
  Serial.print("offset of DataLength = "); 
  Serial.println((unsigned long)((uint8_t*)&TxHeader.DataLength - (uint8_t*)&TxHeader));
  #endif

  if (HAL_FDCAN_AddMessageToTxFifoQ(&Interface, &TxHeader, trimmedDataArray) != HAL_OK) {
    Error_Handler();
  }
}

// function to initialize the physical interface in the HAL 
// to be honest, I'm not sure where this is getting called
// it was generated by the STM32Cube project I used to develop
// this library, but I know nothing works if this isn't here
static uint32_t HAL_RCC_FDCAN_CLK_ENABLED = 0;
void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef * fdcanHandle) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  if (fdcanHandle->Instance == FDCAN1) {
    // initialize peripheral clocks 
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
    PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PCLK1;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
      Error_Handler();
    }

    // fdcan1 clock enable 
    HAL_RCC_FDCAN_CLK_ENABLED++;
    if (HAL_RCC_FDCAN_CLK_ENABLED == 1)
      __HAL_RCC_FDCAN_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();

    // FDCAN1 GPIO Config
    // PA11 --> FDCAN1 RX
    // PA12 --> FDCAN1 TX 
    GPIO_InitStruct.Pin = GPIO_PIN_11|GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; 
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
  else if (fdcanHandle->Instance == FDCAN2) {
    // initialize peripheral clocks 
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
    PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PCLK1;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
      Error_Handler();
    }

    // fdcan1 clock enable 
    HAL_RCC_FDCAN_CLK_ENABLED++;
    if (HAL_RCC_FDCAN_CLK_ENABLED == 1)
      __HAL_RCC_FDCAN_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();

    // FDCAN1 GPIO Config
    // PA11 --> FDCAN1 RX
    // PA12 --> FDCAN1 TX 
    GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP; 
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  }
}
void HAL_FDCAN_MspDeInit(FDCAN_HandleTypeDef * fdcanHandle) {
  if (fdcanHandle->Instance == FDCAN1) {
    HAL_RCC_FDCAN_CLK_ENABLED--;
    if(HAL_RCC_FDCAN_CLK_ENABLED == 0) 
      __HAL_RCC_FDCAN_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_11|GPIO_PIN_12);
  } 
  else if (fdcanHandle->Instance == FDCAN2) {
    HAL_RCC_FDCAN_CLK_ENABLED--;
    if(HAL_RCC_FDCAN_CLK_ENABLED == 0) 
      __HAL_RCC_FDCAN_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_12|GPIO_PIN_13);
  }
}

