#include "StCANFD.hpp"

uint8_t DlcToLen(uint8_t dlcIn) {
  if (dlcIn < 0 || dlcIn > 15)
    return 0;

  uint8_t d[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8,
                    12, 16, 20, 24, 32, 48, 64};

  return d[dlcIn];
}

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
  {20,  1, 17, 2}, // 400,000 bps
  {20,  1, 13, 2}, // 500,000 bps
  {20,  1,  7, 2}, // 800,000 bps
  {16,  1,  7, 2}, //1,000,000 bps
  {16,  1,  5, 2}, //1,250,000 bps
  {10,  1,  7, 2}, //1,600,000 bps
  { 8,  1,  7, 2}, //2,000,000 bps
  { 8,  1,  5, 2}, //2,500,000 bps
  { 4,  1,  7, 2}, //4,000,000 bps
  { 4,  1,  5, 2}, //5,000,000 bps
  { 3,  1,  6, 2}, //6,000,000 bps - use with caution, actually 5,925,926
  { 2,  1,  7, 2}, //8,000,000 bps
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
  Interface.Init.ClockDivider       = FDCAN_CLOCK_DIV1;
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

void FDCanChannel::sendFrame(uint16_t canId, uint8_t canDlc, uint8_t * canData, bool BRS) {
  FDCAN_TxHeaderTypeDef TxHeader;

  uint8_t sanitizedDlc;
  uint8_t messageBytes = DlcToLen(sanitizedDlc);
  uint8_t trimmedDataArray[messageBytes];

  for (uint8_t i = 0; i < messageBytes; i++) {
    trimmedDataArray[i] = canData[i];
  }

  if (canDlc < 0 || canDlc > 15) 
    sanitizedDlc = 0;
  else 
    sanitizedDlc = canDlc;

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

  HAL_FDCAN_AddMessageToTxFifoQ(&Interface, &TxHeader, trimmedDataArray);
}


