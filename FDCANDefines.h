/*  ----------------------------  *
 *  -- FDCANDefines.h         --  *
 *  ----------------------------  */
#include <Arduino.h>
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
};

enum FDCAN_Channel {
    CH1,
    CH2, 
    CH3,
};

enum FDCAN_Bitrate {
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

class FDCAN_Settings {
    public:
        FDCAN_Bitrate NominalBitrate  = b500000; 
        FDCAN_Bitrate DataBitrate     = b2000000;
        FDCAN_FrameFormat FrameFormat = FD_BRS;
        FDCAN_Mode Mode = NORMAL;
        FunctionalState AutoRetransmission = ENABLE;
        FunctionalState TransmitPause      = ENABLE;
        FunctionalState ProtocolException  = DISABLE;
        uint32_t StdFiltersNbr = 0;
        uint32_t ExtFiltersNbr = 0;
        FDCAN_TxFifoQueueMode TxFifoQueueMode = FIFO;
        uint32_t MessageRAMOffset = 0;
        uint32_t RxFifo0ElmtsNbr  = 1;
        FDCAN_ElmtSize RxFifo0ElmtSize = BYTES_64;
        uint32_t RxFifo1ElmtsNbr  = 0;
        FDCAN_ElmtSize RxFifo1ElmtSize = BYTES_64;
        uint32_t RxBuffersNbr = 0;
        FDCAN_ElmtSize RxBufferSize    = BYTES_64;
        uint32_t TxEventsNbr  = 0;
        uint32_t TxBuffersNbr = 0;
        uint32_t TxFifoQueueElmtsNbr = 16;
        FDCAN_ElmtSize TxElmtSize  = BYTES_64;
};
