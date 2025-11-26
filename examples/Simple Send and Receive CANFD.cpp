#include <Arduino.h>
#include "STM32DuinoCANFD.h"

FDCAN_Frame SendFrame;
FDCAN_Frame RecvFrame;
FDCAN_Instance can(FDCAN_Channel::CH1);

uint32_t splitnum = 0;
uint32_t sendLoop = 0;
uint32_t recvLoop = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Up");

    FDCAN_Settings Settings; // defaults to 500k/2M
    Settings.Mode = FDCAN_Mode::EXTERNAL_LOOPBACK;

    if (can.begin(&Settings) != FDCAN_Status::OK) {
        Serial.println("Failed to initialize CAN-FD Channel");
        while (true) { }
    }
}

void loop() {
    // send every 100ms
    if (millis() - sendLoop >= 100) {
        SendFrame.canId = 0x200;
        SendFrame.canDlc = 8;

        // rollover if we reached max value, should be basically impossible
        if (splitnum == ~ (uint32_t) 0)
            splitnum = 0;
        else
            ++splitnum;

        // split 32 bit number into 8 bytes
        for (uint8_t i = 0; i < 4; i++) {
            SendFrame.data[i] = (splitnum >> (i * 8)) & 0xFF;
        }

        if (can.sendFrame(&SendFrame) != FDCAN_Status::OK) {
            Serial.println("Error sending CAN frame!");
        }

        sendLoop = millis();
    }

    // check inbox every 1ms
    if (millis() - recvLoop >= 1) {
        // pop returns FDCAN_Status::FIFO_EMPTY and does nothing
        // to the frame if the receive inbox is empty.
        if (can.inbox.pop(RecvFrame) == FDCAN_Status::OK) {
            Serial.print(millis());             // time
            Serial.print(" - 0x");
            Serial.print(RecvFrame.canId, HEX); // arbitration ID
            Serial.print(" [");
            Serial.print(RecvFrame.canDlc);     // DLC
            Serial.print("] - ");

            uint32_t numBytes = DlcToLen(RecvFrame.canDlc); 
            for (uint8_t i = 0; i < numBytes; i++) {
                Serial.print(RecvFrame.data[i], HEX); // data byte
                Serial.print(" ");
            }
            Serial.println();
        }
        recvLoop = millis();
    }
}
