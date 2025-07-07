# include "HX711.h"
# include <iostream>
#include <unistd.h>
#include <pigpio.h>

// Constants
const int kDataPin = 6; // placeholder for now
const int kClockPin = 5;
const int kSampleTimes = 10;

int main() {
    if (gpioInitialise() < 0)
    {
        std::cerr << "Failed to init pigpio" << std::endl;
        return 1;
    }

    HX711 scale(kDataPin, kClockPin);
    scale.scaleInitialise();

    while (true)
    {
        float weight = scale.readWeight(kSampleTimes);
        std::cout << "Weight: " << weight << "g" << std::endl;
        usleep(500000); // 0.5s
    }

    gpioTerminate();

    return 0;
}