
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



## Supported Hardware
Currently only tested on the following:
- STM32 Nucleo H753ZI
- STM32 Nucleo G474RE
- STM32 Nucleo G0B1RE


## Demo
Just to show how easy it is to get a simple periodic send up and running. Default bitrate is 500k/2M.

    #include <Arduino.h>
    #include "STM32DuinoCANFD.hpp"

    FDCAN_Frame SendFrame;
    FDCAN_Instance can(FDCAN_Channel::CH1);

    uint64_t sendLoop = 0;

    void setup() {
        Serial.begin(115200);
        Serial.println("Starting Up");

        FDCAN_Settings Settings;

        if (can.begin(&Settings) != FDCAN_Status::OK) {
            Serial.println("Failed to initialize CAN-FD Channel");
            while (true) { }
        }
    }

    void loop() {
        // send every 1000ms
        if (micros() - sendLoop >= 1000 * 1000) {
            SendFrame.canId = 0x100;
            SendFrame.canDlc = 2;
            SendFrame.SetUnsigned(42, 0, 8);

            if (can.sendFrame(&SendFrame) != FDCAN_Status::OK) {
                Serial.print(millis());
                Serial.println(" - error sending can frame");
            }
            sendLoop = micros();
        }
    }

