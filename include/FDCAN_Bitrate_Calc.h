/*  ----------------------------  *
 *  -- FDCAN_Bitrate_Calc.h   --  *
 *  ----------------------------  */
#ifndef FDCAN_BITRATE_CALC_H_
#define FDCAN_BITRATE_CALC_H_

FDCAN_ScalerStruct getScalers(uint32_t targetBitrate, uint8_t samplePoint, FDCAN_TimingConstraints constraints);

#endif
