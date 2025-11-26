#include <Arduino.h>
#include "STM32DuinoCANFD.h"

FDCAN_Frame SendFrame;
FDCAN_Frame RecvFrame;
FDCAN_Instance can(FDCAN_Channel::CH1);

uint32_t sendLoop = 0;
uint32_t recvLoop = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting Up");

    FDCAN_Settings Settings(250e3, 1e6);
    Settings.Mode = FDCAN_Mode::INTERNAL_LOOPBACK;

    if (can.begin(&Settings) != FDCAN_Status::OK) {
        Serial.println("Failed to initialize CAN-FD Channel");
        while (true) { }
    }
}

void loop() {
    // send every 1000ms
    if (millis() - sendLoop >= 1000) {
        SendFrame.canId = 0x100;
        SendFrame.canDlc = 8;
        SendFrame.clear();
        SendFrame.SetUnsigned(42, 0, 8);
        SendFrame.SetSigned(-42, 8, 8);
        SendFrame.SetFloat(3.14159, 32, 32);

        if (can.sendFrame(&SendFrame) != FDCAN_Status::OK) {
            Serial.print(millis());
            Serial.println(" - error sending can frame");
        }
        sendLoop = millis();
    }

    // check inbox every 1ms
    if (millis() - recvLoop >= 1) {
        // pop returns FDCAN_Status::FIFO_EMPTY and does nothing
        // to the frame if the receive inbox is empty.
        if (can.inbox.pop(RecvFrame) == FDCAN_Status::OK) {
            Serial.print(millis());
            Serial.println("ms ---------------");
            Serial.print("Unsigned Value : ");
            Serial.println(RecvFrame.GetUnsigned(0, 8));
            Serial.print("Signed Value   : ");
            Serial.println(RecvFrame.GetSigned(8, 8));
            Serial.print("Float Value    : ");
            Serial.println(RecvFrame.GetFloat(32, 32));
            Serial.println();
        }
        recvLoop = millis();
    }
}
