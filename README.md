# lightthingie
## Introduction
After digging out a decorative LED wire I found out
that the controller for the led lights broke.  
The LED wire consists of 100 LEDs that are connected in parallel without a resistor before them.
Weird and I'm sure not an electrical safe choice definitely.
Time to build my own controller then!

The strip is split into two wires and is connected to the original controller like that:  
![Led Connection](/pictures/ledconnection.png)  
The wire splitting ensures, that half of the strip can be controlled seperate from the other half.  
Implementing this can be achieved with an H-Bridge.  
In this case I used a L293D PWM-Driver to drive the lanes seperately.

## Implementation
For the implementation I used a NodeMCU v3 Dev-Board in conjunction with an L293D PWM-Driver.  

This is how the implementation looked like on a breadboard:  
![breadboard implementation](/pictures/breadboard.png)  
![final board implementation](/pictures/final%20board.png)  
![](/pictures/IMG_20201023_182236.jpg)  
![](/pictures/IMG_20201023_182259.jpg)  
![](/pictures/IMG_20201023_182320_BURST1.jpg)  

## Features
The implementation has the following features:
- Alexa integration
- OTA Updating
- Weird flickering from time to time ':D
- A fading mode
