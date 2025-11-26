/*  --------------------------------  *
 *  --  FDCAN_Frame.h             --  *
 *  --------------------------------  */
#include "FDCAN_Frame.h"

// initialize empty frame 
FDCAN_Frame::FDCAN_Frame() {
    canId = 0;
    canDlc = 0;
    brs = true;

    memset(data, 0, sizeof(data));
}

// Clear all data from frame's byte array.
void FDCAN_Frame::clear() {
    memset(data, 0, sizeof(data));
}

// get unsigned data. this is our most important read function. every other data 
// type relies on this function to first get the "raw bits" out of the message
uint32_t FDCAN_Frame::GetUnsigned(uint16_t startBit, uint8_t length, FDCAN_ByteOrder order) {
    uint32_t retVal  =  0; // value that all our bits get or'd into
    int8_t firstByte = -1; // use as a flag and also byte offset

    for(uint8_t i = 0; i < length; i++) {
        uint16_t absBit = startBit + i;  // walk over each individual bit
        uint8_t byteIndex = absBit / 8;  // get the byte number we're on

        if (order == Intel) { // yay the bits are in the correct order
            uint8_t shiftBy  = absBit % 8; 

            // bring bits down to right place and isolate individual bit
            uint8_t bit = (data[byteIndex] >> shiftBy) & 1u;

            // add to return value
            retVal |= bit << i;
        }
        else {               // motorola format >:(
            // set first byte value. this is used to reverse the 
            // direction in which we travel through the byte array
            if (firstByte < 0) firstByte = byteIndex;

            // move up instead of down
            if (byteIndex != firstByte) 
                byteIndex = firstByte - (byteIndex - firstByte);

            // get shift value. positive/negative indicates direction
            int8_t shiftBy  = (absBit % 8) - i;

            // add value to return value
            if (shiftBy >= 0)
                retVal |= (data[byteIndex] >>  shiftBy) & (1u << i);
            else 
                retVal |= (data[byteIndex] << -shiftBy) & (1u << i);
        }
    }

    return retVal;
}

// arbitrary length signed values. we handle moving the sign
// bit on our own so we can return a fixed size signed int
int32_t FDCAN_Frame::GetSigned(uint16_t startBit, uint8_t length, FDCAN_ByteOrder order) {
    // get raw bits in unsigned value
    uint32_t rawValue = GetUnsigned(startBit, length, order);

    // shift the sign bit down to determine if value is negative
    bool isNeg = rawValue >> (length - 1) & 1u; 

    if (isNeg) {
        // generate bit mask for later or
        // 00001010 | 11111000 = 11111010
        int32_t bitMask = -1 << (length - 1);
        return rawValue | bitMask; 
    }

    return static_cast<int32_t>(rawValue);
}

// this is the one time I like floats. the can float 
// data types are the same fixed lengths as in c++
float FDCAN_Frame::GetFloat(uint16_t startBit, uint8_t length, FDCAN_ByteOrder order) {
    uint32_t rawValue = GetUnsigned(startBit, length, order);
    float retVal = * ( float * ) &rawValue; // evil floating point bit hacking 

    return retVal;
}

// main data set function. just like with the receive side, the other data 
// set commands rely on this function to actually write the data into the array 
FDCAN_Status FDCAN_Frame::SetUnsigned(uint32_t value, uint8_t startBit, uint8_t length, FDCAN_ByteOrder order) {
    int8_t firstByte = -1;
    uint32_t upper = (1u << length) - 1;     // get max unsigned value

    // fail out if value doesn't fit in bit allocation
    if (value > upper || value < 0)
        return FDCAN_Status::INVALID_VALUE;

    for (uint8_t i = 0; i < length; i++) {
        uint8_t bit, shiftVal;
        uint16_t absBit = startBit + i; // absolute position of bit we're on
        uint8_t byteIndex = absBit / 8; // calculate which byte we're on

        if (order == Motorola) {
            // set first byte value. this is used to reverse the
            // direction in which we travel through the byte array
            if (firstByte < 0) firstByte = byteIndex; 

            // go up not down
            if (byteIndex != firstByte)
                byteIndex = firstByte - (byteIndex - firstByte);
        }

        bit = (value >> i) & 1u;
        shiftVal  = absBit % 8; 

        if (bit)
            data[byteIndex] |=  (1u << shiftVal);
        else
            data[byteIndex] &= ~(1u << shiftVal);
    }

    return FDCAN_Status::OK;
}

// convert signed bits to unsigned int value and use SetUnsigned to set value
// we have to manually mover the sign bit, because can signed ints are variable length
FDCAN_Status FDCAN_Frame::SetSigned(int32_t value, uint8_t startBit, uint8_t length, FDCAN_ByteOrder order) {
    // bit manipulation to get upper and lower limits
    int32_t lower = -1 << (length - 1);
    int32_t upper = ~lower;

    // fail out if value doesn't fit in bit allocation
    if (value > upper || value < lower)
        return FDCAN_Status::INVALID_VALUE;

    // get & bitmask for final value 
    // we can reuse upper here 
    uint32_t bitmask = ((uint32_t)upper << 1) + 1u;

    // apply the bitmask to the final value 
    value &= bitmask; 

    // set the value 
    return SetUnsigned(value, startBit, length, order);
}

// convert float value bits to unsigned int value and use SetUnsigned to set value
// this is convenient, because can floats are always 32 bits
FDCAN_Status FDCAN_Frame::SetFloat(float value, uint8_t startBit, uint8_t length, FDCAN_ByteOrder order) {
    // check for NaN
    if (value != value)
        return FDCAN_Status::INVALID_VALUE;

    uint32_t longVal = * ( uint32_t * ) &value; // evil floating point bit hacking
    return SetUnsigned(longVal, startBit, length, order);
}
