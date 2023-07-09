# OptiLight

## Presentation

This is an opensource project, providing a simple solution to synchronize a single strip of leds with you screen on a Windows Computer through any Arduino device.
This project offers a high performance synchronizatriuon, with low latency.

## Configuration

By default, the project is configured to work with a 40 leds strip and a 1920*1080 screen.
If you're willing to modify these defaults values, download the project and build it on your own (I recommand using /O2 optimization, and all other optimizations in favor of code speed).

There are some things you might want to change : 

in OptiLight/Source.cpp :

    #define WIDTH 1920 // Width of the screen
    #define HEIGHT 1080 // Heigth of the screen
    #define NB_LED 40 // The number of leds
    #define ECH_X 24 // Sampling on X axis
    #define ECH_Y 27 // Sampling on Y axis

in OptiLight_Arduino/OptiLight_Arduino.ino

    #define NB_LED 40 // The number of leds
    #define LED_PIN 4 // The data pin.

## Licence

No Licence. Feel free to download, use, modify, share this project at your will 😃

## Credits

> Guilhem RICHAUD