#include "fdcan.h"
#include "stm32g4xx_hal_fdcan.h"
#include <cstdint>


uint8_t dlcToLen(uint8_t dlcIn);

struct FDCAN_ScalerStruct {
  uint16_t Prescaler;
  uint8_t  SyncJump;
  uint8_t  Segment1;
  uint8_t  Segment2;
};

enum HwCanChannel {
  CH1,
  CH2, 
  CH3, 
};

enum Bitrate {
  b31250,
  b33333,
  b40000,
  b50000,
  b62500,
  b80000,
  b83333,
  b100000,
  b125000,
  b160000,
  b200000,
  b250000,
  b400000,
  b500000,
  b800000,
  b1000000,
  b1250000,
  b1600000,
  b2000000,
  b2500000,
  b4000000,
  b5000000,
  b6000000,
  b8000000,
};
static const FDCAN_ScalerStruct FDCANScalers[24] = {
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
class FDCanChannel {
  private:
    FDCAN_HandleTypeDef Interface;
    uint32_t timeLastSend;
    uint32_t timeLastRecv;

  public:
    uint32_t lastSend(void);
    uint32_t lastRecv(void);
    void sendFrame(uint16_t canId, uint8_t canDlc, uint8_t * canData, bool BRS = true);

    FDCanChannel(HwCanChannel chan, Bitrate baseRate, Bitrate dataRate);
};

