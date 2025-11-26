/*  ----------------------------  *
 *  -- FDCANDefines.h         --  *
 *  ----------------------------  */
#ifndef FDCAN_DEFINES_H_
#define FDCAN_DEFINES_H_

#include <Arduino.h>
#include <cstdint>

#ifdef ARDUINO_NUCLEO_GOB1RE
#include "Nucleo_G0B1RE/G0B1RE_Defines.h"
#endif
#ifdef ARDUINO_NUCLEO_G474RE
#include "Nucleo_G474RE/G474RE_Defines.h"
#endif
#ifdef ARDUINO_NUCLEO_H753ZI
#include "Nucleo_H753ZI/H753ZI_Defines.h"
#endif

// determine number of possible canfd channel instances
#if defined (FDCAN3)
#define NUM_INST 3 
#elif defined (FDCAN2)
#define NUM_INST 2 
#elif defined (FDCAN1)
#define NUM_INST 1
#endif

// return type for canfd methods
enum FDCAN_Status {
    OK,
    INIT_FAILED,
    START_FAILED,
    FIFO_FULL,
    FIFO_EMPTY,
    SEND_FAILED,
    INVALID_VALUE,
};

// used for array which contains all hardware scaler 
// values, which determine the baud rate
struct FDCAN_ScalerStruct {
    uint16_t Prescaler;
    uint8_t  SyncJump;
    uint8_t  Segment1;
    uint8_t  Segment2;
    uint32_t Bitrate;
    float    SamplePoint;
    float    BitrateError;
    float    SamplePointError;
    uint8_t  TimeQuanta;
};

struct FDCAN_TimingConstraints {
    uint16_t PrescalerMin;
    uint16_t PrescalerMax;
    uint8_t  SyncJumpMin;
    uint8_t  SyncJumpMax;
    uint8_t  TimeSeg1Min;
    uint8_t  TimeSeg1Max;
    uint8_t  TimeSeg2Min;
    uint8_t  TimeSeg2Max;
};

#include "FDCAN_Bitrate_Calc.h"

#ifdef ARDUINO_NUCLEO_G0B1RE
#include "Nucleo_G0B1RE/G0B1RE_Timing.h"
#endif
#ifdef ARDUINO_NUCLEO_G474RE
#include "Nucleo_G474RE/G474RE_Timing.h"
#endif
#ifdef ARDUINO_NUCLEO_H753ZI
#include "Nucleo_H753ZI/H753ZI_Timing.h"
#endif

enum FDCAN_Channel {
    CH1,
    CH2, 
    CH3,
};

enum FDCAN_ByteOrder {
    Intel, 
    Motorola,
};

enum FDCAN_FrameFormat {
    CLASSIC = FDCAN_FRAME_CLASSIC,
    FD_NO_BRS = FDCAN_FRAME_FD_NO_BRS,
    FD_BRS = FDCAN_FRAME_FD_BRS
};
enum FDCAN_Mode {
    NORMAL = FDCAN_MODE_NORMAL,
    RESTRICTED = FDCAN_MODE_RESTRICTED_OPERATION,
    MONITORING = FDCAN_MODE_BUS_MONITORING,
    INTERNAL_LOOPBACK = FDCAN_MODE_INTERNAL_LOOPBACK,
    EXTERNAL_LOOPBACK = FDCAN_MODE_EXTERNAL_LOOPBACK
};

enum FDCAN_TxFifoQueueMode {
    FIFO = FDCAN_TX_FIFO_OPERATION,
    QUEUE = FDCAN_TX_QUEUE_OPERATION
};

#ifdef ARDUINO_NUCLEO_H753ZI
enum FDCAN_ElmtSize {
    BYTES_8  = FDCAN_DATA_BYTES_8,
    BYTES_12 = FDCAN_DATA_BYTES_12,
    BYTES_16 = FDCAN_DATA_BYTES_16,
    BYTES_20 = FDCAN_DATA_BYTES_20,
    BYTES_24 = FDCAN_DATA_BYTES_24,
    BYTES_32 = FDCAN_DATA_BYTES_32,
    BYTES_48 = FDCAN_DATA_BYTES_48,
    BYTES_64 = FDCAN_DATA_BYTES_64
};
#endif

#include "FDCAN_Settings.h"


#endif
