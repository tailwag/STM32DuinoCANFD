/*  -------------------------------  *
 *  --  G474RE_CANFD_SCALERS.cpp --  *
 *  -------------------------------  */
#ifdef ARDUINO_NUCLEO_G474RE
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
  {320, 1, 13, 2}, //  32,250 bps
  {300, 1, 13, 2}, //  33,333 bps
  {250, 1, 13, 2}, //  40,000 bps
  {200, 1, 13, 2}, //  50,000 bps
  {160, 1, 13, 2}, //  62,500 bps
  {125, 1, 13, 2}, //  80,000 bps
  {120, 1, 13, 2}, //  83,333 bps
  {100, 1, 13, 2}, // 100,000 bps
  {80,  1, 13, 2}, // 125,000 bps
  {50,  1, 17, 2}, // 160,000 bps
  {50,  1, 13, 2}, // 200,000 bps
  {40,  1, 13, 2}, // 250,000 bps
  {20,  1, 17, 2}, // 400,000 bps  - works
  {20,  1, 13, 2}, // 500,000 bps  - works
  {20,  1,  7, 2}, // 800,000 bps  - works
  {16,  1,  7, 2}, //1,000,000 bps - works
  {16,  1,  5, 2}, //1,250,000 bps - works
  {10,  1,  7, 2}, //1,600,000 bps - works
  { 8,  1,  7, 2}, //2,000,000 bps - works
  { 8,  1,  5, 2}, //2,500,000 bps - vector: no, peak: yes
  { 4,  1,  7, 2}, //4,000,000 bps - vector: no, peak: yes
  { 4,  1,  5, 2}, //5,000,000 bps - vector: no, peak: no
  { 3,  1,  6, 2}, //6,000,000 bps - use with caution, actually 5,925,926
  { 2,  1,  7, 2}, //8,000,000 bps - vector: no, peak: no
};
#endif
