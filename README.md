
# STM32DuinoCANFD

A minimal library for untilizing the HAL FDCAN peripherals on STM32 microcontrollers. Includes a frame class with methods to set and get DBC compatible values easily. This allows for very easy translation from your database file to your micro code. This library serves as a combination of the MCP2515 libraries I was using in the past, and an old SBC compatibility layer I wrote forever ago. 



## Authors

- [Devin Shoemaker](https://www.github.com/tailwag)


## Acknowledgements
Inspiration taken from the following people:
 - [autowp](https://github.com/autowp) - Used his MCP2515 library for years before CAN-FD was needed. 
 - [pierremolinaro](https://github.com/pierremolinaro) - Several fantastic CAN libraries.


## Compile Flags
I use this library in platformio. To enable the CAN modules add the following to your platformio.ini

    build_flags=
        -DHAL_FDCAN_MODULE_ENABLED



## Warning
I don't know why, but this library doesn't seem to work on Windows. I develop on Linux, so I just haven't put that much effort into tracking down this issue.

## Supported Hardware
Currently only tested on the following:
- STM32 Nucleo H753ZI
- STM32 Nucleo G474RE
- STM32 Nucleo G0B1RE


## Demo
Just to show how easy it is to get a simple periodic send up and running.

    #include <Arduino.h>
    #include "STM32DuinoCANFD.h"
    
    FDCAN_Frame sendFrame;
    FDCAN_Instance can(FDCAN_Channel::CH1);
    
    void setup() {
        Serial.begin(115200);
    
        FDCAN_Settings settings;
        settings.Mode = FDCAN_Mode::EXTERNAL_LOOPBACK;
    
        if (can.begin(&settings) != FDCAN_Status::OK) {
            Serial.println("Error initializing CAN-FD channel!");
            while (true) {}
        }
    }
    
    void loop() {
        sendFrame.canId   = 0x101;
        sendFrame.canDlc  = 8;
        sendFrame.data[0] = 42;

        if (can.sendFrame(&sendFrame) != FDCAN_Status::OK)
            Serial.println(String(millis()) + " - error sending can frame");
    
        delay(1000);
    }

You can also adjust the bitrates and sample points by passing them to the settings constructor. The default values are 500k/2M bitrate, with 80% sample point for both the data and arbitration phases.

    #include <Arduino.h>
    #include "STM32DuinoCANFD.h"
    
    FDCAN_Frame sendFrame;
    FDCAN_Instance can(FDCAN_Channel::CH1);
    
    const uint32_t nominalBitrate = 250e3;
    const uint32_t    dataBitrate = 4e6;
    const uint8_t  nominalSample  = 75;
    const uint8_t     dataSample  = 82;
    
    void setup() {
        Serial.begin(115200);
    
        FDCAN_Settings settings(nominalBitrate, dataBitrate, nominalSample, dataSample);
        settings.Mode = FDCAN_Mode::EXTERNAL_LOOPBACK; // ensure ack is always sent
    
        if (can.begin(&settings) != FDCAN_Status::OK) {
            Serial.println("Error initializing CAN-FD channel!");
            while (true) {}
        }
    
        Serial.println("Actual nominal bitrate      : " + String(settings.GetNominalBitrate()));
        Serial.println("Actual data bitrate         : " + String(settings.GetDataBitrate()));
        Serial.println("Actual nominal sample point : " + String(settings.GetNominalSamplePoint()));
        Serial.println("Actual data sample point    : " + String(settings.GetDataSamplePoint()));
    }
    
    void loop() {
        sendFrame.canId = 0x101;
        sendFrame.canDlc = 8;
    
        uint8_t value    = 42;
        uint8_t startBit = 0;
        uint8_t length   = 8;
   
        // set unsigned int value using DBC parameters
        sendFrame.SetUnsigned(value, startBit, length);
    
        if (can.sendFrame(&sendFrame) != FDCAN_Status::OK)
            Serial.println(String(millis()) + " - error sending can frame");
    
        delay(1000);
    }

An "inbox" is provided for receive messages. This is just a ring buffer.

    #include <Arduino.h>
    #include "STM32DuinoCANFD.h"
    
    uint32_t sendTime = 0;
    uint32_t recvTime = 0;
    
    FDCAN_Frame sendFrame;
    FDCAN_Frame recvFrame;
    
    FDCAN_Instance can(FDCAN_Channel::CH1);
    
    void setup() {
        Serial.begin(115200);
    
        FDCAN_Settings settings;
        settings.Mode        = FDCAN_Mode::INTERNAL_LOOPBACK; // ensure ack is always sent
        settings.FrameFormat = FDCAN_FrameFormat::CLASSIC;    // send "standard" CAN 2.0 frames
    
        if (can.begin(&settings) != FDCAN_Status::OK) {
            Serial.println("Error initializing CAN-FD channel!");
            while (true) {}
        }
    }
    
    void loop() {
        uint8_t startBit = 0;
        uint8_t length   = 32;
    
        if (millis() - sendTime >= 1000) {
            sendFrame.canId = 0x101;
            sendFrame.canDlc = 8;
    
            float value = 3.14159;
            sendFrame.SetFloat(value, startBit, length);
    
            if (can.sendFrame(&sendFrame) != FDCAN_Status::OK)
                Serial.println(String(millis()) + " - error sending can frame");
    
            sendTime = millis();
        }
    
        if (millis() - recvTime >= 10) {
            // only continue if there was a message present to retrieve
            if (can.inbox.pop(recvFrame) == FDCAN_Status::OK) {
                float value = recvFrame.GetFloat(startBit, length);
    
                Serial.print(String(millis()) + " - ID: 0x");
                Serial.print(recvFrame.canId, HEX);
                Serial.print(", Value: ");
                Serial.println(value);
            }
    
            recvTime = millis();
        }
    }    
