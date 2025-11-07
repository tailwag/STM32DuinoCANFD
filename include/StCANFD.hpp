#include "stm32g474xx.h"
#include <cstdint>
#ifndef _MAINH
#include "main.hpp"
#endif // !_MAINH

#ifndef _STCANFDHPP
#define _STCANFDHPP

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

enum Endian {
  Little, 
  Big,
};

uint8_t DlcToLen(uint8_t dlcIn);

class CanFrame {
  public:
    uint16_t canId;
    uint8_t canDlc;
    uint8_t data[64];
    bool brs;

    uint32_t GetUnsigned(uint8_t startByte, uint8_t startBit, uint8_t length, Endian order = Little);
    int32_t    GetSigned(uint8_t startByte, uint8_t startBit, uint8_t length, Endian order = Little);
    float       GetFloat(uint8_t startByte, uint8_t startBit, uint8_t length, Endian order = Little);

    void     SetUnsigned(uint32_t value, uint8_t startByte, uint8_t startBit, uint8_t length, Endian order = Little);
    void       SetSigned(int32_t value,  uint8_t startByte, uint8_t startBit, uint8_t length, Endian order = Little);
    void        SetFloat(float value,    uint8_t startByte, uint8_t startBit, uint8_t length, Endian order = Little);

    CanFrame();
};


class FDCanChannel {
  private:
    FDCAN_HandleTypeDef Interface;
    uint32_t timeLastSend;
    uint32_t timeLastRecv;

  public:
    uint32_t lastSend(void);
    uint32_t lastRecv(void);
    void start(void);
    void sendFrame(CanFrame * Frame);

    FDCanChannel(HwCanChannel chan, Bitrate baseRate, Bitrate dataRate);
};

#endif
