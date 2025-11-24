/*  --------------------------------  *
 *  --  STM32DuinoCANFD.hpp       --  *
 *  --------------------------------  */
#include "FDCANDefines.h"
#include "FDCANHAL.h"
#include <cstdint>

#ifndef _STCANFDHPP
#define _STCANFDHPP

extern const FDCAN_ScalerStruct FDCANScalers[24];

uint32_t DlcToLen(uint32_t dlcIn);

class FDCAN_Frame {
    public:
        uint32_t canId;
        uint32_t canDlc;
        uint8_t data[64];
        bool brs;
        FDCAN_FrameFormat FDFormat; 

        void clear();

        uint32_t GetUnsigned(uint16_t startBit, uint8_t length, FDCAN_ByteOrder order = Intel);
        int32_t    GetSigned(uint16_t startBit, uint8_t length, FDCAN_ByteOrder order = Intel);
        float       GetFloat(uint16_t startBit, uint8_t length, FDCAN_ByteOrder order = Intel);

        FDCAN_Status SetUnsigned(uint32_t value, uint8_t startBit, uint8_t length, FDCAN_ByteOrder order = Intel);
        FDCAN_Status   SetSigned(int32_t value,  uint8_t startBit, uint8_t length, FDCAN_ByteOrder order = Intel);
        FDCAN_Status    SetFloat(float value,    uint8_t startBit, uint8_t length, FDCAN_ByteOrder order = Intel);

        FDCAN_Frame();
};

class FDCAN_Inbox {
    private:
        static constexpr size_t MAX_MESSAGES = 16;
        FDCAN_Frame buffer [MAX_MESSAGES]; 
        volatile uint8_t head = 0;
        volatile uint8_t tail = 0;

    public:
        FDCAN_Status push(const FDCAN_RxHeaderTypeDef &rxHeader, const uint8_t *data);
        FDCAN_Status  pop(FDCAN_Frame &out);

        bool empty() const { return head == tail; }
        bool  full() const { return ((head + 1) % MAX_MESSAGES) == tail; }
};

class FDCAN_Instance {
    private:
        FDCAN_HandleTypeDef Interface;
        FDCAN_Channel ChannelID;

        uint32_t timeLastSend;
        uint32_t timeLastRecv;

        static FDCAN_Instance *Instances[NUM_INST];

    public:
        FDCAN_Inbox inbox;
        void handleRxInterrupt();

        uint32_t lastSend() const { return timeLastRecv; }
        uint32_t lastRecv() const { return timeLastSend; }

        FDCAN_Status begin(FDCAN_Settings *Settings);
        FDCAN_Status sendFrame(FDCAN_Frame * Frame);

        FDCAN_HandleTypeDef   *getHandle() { return &Interface; }
        static FDCAN_Instance *getInstance(FDCAN_Channel chan) { return Instances[chan]; }

        FDCAN_Instance(FDCAN_Channel chan);
};
#endif
