/*  --------------------------------  *
 *  --  FDCAN_Frame.h             --  *
 *  --------------------------------  */
#ifndef FDCAN_FRAME_H_
#define FDCAN_FRAME_H_
#include "FDCAN_Defines.h"

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
#endif
