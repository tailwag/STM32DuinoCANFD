#include "StCANFD.hpp"
#include <cstring>

// global channel definition
FDCAN_GlobalTypeDef * AvailableChannels[3] = { FDCAN1, FDCAN2, FDCAN3 };

extern "C" void FDCAN1_IT0_IRQHandler(void) {
  auto inst = FDCanChannel::getInstance(CH1);
  if (inst)
    HAL_FDCAN_IRQHandler(inst->getHandle());
}

extern "C" void FDCAN2_IT0_IRQHandler(void) {
  auto inst = FDCanChannel::getInstance(CH2);
  if (inst)
    HAL_FDCAN_IRQHandler(inst->getHandle());
}

extern "C" void FDCAN3_IT0_IRQHandler(void) {
  auto inst = FDCanChannel::getInstance(CH3);
  if (inst)
    HAL_FDCAN_IRQHandler(inst->getHandle());
}

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


/* --------------------------------------------------------------- *
 * -- METHOD DEFINITIONS: CanFrame class                        -- *
 * --------------------------------------------------------------- */
CanFrame::CanFrame() {
  canId = 0;
  canDlc = 0;
  
  for (uint8_t i = 0; i < 64; i++) {
    data[i] = 0;
  }

  brs = true;
}

void CanFrame::clear() {
  memset(data, 0, sizeof(data));
}

uint32_t CanFrame::GetUnsigned(uint8_t startByte, uint8_t startBit, uint8_t length, Endian order) {
  // value that eventually gets returned
  uint32_t retVal = 0;

  // calculate absolute start bit position
  uint16_t absStart = startByte * 8 + startBit; 

  for (uint8_t i = 0; i < length; i++) {
    // calculate target bit index
    uint16_t bitPos = absStart + i; 

    uint8_t byteIndex = bitPos / 8; // integer division, no remainder
    uint8_t bitIndex  = bitPos % 8; // get remainder

    // isolate individual bit
    uint8_t bit = (data[byteIndex] >> bitIndex) & 1u;

    // put isolated bit into return value
    retVal |= (static_cast<uint32_t>(bit) << i);
  }

  return retVal;
}

int32_t CanFrame::GetSigned(uint8_t startByte, uint8_t startBit, uint8_t length, Endian order) {
  // get raw bits in unsigned value
  uint32_t rawValue = GetUnsigned(startByte, startBit, length, order);

  // shift the sign bit down to determine if value is negative
  bool isNeg = rawValue >> (length - 1) & 1u; 

  if (isNeg) {
    // generate bit mask for later or
    // 00001010 | 11111000 = 11111010
    int32_t bitMask = -1 << (length - 1);
    return rawValue | bitMask; 
  }

  return static_cast<int32_t>(rawValue);
}

float CanFrame::GetFloat(uint8_t startByte, uint8_t startBit, uint8_t length, Endian order) {
  uint32_t rawValue = GetUnsigned(startByte, startBit, length);
  float retVal = * ( float * ) &rawValue;
  return retVal;
}

void CanFrame::SetUnsigned(uint32_t value, uint8_t startByte, uint8_t startBit, uint8_t length, Endian order) {
  // bit manipulation to get the max unsigned value 
  // -1 of a signed value is all ones, shift left to introduce 0s 
  // invert to change all 0s to 1s and vise versa
  uint32_t upper = static_cast<uint32_t>(~(-1 << length));

  value = (value > upper) ? upper : value; // make sure value doesn't exceed limit
  value = (value < 0)     ? 0     : value; // if this evals true something has gone horribly wrong
                                           
  // calculate the absolute bit start position
  uint16_t absStart = startByte * 8 + startBit;

  for (uint8_t i = 0; i < length; i++) {
    // calculate target bit index
    uint16_t bitPos = absStart + i;

    uint8_t byteIndex = bitPos / 8; // integer division, no remainder
    uint8_t bitIndex  = bitPos % 8; // get remainder

    // bring target bit down into position 0, then mask it out
    uint8_t bit = (value >> i) & 1u;

    // set or clear bit 
    if (bit)
      data[byteIndex] |=  (1u << bitIndex);
    else 
      data[byteIndex] &= ~(1u << bitIndex);
  }
}

// convert signed bits to unsigned int value and use SetUnsigned to set value
// we have to manually mover the sign bit, because can signed ints are variable length
void CanFrame::SetSigned(int32_t value, uint8_t startByte, uint8_t startBit, uint8_t length, Endian order) {
  // bit manipulation to get upper and lower limits
  int32_t lower = -1 << (length - 1);
  int32_t upper = ~lower;

  // if value falls outside of range, set to max or min 
  value = (value > upper) ? upper : value;
  value = (value < lower) ? lower : value;

  // get & bitmask for final value 
  // we can reuse upper here 
  uint32_t bitmask = ((uint32_t)upper << 1) + 1u;

  // apply the bitmask to the final value 
  value &= bitmask; 

  // set the value 
  SetUnsigned(value, startByte, startBit, length, order);
}

// convert float value bits to unsigned int value and use SetUnsigned to set value
// this is convenient, because can floats are always 32 bits
void CanFrame::SetFloat(float value, uint8_t startByte, uint8_t startBit, uint8_t length, Endian order) {
  uint32_t longVal = * ( uint32_t * ) &value;
  SetUnsigned(longVal, startByte, startBit, length, order);
}

/* --------------------------------------------------------------- *
 * -- METHOD DEFINITIONS: CanInbox class                        -- *
 * --------------------------------------------------------------- */
