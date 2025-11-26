/*  --------------------------------  *
 *  --  FDCAN_Settings.h          --  *
 *  --------------------------------  */
#ifndef FDCAN_SETTINGS_H_
#define FDCAN_SETTINGS_H_ 
#include "FDCAN_Defines.h"
#include <cstdint>

class FDCAN_Settings {
    private:
        uint32_t NominalPrescaler   = 0;
        uint32_t NominalSyncJump    = 0;
        uint32_t NominalSegment1    = 0;
        uint32_t NominalSegment2    = 0;

        uint32_t NominalBitrate     = 0;
        uint32_t NominalTimeQuanta  = 0;
        float    NominalSamplePoint = 0.0;
        float    NominalBitrateErr  = 0.0;
        float    NominalSampleErr   = 0.0;

        uint32_t DataPrescaler      = 0;
        uint32_t DataSyncJump       = 0;
        uint32_t DataSegment1       = 0;
        uint32_t DataSegment2       = 0;

        uint32_t DataBitrate        = 0;
        uint32_t DataTimeQuanta     = 0;
        float    DataSamplePoint    = 0.0;
        float    DataBitrateErr     = 0.0;
        float    DataSampleErr      = 0.0;

    public:
        uint32_t GetNominalPrescaler()   const { return NominalPrescaler; }
        uint32_t GetNominalSyncJump()    const { return NominalSyncJump;  }
        uint32_t GetNominalSegment1()    const { return NominalSegment1;  }
        uint32_t GetNominalSegment2()    const { return NominalSegment2;  }

        uint32_t GetNominalBitrate()     const { return NominalBitrate;     }
        uint32_t GetNominalTimeQuanta()  const { return NominalTimeQuanta;  }
        float    GetNominalSamplePoint() const { return NominalSamplePoint; }
        float    GetNominalBitrateErr()  const { return NominalBitrateErr;  }
        float    GetNominalSampleErr()   const { return NominalSampleErr;   }

        uint32_t GetDataPrescaler()      const { return DataPrescaler; }
        uint32_t GetDataSyncJump()       const { return DataSyncJump;  }
        uint32_t GetDataSegment1()       const { return DataSegment1;  }
        uint32_t GetDataSegment2()       const { return DataSegment2;  }

        uint32_t GetDataBitrate()        const { return DataBitrate;     }
        uint32_t GetDataTimeQuanta()     const { return DataTimeQuanta;  }
        float    GetDataSamplePoint()    const { return DataSamplePoint; }
        float    GetDataBitrateErr()     const { return DataBitrateErr;  }
        float    GetDataSampleErr()      const { return DataSampleErr;   }

        FDCAN_FrameFormat FrameFormat = FD_BRS;
        FDCAN_Mode Mode = NORMAL;
        FunctionalState AutoRetransmission = ENABLE;
        FunctionalState TransmitPause      = ENABLE;
        FunctionalState ProtocolException  = DISABLE;
        uint32_t StdFiltersNbr = 0;
        uint32_t ExtFiltersNbr = 0;
        FDCAN_TxFifoQueueMode TxFifoQueueMode = FIFO;

        #ifdef ARDUINO_NUCLEO_H753ZI
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
        #endif

        FDCAN_Settings(uint32_t nominalBitrate = 500000, 
                        uint32_t dataBitrate = 2000000,
                        uint8_t nominalSamplePoint = 80,
                        uint8_t dataSamplePoint = 80);
};
#endif
