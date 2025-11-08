#include "stm32_def.h"
#include "stm32g474xx.h"
#include "stm32g4xx_hal_rcc.h"
#include "stm32g4xx_hal_fdcan.h"

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

    void clear();
    
    uint32_t GetUnsigned(uint16_t startBit, uint8_t length, Endian order = Little);
    int32_t    GetSigned(uint16_t startBit, uint8_t length, Endian order = Little);
    float       GetFloat(uint16_t startBit, uint8_t length, Endian order = Little);

    void     SetUnsigned(uint32_t value, uint8_t startBit, uint8_t length, Endian order = Little);
    void       SetSigned(int32_t value,  uint8_t startBit, uint8_t length, Endian order = Little);
    void        SetFloat(float value,    uint8_t startBit, uint8_t length, Endian order = Little);

    CanFrame();
};

class CanInbox {
  private:
    static constexpr size_t MAX_MESSAGES = 16;
    CanFrame buffer [MAX_MESSAGES]; 
    volatile uint8_t head = 0;
    volatile uint8_t tail = 0;

  public:
    bool  push(const FDCAN_RxHeaderTypeDef &rxHeader, const uint8_t *data);
    bool   pop(CanFrame &out);
    bool empty() const { return head == tail; }
    bool  full() const { return ((head + 1) % MAX_MESSAGES) == tail; }
};

class FDCanChannel {
  private:
    FDCAN_HandleTypeDef Interface;
    HwCanChannel ChannelID;
    uint32_t timeLastSend;
    uint32_t timeLastRecv;

    static FDCanChannel *Instances[3];

  public:
    CanInbox inbox;
    uint32_t lastSend() const { return timeLastRecv; }
    uint32_t lastRecv() const { return timeLastSend; }
    void start(void);
    void sendFrame(CanFrame * Frame);
    void handleRxInterrupt();

    static FDCanChannel *getInstance(HwCanChannel chan) { return Instances[chan]; }
    FDCAN_HandleTypeDef *getHandle() { return &Interface; }

    FDCanChannel(HwCanChannel chan, Bitrate baseRate, Bitrate dataRate);
};

#endif
