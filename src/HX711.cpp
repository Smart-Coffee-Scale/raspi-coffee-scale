#include "HX711.h"
#include <pigpio.h>
#include <unistd.h>

// Constants
const float kScale = 2280.0f; // To-do: Placeholder for now. Adjust this based on calibration weight
const int kSampleTimes = 10;

// ------------------------------------------------------------------

HX711::HX711(int dataPin, int clockPin)
    : m_dataPin(dataPin),
      m_clockPin(clockPin)
{
}

// ------------------------------------------------------------------

void HX711::scaleInitialise()
{
    gpioSetup();
    tare(kSampleTimes);
    setScale(kScale);
}

// ------------------------------------------------------------------

float HX711::readWeight(int times)
{
    long sum = 0;
    for (int i = 0; i < times; i++)
    {
        sum += readRaw();
        usleep(5000);
    }

    return (sum - m_offset) / m_scale;
}

// ------------------------------------------------------------------

void HX711::gpioSetup()
{
    gpioSetMode(m_dataPin, PI_INPUT);
    gpioSetMode(m_clockPin, PI_OUTPUT);
    gpioWrite(m_clockPin, 0);
}

// ------------------------------------------------------------------

void HX711::tare(int times)
{
    long sum;
    for (int i = 0; i < times; i++)
    {
        sum += readRaw();
        usleep(5000);
    }
    setOffset(sum / times);
}

// ------------------------------------------------------------------

void HX711::setScale(float scale)
{
    m_scale = scale;
}

// ------------------------------------------------------------------

void HX711::setOffset(long offset)
{
    m_offset = offset;
}

// ------------------------------------------------------------------

long HX711::readRaw()
{
    // Wait for Data
    while (gpioRead(m_dataPin) == 1)
    {
        usleep(1);
    };

    long value = 0;
    for (int i = 0; i < 24; i++)
    {
        gpioWrite(m_clockPin, 1);
        value = (value << 1) | gpioRead(m_dataPin);
        gpioWrite(m_clockPin, 0);
    }

    // set gain = 128(1 pulse)
    gpioWrite(m_clockPin, 1);
    gpioWrite(m_clockPin, 0);

    // Convert to signed 24-bit int
    if (value & 0x800000)
    {
        value |= ~0xFFFFFF; // sign extend
    }
    return value;
}