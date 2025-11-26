/*  ----------------------------  *
 *  -- FDCAN_Bitrate_Calc.cpp --  *
 *  ----------------------------  */
#include "FDCAN_Defines.h"
#include "FDCAN_Bitrate_Calc.h"

FDCAN_ScalerStruct getScalers(uint32_t targetBitrate, uint8_t samplePoint, FDCAN_TimingConstraints constraints) {
    uint32_t clkFreq = getCanClock();

    FDCAN_ScalerStruct best {}; 
    bool bestValid = false;

    for (uint16_t ps = constraints.PrescalerMin; ps <= constraints.PrescalerMax; ++ps) {
        for (uint8_t sj = constraints.SyncJumpMin; sj <= constraints.SyncJumpMax; ++sj) {
            for (uint8_t s1 = constraints.TimeSeg1Min; s1 <= constraints.TimeSeg1Max; ++s1) {
                for (uint8_t s2 = constraints.TimeSeg2Min; s2 <= constraints.TimeSeg2Max; ++s2) {
                    // SyncJump can't exceed min of timeseg
                    if (sj > s1 || sj > s2) continue;

                    uint32_t tqTotal = sj + s1 + s2;
                    uint32_t divisor = ps * tqTotal; 

                    if (divisor == 0) continue;

                    uint32_t bitrate = clkFreq / divisor;

                    float bitrateErr = ((bitrate > targetBitrate) ?
                            (bitrate - targetBitrate) :
                            (targetBitrate - bitrate)) / (float)targetBitrate;

                    if (bitrateErr > 0.02f) continue;

                    FDCAN_ScalerStruct cur; 
                    cur.Prescaler    = ps;
                    cur.SyncJump     = sj;
                    cur.Segment1     = s1;
                    cur.Segment2     = s2;
                    cur.Bitrate      = bitrate;
                    cur.TimeQuanta   = tqTotal;
                    cur.SamplePoint  = (1.0f + s1) * 100.0f / tqTotal;
                    cur.BitrateError = bitrateErr * 1e6f; // convert to ppm

                    float spErr = (cur.SamplePoint > samplePoint) ?
                        (cur.SamplePoint - samplePoint) :
                        (samplePoint - cur.SamplePoint);

                    cur.SamplePointError = spErr;

                    if (!bestValid) {
                        best = cur;
                        bestValid = true;
                    }
                    else {
                        float spDiff = best.SamplePointError - cur.SamplePointError;
                        if (spDiff < 0)
                            spDiff = -spDiff;

                        float brDiff = best.SamplePointError - cur.SamplePointError;
                        if (brDiff < 0)
                            brDiff = -brDiff;

                        bool chooseCur; 

                        if (spDiff < 0.05f) {
                            if (brDiff < 100.0f)
                                chooseCur = (cur.TimeQuanta > best.TimeQuanta);
                            else 
                                chooseCur = (cur.BitrateError < best.BitrateError);
                        }
                        else {
                            chooseCur = (cur.SamplePointError < best.SamplePointError);
                        }

                        if (chooseCur) best = cur;
                    }
                }
            }
        }
    }
    return best;
}