bool CanInbox::push(const FDCAN_RxHeaderTypeDef &rxHeader, const uint8_t *data) {
  // if next message is out of bounds, loop back around to start
  uint8_t next = (head + 1) % MAX_MESSAGES;

  // overwrite oldest if full
  if (next == tail)
    tail = (tail + 1) % MAX_MESSAGES; 

  // select oldest slot to write data into
  CanFrame &msg = buffer[head];

  msg.canId  = rxHeader.Identifier;
  msg.canDlc = rxHeader.DataLength; // >> 16 // HAL encodes DLC in bits 19:16
  msg.brs    = (rxHeader.BitRateSwitch == FDCAN_BRS_ON);

  memcpy(msg.data, data, DlcToLen(msg.canDlc));

  head = next; 
  return true;
}

bool CanInbox::pop(CanFrame &out) {
  if (head == tail)
    return false;

  out  = buffer[tail];
  tail = (tail + 1) % MAX_MESSAGES;
  return true;
}

/* --------------------------------------------------------------- *
 * -- METHOD DEFINITIONS: FDCanChannel class                    -- *
 * --------------------------------------------------------------- */
// constructor for FDCanChannel class
FDCanChannel::FDCanChannel(HwCanChannel chan, Bitrate baseRate, Bitrate dataRate) {
  ChannelID = chan;

  // store pointer to object in global array
  Instances[chan] = this;

  // get can interface handle 
  Interface.Instance = AvailableChannels[chan];

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
  Interface.Init.StdFiltersNbr    = 0;
  Interface.Init.ExtFiltersNbr    = 0;
  Interface.Init.TxFifoQueueMode  = FDCAN_TX_FIFO_OPERATION;

  // initialize interface
  if (HAL_FDCAN_Init(&Interface) != HAL_OK) {
    Error_Handler();
  }
}

FDCanChannel* FDCanChannel::Instances[3] = { nullptr, nullptr, nullptr };

void FDCanChannel::handleRxInterrupt() {
  FDCAN_RxHeaderTypeDef rxHeader; 
  uint8_t rxData[64];

  if (HAL_FDCAN_GetRxMessage(&Interface, FDCAN_RX_FIFO0, &rxHeader, rxData) != HAL_OK) {
    Error_Handler();
    return;
  }

  inbox.push(rxHeader, rxData);
  timeLastRecv = HAL_GetTick();
}

void FDCanChannel::start(void) {
  __HAL_RCC_FDCAN_CLK_ENABLE();
  HAL_FDCAN_Start(&Interface);
  HAL_FDCAN_ActivateNotification(&Interface, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);

  // Enable IRQ in NVIC
  if (Interface.Instance == FDCAN1)
    HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 1, 0), HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
  else if (Interface.Instance == FDCAN2)
    HAL_NVIC_SetPriority(FDCAN2_IT0_IRQn, 1, 0), HAL_NVIC_EnableIRQ(FDCAN2_IT0_IRQn);
  else if (Interface.Instance == FDCAN3)
    HAL_NVIC_SetPriority(FDCAN3_IT0_IRQn, 1, 0), HAL_NVIC_EnableIRQ(FDCAN3_IT0_IRQn);
}

// send frame function
void FDCanChannel::sendFrame(CanFrame * Frame) {
  FDCAN_TxHeaderTypeDef TxHeader;
  
  uint8_t canDlc = Frame->canDlc;
  uint16_t canId = Frame->canId;

  // set min and max values
  canId  = (canId  < 0)     ? 0     : canId;
  canId  = (canId  > 0x7FF) ? 0x7FF : canId;
  canDlc = (canDlc < 0)     ? 0     : canDlc;
  canDlc = (canDlc > 15)    ? 15    : canDlc;

  // input sanitization hadled by DlcToLen
  uint8_t messageBytes = DlcToLen(canDlc);

  // pass only the bytes we need
  uint8_t trimmedDataArray[messageBytes];
  for (uint8_t i = 0; i < messageBytes; i++) {
    trimmedDataArray[i] = Frame->data[i];
  }

  // construct message header
  if (Frame->brs)
    TxHeader.BitRateSwitch = FDCAN_BRS_ON;
  else
    TxHeader.BitRateSwitch = FDCAN_BRS_OFF;

  TxHeader.Identifier          = canId;
  TxHeader.IdType              = FDCAN_STANDARD_ID;
  TxHeader.TxFrameType         = FDCAN_DATA_FRAME;
  TxHeader.DataLength          = canDlc;
  TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE; 
  TxHeader.FDFormat            = FDCAN_FD_CAN;
  TxHeader.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
  TxHeader.MessageMarker       = 0;

  byte halOpStatus = HAL_FDCAN_AddMessageToTxFifoQ(&Interface, &TxHeader, trimmedDataArray);

  if (halOpStatus != HAL_OK) 
    Error_Handler();
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

// receive callback function
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
  if(!(RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE))
    return;

  for (int i = 0; i < 3; i++) {
    // get hardware channel ID for iterator from enum 
    HwCanChannel c = static_cast<HwCanChannel>(i);

    // FDCanChannel::getInstance(HwCanChannel) returns pointer to object
    // if an instance hasn't been initialized yet, it returns a nullptr
    if (FDCanChannel::getInstance(c) && hfdcan->Instance == AvailableChannels[i]) {
      // use the interupt handler for the specific channel
      FDCanChannel::getInstance(c)->handleRxInterrupt();
      break;
    }
  }
}
