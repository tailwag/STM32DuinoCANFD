/*  --------------------------------  *
 *  --  FDCAN_Settings.cpp        --  *
 *  --------------------------------  */
#include "FDCAN_Settings.h"

FDCAN_Settings::FDCAN_Settings(uint32_t nominalBitrate,
                                uint32_t dataBitrate,
                                uint8_t nominalSamplePoint,
                                uint8_t dataSamplePoint) {

    FDCAN_ScalerStruct nominalScalerStruct = getScalers(nominalBitrate, 
                                                        nominalSamplePoint, 
                                                        NominalConstraints);

    FDCAN_ScalerStruct dataScalerStruct = getScalers(dataBitrate,
                                                     dataSamplePoint,
                                                     DataConstraints);

    NominalPrescaler   = nominalScalerStruct.Prescaler;
    NominalSyncJump    = nominalScalerStruct.SyncJump;
    NominalSegment1    = nominalScalerStruct.Segment1;
    NominalSegment2    = nominalScalerStruct.Segment2;
    NominalBitrate     = nominalScalerStruct.Bitrate;
    NominalTimeQuanta  = nominalScalerStruct.TimeQuanta;
    NominalSamplePoint = nominalScalerStruct.SamplePoint;
    NominalBitrateErr  = nominalScalerStruct.BitrateError;
    NominalSampleErr   = nominalScalerStruct.SamplePointError;

    DataPrescaler   = dataScalerStruct.Prescaler;
    DataSyncJump    = dataScalerStruct.SyncJump;
    DataSegment1    = dataScalerStruct.Segment1;
    DataSegment2    = dataScalerStruct.Segment2;
    DataBitrate     = dataScalerStruct.Bitrate;
    DataTimeQuanta  = dataScalerStruct.TimeQuanta;
    DataSamplePoint = dataScalerStruct.SamplePoint;
    DataBitrateErr  = dataScalerStruct.BitrateError;
    DataSampleErr   = dataScalerStruct.SamplePointError;
}
