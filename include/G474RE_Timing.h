/*  -------------------------------  *
 *  -- G474RE_Timing.h           --  *
 *  -------------------------------  */
#ifndef G474RE_TIMING_H_
#define G474RE_TIMING_H_

// more or less taken directly from STM Cube IDE 
// constrained down further so the search loop is faster

const FDCAN_TimingConstraints NominalConstraints {
    6,   // PrescalerMin
    200, // PrescalerMax
    1,   // SyncJumpMin
    2,   // SyncJumpMax
    4,   // TimeSeg1Min
    16,  // TimeSeg1Max
    2,   // TimeSeg2Min 
    16   // TimeSeg2Max
};


const FDCAN_TimingConstraints DataConstraints {
    1,   // PrescalerMin
    32,  // PrescalerMax
    1,   // SyncJumpMin
    16,  // SyncJumpMax
    1,   // TimeSeg1Min
    24,  // TimeSeg1Max
    1,   // TimeSeg2Min 
    16   // TimeSeg2Max
};

#endif
