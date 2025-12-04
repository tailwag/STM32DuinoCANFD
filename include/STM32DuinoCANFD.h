/*  --------------------------------  *
 *  --  STM32DuinoCANFD.h         --  *
 *  --------------------------------  */
#ifndef STM32DUINOCANFD_H_
#define STM32DUINOCANFD_H_

#include "FDCAN_Defines.h"
#include "FDCAN_Frame.h"
#include "FDCANHAL.h"

extern const FDCAN_ScalerStruct FDCANScalers[24];

uint32_t DlcToLen(uint32_t dlcIn);


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
