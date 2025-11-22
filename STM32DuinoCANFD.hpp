/*  --------------------------------  *
 *  --  STM32DuinoCANFD.hpp       --  *
 *  --------------------------------  */
#include <Arduino.h>
#include "FDCANHAL.h"
#include "stm32_def.h"

#ifdef ARDUINO_NUCLEO_GOB1RE
#include "stm32g0b1xx.h"
#include "stm32g0xx_hal_def.h"
#include "stm32g0xx_hal_rcc.h"
#include "stm32g0xx_hal_fdcan.h"
#endif
#ifdef ARDUINO_NUCLEO_G474RE
#include "stm32g474xx.h"
#include "stm32g4xx_hal_def.h"
#include "stm32g4xx_hal_rcc.h"
#include "stm32g4xx_hal_fdcan.h"
#endif
#ifdef ARDUINO_NUCLEO_H753ZI
#include "stm32h753xx.h"
#include "stm32h7xx_hal_def.h"
#include "stm32h7xx_hal_rcc.h"
#include "stm32h7xx_hal_fdcan.h"
#endif

#ifndef _STCANFDHPP
#define _STCANFDHPP

#if defined (FDCAN3)
#define NUM_INST 3 
#elif defined (FDCAN2)
#define NUM_INST 2 
#elif defined (FDCAN1)
#define NUM_INST 1
#endif

struct FDCAN_ScalerStruct {
    uint16_t Prescaler;
    uint8_t  SyncJump;
    uint8_t  Segment1;
    uint8_t  Segment2;
};

extern FDCAN_ScalerStruct FDCANScalers[24];

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
    Intel, 
    Motorola,
};

uint8_t DlcToLen(uint8_t dlcIn);

class CanFrame {
    public:
        uint16_t canId;
        uint8_t canDlc;
        uint8_t data[64];
        bool brs;

        void clear();

        uint32_t GetUnsigned(uint16_t startBit, uint8_t length, Endian order = Intel);
        int32_t    GetSigned(uint16_t startBit, uint8_t length, Endian order = Intel);
        float       GetFloat(uint16_t startBit, uint8_t length, Endian order = Intel);

        void     SetUnsigned(uint32_t value, uint8_t startBit, uint8_t length, Endian order = Intel);
        void       SetSigned(int32_t value,  uint8_t startBit, uint8_t length, Endian order = Intel);
        void        SetFloat(float value,    uint8_t startBit, uint8_t length, Endian order = Intel);

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

        static FDCanChannel *Instances[NUM_INST];

    public:
        CanInbox inbox;
        void begin(void);
        void handleRxInterrupt();
        HAL_StatusTypeDef sendFrame(CanFrame * Frame);
        uint32_t lastSend() const { return timeLastRecv; }
        uint32_t lastRecv() const { return timeLastSend; }
        FDCAN_HandleTypeDef *getHandle() { return &Interface; }
        static FDCanChannel *getInstance(HwCanChannel chan) { return Instances[chan]; }

        FDCanChannel(HwCanChannel chan, Bitrate baseRate, Bitrate dataRate);
};

#endif
