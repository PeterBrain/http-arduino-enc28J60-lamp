# Homebridge Arduino ENC28J60 Light
Added HomeKit support to my selfmade LED roomlamp using Raspberry, Arduino and ENC28J60 Module.
The lamp is dimmable, sends its status, can be turned on and off with a physical switch and can be controlled via a webbrowser.

Special thanks to:
* Nick Farina (nfarina) - Creator of Homebridge https://github.com/nfarina/homebridge
* Adrian Rudman (rudders) - Creator of http-plugin https://github.com/rudders/homebridge-http
* Jean-Claude Wippler (jcw) - Creator of the EtherCard Library https://github.com/jcw/ethercard
* -->ME<--... i'm genious (pretty large ego) ;P


## Homebridge
Soon the configurationfile of homebridge will appear.

## Arduino
I just uploaded my latest Arduino project. It has some bugs though. :(

Pin connection Diagramm for ENC28J60
CS - Pin 10
SI - Pin 11
SO - Pin 12
SCK - Pin 13
GND - GND
VCC - 3.3V

Breadboard testing with LED:
LED+ (with 270Ω Resistor) - Digital 9 //I didn't calculate the resistor value, but the led is still fine
LED- - GND

Physical switch - Digital 8 (Don't directly connect ~230/110V to arduino... doesn't smell that good. Use a relay to galvanically insulate the circuits)


## Hardware
Don't be afraid, it isn't that much hardware. More precisely, just one electronic component is needed to make it work... for testing purpose on the actual lamp.

The one component is a MOSFET and is needed to handle the high current, that my lamp is consuming. On a breadboard one simple LED and a resistor wired to arduino is enough for testing.




If anyone of you has got some superior skills and knows a more efficient way to code, you are welcome to create an issue
