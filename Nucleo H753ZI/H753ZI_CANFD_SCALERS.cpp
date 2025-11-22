/*  ------------------------------  *
 *  -- H753ZI_CANFD_SCALERS.cpp --  *
 *  ------------------------------  */
#ifdef ARDUINO_NUCLEO_H753ZI
#include "STM32DuinoCANFD.hpp"

/* -------------------------------------------------------------------- *
 * -- enum Bitrate in StCANFD.hpp contains possible bitrates         -- * 
 * -- this is used to index this array to get the timing values      -- *
 * -- FDCANScalers[Bitrate::b500000] returns a struct with           -- *
 * -- Prescaler=20, SyncJump=1, Segment1=13, Segment2=2              -- *
 * -- these values are used to initialize the HAL CAN hardware       -- *
 * -- and hit the desired bitrate. The values below were calculated  -- *
 * -- assuming your clock is running at 160MHz. the g474re's default -- *
 * -- clock is 170MHz, which doesn't divide neatly into most CANFD   -- *
 * -- bitrates, so setting it to 160MHz is important.                -- *
 * -------------------------------------------------------------------- */
 FDCAN_ScalerStruct FDCANScalers[24] = {
  // prescaler, sync, seg1, seg2 
     {240, 1, 13, 2}, //  31,250 bps = 120MHz/(240*16)
     {225, 1, 13, 2}, //  33,333 bps = 120MHz/(225*16)
     {200, 1, 13, 2}, //  37,500 bps = 120MHz/(200*16)
     {150, 1, 13, 2}, //  50,000 bps = 120MHz/(150*16)
     {120, 1, 13, 2}, //  62,500 bps = 120MHz/(120*16)
     {100, 1, 13, 2}, //  75,000 bps = 120MHz/(100*16)
     {90,  1, 13, 2}, //  83,333 bps = 120MHz/(90*16)
     {75,  1, 13, 2}, // 100,000 bps = 120MHz/(75*16)
     {60,  1, 13, 2}, // 125,000 bps = 120MHz/(60*16)
     {50,  1, 13, 2}, // 150,000 bps = 120MHz/(50*16)
     {40,  1, 13, 2}, // 187,500 bps = 120MHz/(40*16)
     {30,  1, 13, 2}, // 250,000 bps = 120MHz/(30*16)
     {20,  1, 13, 2}, // 375,000 bps = 120MHz/(20*16)
     { 8,  1, 23, 6}, // 500,000 bps = 120MHz/(8*30)  ✓ VERIFIED
     {10,  1, 13, 2}, // 750,000 bps = 120MHz/(10*16)
     { 8,  1, 13, 2}, // 937,500 bps = 120MHz/(8*16)
     { 6,  1, 13, 2}, //1,250,000 bps = 120MHz/(6*16)
     { 5,  1, 13, 2}, //1,500,000 bps = 120MHz/(5*16)
     { 5,  1,  8, 3}, //2,000,000 bps = 120MHz/(5*12)  ✓ VERIFIED
     { 4,  1,  8, 3}, //2,500,000 bps = 120MHz/(4*12)
     { 3,  1,  7, 2}, //4,000,000 bps = 120MHz/(3*10)
     { 2,  1,  8, 3}, //5,000,000 bps = 120MHz/(2*12)
     { 2,  1,  7, 2}, //6,000,000 bps = 120MHz/(2*10)
     { 2,  1,  5, 2}, //7,500,000 bps = 120MHz/(2*8)
};
#endif
