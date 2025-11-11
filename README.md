Library to make make it a bit easier to interact with the CAN-FD peripherals on some STM32 microcontrollers. 

Some inspiration taken from the various "ACAN" micro controller CAN libraries, as well as auto-wp's MCP2515
library. The latter of which I've used EXTENSIVELY, and has allowed me to build some very cool things very 
quickly, and look like the hero at work. Alas, pretty much everything we use is CAN-FD lately. I messed 
around with the MCP2517/MCP2518 chips for quite a while, but I could just never get them to work reliably.
Figured the best way forward was to use a micro with a built in CAN-FD peripheral. Couldn't get any existing 
library to work reliably, so I ended up writing my own. Although, now that I know a bit more about ST clock 
timing and HAL troubleshooting, I'm sure I could get the others to work. 
